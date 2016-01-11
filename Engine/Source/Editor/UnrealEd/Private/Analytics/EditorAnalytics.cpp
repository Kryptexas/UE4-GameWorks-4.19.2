// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "TargetPlatform.h"

#include "EngineAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"

#include "EditorAnalytics.h"
#include "GeneralProjectSettings.h"
#include "GameProjectGenerationModule.h"

#define LOCTEXT_NAMESPACE "EditorAnalytics"

namespace EditorAnalyticsDefs
{
	static const FTimespan SessionRecordExpiration(30, 0, 0, 0);	// 30 days
	static const FTimespan SessionRecordTimeout(0, 30, 0);			// 30 minutes
	static const FTimespan GlobalLockWaitTimeout(0, 0, 0, 0, 500);	// 1/2 second
	static const FString StoreId(TEXT("Epic Games"));
	static const FString CrashSessionToken(TEXT("Crashed"));
	static const FString DebuggerSessionToken(TEXT("Debugger"));
	static const FString AbnormalSessionToken(TEXT("AbnormalShutdown"));
	static const FString SessionRecordListSection(TEXT("List"));
	static const FString SessionRecordSectionPrefix(TEXT("Unreal Engine/Editor Sessions/"));
	static const FString EditorSessionsVersionString(TEXT("1_1"));
	static const FString CrashStoreKey(TEXT("IsCrash"));
	static const FString EngineVersionStoreKey(TEXT("EngineVersion"));
	static const FString TimestampStoreKey(TEXT("Timestamp"));
	static const FString DebuggerStoreKey(TEXT("IsDebugger"));
	static const FString GlobalLockName(TEXT("UE4_EditorAnalytics_Lock"));
	static const FString FalseValueString(TEXT("0"));
	static const FString TrueValueString(TEXT("1"));
}

/* Handles writing session records to platform's storage to track crashed and timed-out editor sessions */
class FSessionManager
{
public:
	FSessionManager()
		: bInitialized(false)
		, bEditorCrashed(false)
	{}

	void Initialize(bool bWaitForLock = true)
	{
		if (!FEngineAnalytics::IsAvailable())
		{
			return;
		}

		TArray<FSessionRecord> SessionRecordsToReport;

		{
			// Scoped lock
			FSystemWideCriticalSection StoredValuesLock(EditorAnalyticsDefs::GlobalLockName, bWaitForLock ? EditorAnalyticsDefs::GlobalLockWaitTimeout : FTimespan::Zero());

			// Get list of sessions in storage
			if (StoredValuesLock.IsValid() && BeginReadWriteRecords())
			{
				TArray<FSessionRecord> SessionRecordsToDelete;

				// Attempt check each stored session
				for (FSessionRecord& Record : SessionRecords)
				{
					FTimespan RecordAge = FDateTime::UtcNow() - Record.Timestamp;

					if (Record.bCrashed)
					{
						// Crashed sessions
						SessionRecordsToReport.Add(Record);
						SessionRecordsToDelete.Add(Record);
					}
					else if (RecordAge > EditorAnalyticsDefs::SessionRecordExpiration)
					{
						// Delete expired session records
						SessionRecordsToDelete.Add(Record);
					}
					else if (RecordAge > EditorAnalyticsDefs::SessionRecordTimeout)
					{
						// Timed out sessions
						SessionRecordsToReport.Add(Record);
						SessionRecordsToDelete.Add(Record);
					}
				}

				for (FSessionRecord& DeletingRecord : SessionRecordsToDelete)
				{
					DeleteStoredRecord(DeletingRecord);
				}

				// Create a session record for this session
				CreateAndWriteRecordForSession();

				// Update and release list of sessions in storage
				EndReadWriteRecords();

				// Register for crash callbacks
				FCoreDelegates::OnHandleSystemError.AddRaw(this, &FSessionManager::OnCrashing);

				bInitialized = true;
			}
		}

		for (FSessionRecord& ReportingSession : SessionRecordsToReport)
		{
			// Send error report for session that timed out or crashed
			SendAbnormalShutdownReport(ReportingSession);
		}
	}

	void Heartbeat()
	{
		if (!FEngineAnalytics::IsAvailable())
		{
			return;
		}

		if (!bInitialized)
		{
			// Try late initialization
			const bool bWaitForLock = false;
			Initialize(bWaitForLock);
		}

		// Update timestamp in the session record for this session 
		if (bInitialized)
		{
			CurrentSession.Timestamp = FDateTime::UtcNow();

			FPlatformMisc::SetStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::TimestampStoreKey, TimestampToString(CurrentSession.Timestamp));
		}
	}

	void Shutdown()
	{
		// Clear the session record for this session
		if (bInitialized)
		{
			FCoreDelegates::OnHandleSystemError.RemoveAll(this);

			if (!bEditorCrashed)
			{
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::CrashStoreKey);
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::EngineVersionStoreKey);
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::TimestampStoreKey);
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::DebuggerStoreKey);
			}

			bInitialized = false;
		}
	}

private:
	struct FSessionRecord
	{
		FString SessionId;
		FString EngineVersion;
		FDateTime Timestamp;
		bool bCrashed;
		bool bIsDebugger;
	};

private:
	bool BeginReadWriteRecords()
	{
		SessionRecords.Empty();

		// Lock and read the list of sessions in storage
		FString ListSectionName = GetStoreSectionString(EditorAnalyticsDefs::SessionRecordListSection);

		// Write list to SessionRecords member
		FString SessionListString;
		FPlatformMisc::GetStoredValue(EditorAnalyticsDefs::StoreId, ListSectionName, TEXT("SessionList"), SessionListString);

		// Parse SessionListString for session ids
		TArray<FString> SessionIds;
		SessionListString.ParseIntoArray(SessionIds, TEXT(","));

		// Retrieve all the sessions in the list from storage
		for (FString& SessionId : SessionIds)
		{
			FString SectionName = GetStoreSectionString(SessionId);

			FString IsCrashString;
			FString EngineVersionString;
			FString TimestampString;
			FString IsDebuggerString;

			if (FPlatformMisc::GetStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::CrashStoreKey, IsCrashString) &&
				FPlatformMisc::GetStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::EngineVersionStoreKey, EngineVersionString) &&
				FPlatformMisc::GetStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::TimestampStoreKey, TimestampString) &&
				FPlatformMisc::GetStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::DebuggerStoreKey, IsDebuggerString))
			{
				FSessionRecord NewRecord;
				NewRecord.SessionId = SessionId;
				NewRecord.EngineVersion = EngineVersionString;
				NewRecord.Timestamp = StringToTimestamp(TimestampString);
				NewRecord.bCrashed = IsCrashString == EditorAnalyticsDefs::TrueValueString;
				NewRecord.bIsDebugger = IsDebuggerString == EditorAnalyticsDefs::TrueValueString;

				SessionRecords.Add(NewRecord);
			}
			else
			{
				// Clean up orphaned values, if there are any
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::CrashStoreKey);
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::EngineVersionStoreKey);
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::TimestampStoreKey);
				FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::DebuggerStoreKey);
			}
		}

		return true;
	}

	void EndReadWriteRecords()
	{
		// Update the list of sessions in storage to match SessionRecords
		FString SessionListString;
		if (SessionRecords.Num() > 0)
		{
			for (FSessionRecord& Session : SessionRecords)
			{
				SessionListString.Append(Session.SessionId);
				SessionListString.Append(TEXT(","));
			}
			SessionListString.RemoveAt(SessionListString.Len() - 1);
		}

		FString ListSectionName = GetStoreSectionString(EditorAnalyticsDefs::SessionRecordListSection);
		FPlatformMisc::SetStoredValue(EditorAnalyticsDefs::StoreId, ListSectionName, TEXT("SessionList"), SessionListString);

		// Clear SessionRecords member
		SessionRecords.Empty();
	}

	void DeleteStoredRecord(const FSessionRecord& Record)
	{
		// Delete the session record in storage
		FString SessionId = Record.SessionId;
		FString SectionName = GetStoreSectionString(SessionId);

		FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::CrashStoreKey);
		FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::EngineVersionStoreKey);
		FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::TimestampStoreKey);
		FPlatformMisc::DeleteStoredValue(EditorAnalyticsDefs::StoreId, SectionName, EditorAnalyticsDefs::DebuggerStoreKey);

		// Remove the session record from SessionRecords list
		SessionRecords.RemoveAll([&SessionId](const FSessionRecord& X){ return X.SessionId == SessionId; });
	}

	void SendAbnormalShutdownReport(const FSessionRecord& Record)
	{
		FGameProjectGenerationModule& GameProjectModule = FModuleManager::LoadModuleChecked<FGameProjectGenerationModule>(TEXT("GameProjectGeneration"));
		bool bHasCode = GameProjectModule.Get().ProjectHasCodeFiles();

#if PLATFORM_WINDOWS
		const FString PlatformName(TEXT("Windows"));
#elif PLATFORM_MAC
		const FString PlatformName(TEXT("Mac"));
#elif PLATFORM_LINUX
		const FString PlatformName(TEXT("Linux"));
#else
		const FString PlatformName(TEXT("Unknown"));
#endif

		FGuid SessionId;
		FString SessionIdString = Record.SessionId;
		if (FGuid::Parse(SessionIdString, SessionId))
		{
			// convert session guid to one with braces for sending to analytics
			SessionIdString = SessionId.ToString(EGuidFormats::DigitsWithHyphensInBraces);
		}

		FString ShutdownTypeString = Record.bCrashed ? EditorAnalyticsDefs::CrashSessionToken :
			(Record.bIsDebugger ? EditorAnalyticsDefs::DebuggerSessionToken : EditorAnalyticsDefs::AbnormalSessionToken);

		TArray< FAnalyticsEventAttribute > AbnormalShutdownAttributes;
		AbnormalShutdownAttributes.Add(FAnalyticsEventAttribute(FString("SessionId"), SessionIdString));
		AbnormalShutdownAttributes.Add(FAnalyticsEventAttribute(FString("EngineVersion"), Record.EngineVersion));
		AbnormalShutdownAttributes.Add(FAnalyticsEventAttribute(FString("ShutdownType"), ShutdownTypeString));
		AbnormalShutdownAttributes.Add(FAnalyticsEventAttribute(FString("Timestamp"), Record.Timestamp.ToIso8601()));

		FEditorAnalytics::ReportEvent(TEXT("Editor.AbnormalShutdown"), PlatformName, bHasCode, AbnormalShutdownAttributes);
	}

	void CreateAndWriteRecordForSession()
	{
		FGuid SessionId;
		if (FGuid::Parse(FEngineAnalytics::GetProvider().GetSessionID(), SessionId))
		{
			// convert session guid to one without braces or other chars that might not be suitable for storage
			CurrentSession.SessionId = SessionId.ToString(EGuidFormats::DigitsWithHyphens);
		}
		else
		{
			CurrentSession.SessionId = FEngineAnalytics::GetProvider().GetSessionID();
		}

		CurrentSession.EngineVersion = GEngineVersion.ToString(EVersionComponent::Changelist);
		CurrentSession.Timestamp = FDateTime::UtcNow();
		CurrentSession.bCrashed = false;
		CurrentSession.bIsDebugger = FPlatformMisc::IsDebuggerPresent();
		CurrentSessionSectionName = GetStoreSectionString(CurrentSession.SessionId);

		FString IsDebuggerString = CurrentSession.bIsDebugger ? EditorAnalyticsDefs::TrueValueString : EditorAnalyticsDefs::FalseValueString;

		FPlatformMisc::SetStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::CrashStoreKey, EditorAnalyticsDefs::FalseValueString);
		FPlatformMisc::SetStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::EngineVersionStoreKey, CurrentSession.EngineVersion);
		FPlatformMisc::SetStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::TimestampStoreKey, TimestampToString(CurrentSession.Timestamp));
		FPlatformMisc::SetStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::DebuggerStoreKey, IsDebuggerString);

		SessionRecords.Add(CurrentSession);
	}

	void OnCrashing()
	{
		if (!bEditorCrashed && bInitialized)
		{
			bEditorCrashed = true;

			FPlatformMisc::SetStoredValue(EditorAnalyticsDefs::StoreId, CurrentSessionSectionName, EditorAnalyticsDefs::CrashStoreKey, EditorAnalyticsDefs::TrueValueString);
		}
	}

	static FString TimestampToString(FDateTime InTimestamp)
	{
		return LexicalConversion::ToString(InTimestamp.ToUnixTimestamp());
	}

	static FDateTime StringToTimestamp(FString InString)
	{
		int64 TimestampUnix;
		if (LexicalConversion::TryParseString(TimestampUnix, *InString))
		{
			return FDateTime::FromUnixTimestamp(TimestampUnix);
		}
		return FDateTime::MinValue();
	}

	static FString GetStoreSectionString(FString InSuffix)
	{
		return FString::Printf(TEXT("%s%s/%s"), *EditorAnalyticsDefs::SessionRecordSectionPrefix, *EditorAnalyticsDefs::EditorSessionsVersionString, *InSuffix);
	}

private:
	bool bInitialized;
	bool bEditorCrashed;
	FSessionRecord CurrentSession;
	FString CurrentSessionSectionName;
	TArray<FSessionRecord> SessionRecords;
};


TSharedPtr<FSessionManager> FEditorAnalytics::SessionManager;

void FEditorAnalytics::InitializeSessionManager()
{
	// Create the session manager singleton if we are a normal editor session
	if (GIsEditor && !IsRunningCommandlet() && !SessionManager.IsValid() )
	{
		SessionManager = MakeShareable(new FSessionManager());
		SessionManager->Initialize();
	}
}

void FEditorAnalytics::ShutdownSessionManager()
{
	// Destroy the session manager singleton if it exists
	if (SessionManager.IsValid())
	{
		SessionManager->Shutdown();
		SessionManager.Reset();
	}
}

void FEditorAnalytics::SessionHeartbeat()
{
	if (SessionManager.IsValid())
	{
		SessionManager->Heartbeat();
	}
}

void FEditorAnalytics::ReportBuildRequirementsFailure(FString EventName, FString PlatformName, bool bHasCode, int32 Requirements)
{
	TArray<FAnalyticsEventAttribute> ParamArray;
	ParamArray.Add(FAnalyticsEventAttribute(TEXT("Time"), 0.0));
	if (Requirements & ETargetPlatformReadyStatus::SDKNotFound)
	{
		ReportEvent(EventName, PlatformName, bHasCode, EAnalyticsErrorCodes::SDKNotFound, ParamArray);
	}
	if (Requirements & ETargetPlatformReadyStatus::ProvisionNotFound)
	{
		ReportEvent(EventName, PlatformName, bHasCode, EAnalyticsErrorCodes::ProvisionNotFound, ParamArray);
	}
	if (Requirements & ETargetPlatformReadyStatus::SigningKeyNotFound)
	{
		ReportEvent(EventName, PlatformName, bHasCode, EAnalyticsErrorCodes::CertificateNotFound, ParamArray);
	}
	if (Requirements & ETargetPlatformReadyStatus::CodeUnsupported)
	{
		ReportEvent(EventName, PlatformName, bHasCode, EAnalyticsErrorCodes::CodeUnsupported, ParamArray);
	}
	if (Requirements & ETargetPlatformReadyStatus::PluginsUnsupported)
	{
		ReportEvent(EventName, PlatformName, bHasCode, EAnalyticsErrorCodes::PluginsUnsupported, ParamArray);
	}
}

void FEditorAnalytics::ReportEvent(FString EventName, FString PlatformName, bool bHasCode)
{
	if( FEngineAnalytics::IsAvailable() )
	{
		const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ProjectID"), ProjectSettings.ProjectID.ToString()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Platform"), PlatformName));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ProjectType"), bHasCode ? TEXT("C++ Code") : TEXT("Content Only")));

		FEngineAnalytics::GetProvider().RecordEvent( EventName, ParamArray );
	}
}

void FEditorAnalytics::ReportEvent(FString EventName, FString PlatformName, bool bHasCode, TArray<FAnalyticsEventAttribute>& ExtraParams)
{
	if( FEngineAnalytics::IsAvailable() )
	{
		const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ProjectID"), ProjectSettings.ProjectID.ToString()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Platform"), PlatformName));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ProjectType"), bHasCode ? TEXT("C++ Code") : TEXT("Content Only")));
		ParamArray.Append(ExtraParams);

		FEngineAnalytics::GetProvider().RecordEvent( EventName, ParamArray );
	}
}

void FEditorAnalytics::ReportEvent(FString EventName, FString PlatformName, bool bHasCode, int32 ErrorCode, TArray<FAnalyticsEventAttribute>& ExtraParams)
{
	if( FEngineAnalytics::IsAvailable() )
	{
		const UGeneralProjectSettings& ProjectSettings = *GetDefault<UGeneralProjectSettings>();
		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ProjectID"), ProjectSettings.ProjectID.ToString()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Platform"), PlatformName));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ProjectType"), bHasCode ? TEXT("C++ Code") : TEXT("Content Only")));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ErrorCode"), ErrorCode));
		const FString ErrorMessage = TranslateErrorCode(ErrorCode);
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ErrorName"), ErrorMessage));
		ParamArray.Append(ExtraParams);

		FEngineAnalytics::GetProvider().RecordEvent( EventName, ParamArray );
	}
}

FString FEditorAnalytics::TranslateErrorCode(int32 ErrorCode)
{
	switch (ErrorCode)
	{
	case EAnalyticsErrorCodes::UATNotFound:
		return TEXT("UAT Not Found");
	case EAnalyticsErrorCodes::Unknown:
		return TEXT("Unknown Error");
	case EAnalyticsErrorCodes::Arguments:
		return TEXT("Invalid Arguments");
	case EAnalyticsErrorCodes::UnknownCommand:
		return TEXT("Unknown Command");
	case EAnalyticsErrorCodes::SDKNotFound:
		return TEXT("SDK Not Found");
	case EAnalyticsErrorCodes::ProvisionNotFound:
		return TEXT("Provision Not Found");
	case EAnalyticsErrorCodes::CertificateNotFound:
		return TEXT("Certificate Not Found");
	case EAnalyticsErrorCodes::ManifestNotFound:
		return TEXT("Manifest Not Found");
	case EAnalyticsErrorCodes::KeyNotFound:
		return TEXT("Key Not Found in Manifest");
	case EAnalyticsErrorCodes::ProvisionExpired:
		return TEXT("Provision Has Expired");
	case EAnalyticsErrorCodes::CertificateExpired:
		return TEXT("Certificate Has Expired");
	case EAnalyticsErrorCodes::CertificateProvisionMismatch:
		return TEXT("Certificate Doesn't Match Provision");
	case EAnalyticsErrorCodes::LauncherFailed:
		return TEXT("LauncherWorker Failed to Launch");
	case EAnalyticsErrorCodes::UATLaunchFailure:
		return TEXT("UAT Failed to Launch");
	case EAnalyticsErrorCodes::UnknownCookFailure:
		return TEXT("Unknown Cook Failure");
	case EAnalyticsErrorCodes::UnknownDeployFailure:
		return TEXT("Unknown Deploy Failure");
	case EAnalyticsErrorCodes::UnknownBuildFailure:
		return TEXT("Unknown Build Failure");
	case EAnalyticsErrorCodes::UnknownPackageFailure:
		return TEXT("Unknown Package Failure");
	case EAnalyticsErrorCodes::UnknownLaunchFailure:
		return TEXT("Unknown Launch Failure");
	case EAnalyticsErrorCodes::StageMissingFile:
		return TEXT("Could not find file for staging");
	case EAnalyticsErrorCodes::FailedToCreateIPA:
		return TEXT("Failed to Create IPA");
	case EAnalyticsErrorCodes::FailedToCodeSign:
		return TEXT("Failed to Code Sign");
	case EAnalyticsErrorCodes::DeviceBackupFailed:
		return TEXT("Failed to backup device");
	case EAnalyticsErrorCodes::AppUninstallFailed:
		return TEXT("Failed to Uninstall app");
	case EAnalyticsErrorCodes::AppInstallFailed:
		return TEXT("Failed to Install app");
	case EAnalyticsErrorCodes::AppNotFound:
		return TEXT("App package file not found for Install");
	case EAnalyticsErrorCodes::StubNotSignedCorrectly:
		return TEXT("Stub not signed correctly.");
	case EAnalyticsErrorCodes::IPAMissingInfoPList:
		return TEXT("Failed to find Info.plist in IPA");
	case EAnalyticsErrorCodes::DeleteFile:
		return TEXT("Could not delete file");
	case EAnalyticsErrorCodes::DeleteDirectory:
		return TEXT("Could not delete directory");
	case EAnalyticsErrorCodes::CreateDirectory:
		return TEXT("Could not create directory");
	case EAnalyticsErrorCodes::CopyFile:
		return TEXT("Could not copy file");
	case EAnalyticsErrorCodes::OnlyOneObbFileSupported:
		return TEXT("Android packaging supports only exactly one obb/pak file");
	case EAnalyticsErrorCodes::FailureGettingPackageInfo:
		return TEXT("Failed to get package info from APK file");
	case EAnalyticsErrorCodes::OnlyOneTargetConfigurationSupported:
		return TEXT("Android is only able to package a single target configuration");
	case EAnalyticsErrorCodes::ObbNotFound:
		return TEXT("OBB/PAK file not found");
	case EAnalyticsErrorCodes::AndroidBuildToolsPathNotFound:
		return TEXT("Android build-tools directory not found");
	case EAnalyticsErrorCodes::NoApkSuitableForArchitecture:
		return TEXT("No APK suitable for architecture found");
	case EAnalyticsErrorCodes::FailedToDeleteStagingDirectory :
		return TEXT("Failed to delete staging directory.  This could be because something is currently using the staging directory (ps4/xbox/etc)");
	case EAnalyticsErrorCodes::MissingExecutable:
		return LOCTEXT("UATErrorMissingExecutable", "Missing UE4Game binary.\nYou may have to build the UE4 project with your IDE. Alternatively, build using UnrealBuildTool with the commandline:\nUE4Game <Platform> <Configuration>").ToString();
	case EAnalyticsErrorCodes::FilesInstallFailed:
		return TEXT("Failed to deploy files to device.  Check to make sure your device is connected.");
	case EAnalyticsErrorCodes::DeviceNotSetupForDevelopment:
		return TEXT("Failed to launch on device.  Make sure your device has been enabled for development from within the Xcode Devices window.");
	case EAnalyticsErrorCodes::DeviceOSNewerThanSDK:
		return TEXT("Failed to launch on device.  Make sure your install of Xcode matches or is newer than the OS on your device.");
	case EAnalyticsErrorCodes::RemoteCertificatesNotFound:
		return TEXT("Failed to sign executable.  Make sure your developer certificates have been installed in the System Keychain on the remote Mac.");
	}
	return TEXT("Unknown Error");
}

bool FEditorAnalytics::ShouldElevateMessageThroughDialog(const int32 ErrorCode)
{
	return (EAnalyticsErrorCodes::Type)ErrorCode == EAnalyticsErrorCodes::MissingExecutable;
}

#undef LOCTEXT_NAMESPACE