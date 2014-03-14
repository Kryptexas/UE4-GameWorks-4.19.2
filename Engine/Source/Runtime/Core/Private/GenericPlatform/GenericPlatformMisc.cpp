// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GenericPlatformMisc.cpp: Generic implementations of misc platform functions
=============================================================================*/

#include "CorePrivate.h"
#include "MallocAnsi.h"
#include "GenericApplication.h"
#include "GenericPlatformChunkInstall.h"
#include "HAL/FileManagerGeneric.h"
#include "ModuleManager.h"
#include "VarargsHelper.h"
#include "SecureHash.h"

#include "UProjectInfo.h"

#if UE_ENABLE_ICU
#include <unicode/locid.h>
#endif

DEFINE_LOG_CATEGORY_STATIC(LogGenericPlatformMisc, Log, All);

/** Holds an override path if a program has special needs */
FString OverrideGameDir;

/* EBuildConfigurations interface
 *****************************************************************************/

namespace EBuildConfigurations
{
	EBuildConfigurations::Type FromString( const FString& Configuration )
	{
		if (FCString::Strcmp(*Configuration, TEXT("Debug")) == 0)
		{
			return Debug;
		}
		else if (FCString::Strcmp(*Configuration, TEXT("DebugGame")) == 0)
		{
			return DebugGame;
		}
		else if (FCString::Strcmp(*Configuration, TEXT("Development")) == 0)
		{
			return Development;
		}
		else if (FCString::Strcmp(*Configuration, TEXT("Shipping")) == 0)
		{
			return Shipping;
		}
		else if(FCString::Strcmp(*Configuration, TEXT("Test")) == 0)
		{
			return Test;
		}

		return Unknown;
	}


	const TCHAR* ToString( EBuildConfigurations::Type Configuration )
	{
		switch (Configuration)
		{
			case Debug:
				return TEXT("Debug");

			case DebugGame:
				return TEXT("DebugGame");

			case Development:
				return TEXT("Development");

			case Shipping:
				return TEXT("Shipping");

			case Test:
				return TEXT("Test");

			default:
				return TEXT("Unknown");
		}
	}

	FText ToText( EBuildConfigurations::Type Configuration )
	{
		switch (Configuration)
		{
		case Debug:
			return NSLOCTEXT("UnrealBuildConfigurations", "DebugName", "Debug");

		case DebugGame:
			return NSLOCTEXT("UnrealBuildConfigurations", "DebugGameName", "DebugGame");

		case Development:
			return NSLOCTEXT("UnrealBuildConfigurations", "DevelopmentName", "Development");

		case Shipping:
			return NSLOCTEXT("UnrealBuildConfigurations", "ShippingName", "Shipping");

		case Test:
			return NSLOCTEXT("UnrealBuildConfigurations", "TestName", "Test");

		default:
			return NSLOCTEXT("UnrealBuildConfigurations", "UnknownName", "Unknown");
		}
	}
}


/* EBuildConfigurations interface
 *****************************************************************************/

namespace EBuildTargets
{
	EBuildTargets::Type FromString( const FString& Target )
	{
		if (FCString::Strcmp(*Target, TEXT("Editor")) == 0)
		{
			return Editor;
		}
		else if (FCString::Strcmp(*Target, TEXT("Game")) == 0)
		{
			return Game;
		}
		else if (FCString::Strcmp(*Target, TEXT("Server")) == 0)
		{
			return Server;
		}

		return Unknown;
	}


	const TCHAR* ToString( EBuildTargets::Type Target )
	{
		switch (Target)
		{
			case Editor:
				return TEXT("Editor");

			case Game:
				return TEXT("Game");

			case Server:
				return TEXT("Server");

			default:
				return TEXT("Unknown");
		}
	}
}


/* FGenericPlatformMisc interface
 *****************************************************************************/
#if !UE_BUILD_SHIPPING
	bool FGenericPlatformMisc::bShouldPromptForRemoteDebugging = false;
	bool FGenericPlatformMisc::bPromptForRemoteDebugOnEnsure = false;
#endif	//#if !UE_BUILD_SHIPPING

GenericApplication* FGenericPlatformMisc::CreateApplication()
{
	return new GenericApplication( nullptr );
}

TArray<uint8> FGenericPlatformMisc::GetMacAddress()
{
	return TArray<uint8>();
}

FString FGenericPlatformMisc::GetMacAddressString()
{
	TArray<uint8> MacAddr = FPlatformMisc::GetMacAddress();
	FString Result;
	for (TArray<uint8>::TConstIterator it(MacAddr);it;++it)
	{
		Result += FString::Printf(TEXT("%02x"),*it);
	}
	return Result;
}

FString FGenericPlatformMisc::GetHashedMacAddressString()
{
	return FMD5::HashAnsiString(*FPlatformMisc::GetMacAddressString());
}

FString FGenericPlatformMisc::GetUniqueDeviceId()
{
	return FPlatformMisc::GetHashedMacAddressString();
}

void FGenericPlatformMisc::SubmitErrorReport( const TCHAR* InErrorHist, EErrorReportMode::Type InMode )
{
	UE_LOG(LogGenericPlatformMisc, Error, TEXT("This platform cannot submit a crash report. Report was:\n%s"), InErrorHist);
}

void FGenericPlatformMisc::MemoryBarrier()
{
}

void FGenericPlatformMisc::HandleIOFailure( const TCHAR* Filename )
{
	UE_LOG(LogGenericPlatformMisc, Fatal,TEXT("I/O failure operating on '%s'"), Filename ? Filename : TEXT("Unknown file"));
}

bool FGenericPlatformMisc::GetRegistryString(const FString& InRegistryKey, const FString& InValueName, bool bPerUserSetting, FString& OutValue)
{
	// By default, fail.
	return false;
}

void FGenericPlatformMisc::LowLevelOutputDebugString( const TCHAR *Message )
{
	FPlatformMisc::LocalPrint( Message );
}

void FGenericPlatformMisc::LowLevelOutputDebugStringf(const TCHAR *Fmt, ... )
{
	GROWABLE_LOGF(
		FPlatformMisc::LowLevelOutputDebugString( Buffer );
	);
}

void FGenericPlatformMisc::LocalPrint( const TCHAR* Str )
{
#if PLATFORM_USE_LS_SPEC_FOR_WIDECHAR
	printf("%ls", Str);
#else
	printf("%s", Str);
#endif
}

void FGenericPlatformMisc::RequestExit( bool Force )
{
	UE_LOG(LogGenericPlatformMisc, Log,  TEXT("FPlatformMisc::RequestExit(%i)"), Force );
	if( Force )
	{
		// Force immediate exit.
		// Dangerous because config code isn't flushed, global destructors aren't called, etc.
		// Suppress abort message and MS reports.
		abort();
	}
	else
	{
		// Tell the platform specific code we want to exit cleanly from the main loop.
		GIsRequestingExit = 1;
	}
}

const TCHAR* FGenericPlatformMisc::GetSystemErrorMessage(TCHAR* OutBuffer, int32 BufferCount, int32 Error)
{
	const TCHAR* Message = TEXT("No system errors available on this platform.");
	check(OutBuffer && BufferCount > 80);
	Error = 0;
	FCString::Strcpy(OutBuffer, BufferCount - 1, Message);
	return OutBuffer;
}

void FGenericPlatformMisc::ClipboardCopy(const TCHAR* Str)
{

}
void FGenericPlatformMisc:: ClipboardPaste(class FString& Dest)
{
	Dest = FString();
}

void FGenericPlatformMisc::CreateGuid(FGuid& Guid)
{
	int32 Year=0, Month=0, DayOfWeek=0, Day=0, Hour=0, Min=0, Sec=0, MSec=0;

	FPlatformTime::SystemTime(Year, Month, DayOfWeek, Day, Hour, Min, Sec, MSec);

	Guid = FGuid(Day | (Hour << 16), Month | (Sec << 16), MSec | (Min << 16), Year ^ FPlatformTime::Cycles());
}

EAppReturnType::Type FGenericPlatformMisc::MessageBoxExt( EAppMsgType::Type MsgType, const TCHAR* Text, const TCHAR* Caption )
{
	if (GWarn)
	{
		UE_LOG(LogGenericPlatformMisc, Warning, TEXT("Cannot display dialog box on this platform: %s : %s"), Caption, Text);
	}

	switch(MsgType)
	{
	case EAppMsgType::Ok:
		return EAppReturnType::Ok; // Ok
	case EAppMsgType::YesNo:
		return EAppReturnType::No; // No
	case EAppMsgType::OkCancel:
		return EAppReturnType::Cancel; // Cancel
	case EAppMsgType::YesNoCancel:
		return EAppReturnType::Cancel; // Cancel
	case EAppMsgType::CancelRetryContinue:
		return EAppReturnType::Cancel; // Cancel
	case EAppMsgType::YesNoYesAllNoAll:
		return EAppReturnType::No; // No
	case EAppMsgType::YesNoYesAllNoAllCancel:
		return EAppReturnType::Yes; // Yes
	default:
		check(0);
	}
	return EAppReturnType::Cancel; // Cancel
}

const TCHAR* FGenericPlatformMisc::RootDir()
{
	static FString Path;
	if (Path.Len() == 0)
	{
		FString TempPath = FPaths::EngineDir();
		int32 chopPos = TempPath.Find(TEXT("/Engine"));
		if (chopPos != INDEX_NONE)
		{
			TempPath = TempPath.Left(chopPos + 1);
			TempPath = FPaths::ConvertRelativePathToFull(TempPath);
			Path = TempPath;
		}
		else
		{
			Path = FPlatformProcess::BaseDir();

			// if the path ends in a separator, remove it
			if( Path.Right(1)==TEXT("/") )
			{
				Path = Path.LeftChop( 1 );
			}

			// keep going until we've removed Binaries
#if IS_MONOLITHIC && !IS_PROGRAM
			int32 pos = Path.Find(*FString::Printf(TEXT("/%s/Binaries"), GGameName));
#else
			int32 pos = Path.Find(TEXT("/Engine/Binaries"), ESearchCase::IgnoreCase);
#endif
			if ( pos != INDEX_NONE )
			{
				Path = Path.Left(pos + 1);
			}
			else
			{
				pos = Path.Find(TEXT("/../Binaries"), ESearchCase::IgnoreCase);
				if ( pos != INDEX_NONE )
				{
					Path = Path.Left(pos + 1) + TEXT("../../");
				}
				else
				{
					while( Path.Len() && Path.Right(1)!=TEXT("/") )
					{
						Path = Path.LeftChop( 1 );
					}
				}

			}
		}
	}
	return *Path;
}

const TCHAR* FGenericPlatformMisc::EngineDir()
{
	static FString EngineDirectory = TEXT("");
	if (EngineDirectory.Len() == 0)
	{
		// See if we are a root-level project
		FString CheckDir = TEXT("../../../Engine/");
#if PLATFORM_DESKTOP
		//@todo. Need to have a define specific for this scenario??
		if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*(CheckDir / TEXT("Binaries"))))
		{
			EngineDirectory = CheckDir;
		}
		else
		{
#ifdef UE_ENGINE_DIRECTORY
			CheckDir = TEXT( PREPROCESSOR_TO_STRING(UE_ENGINE_DIRECTORY) );
#endif
			if ((CheckDir.Len() == 0) || 
				(FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*(CheckDir / TEXT("Binaries"))) == false))
			{
				// Try using the registry to find the 'installed' version
				CheckDir = FRocketSupport::GetInstalledEngineDirectory();
				if ((CheckDir.Len() != 0) && (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*(CheckDir / TEXT("Binaries")))))
				{
					EngineDirectory = CheckDir;
				}
			}
			else
			{
				EngineDirectory = CheckDir;
			}
		}
#else
		EngineDirectory = CheckDir;
#endif
		if (EngineDirectory.Len() == 0)
		{
			// Temporary work-around for legacy dependency on ../../../ (re Lightmass)
			EngineDirectory = TEXT("../../../Engine/");
			UE_LOG(LogGenericPlatformMisc, Warning, TEXT("Failed to determine engine directory: Defaulting to %s"), *EngineDirectory);
		}
	}
	return *EngineDirectory;
}

const TCHAR* FGenericPlatformMisc::GetNullRHIShaderFormat()
{
	return TEXT("PCD3D_SM5");
}

IPlatformChunkInstall* FGenericPlatformMisc::GetPlatformChunkInstall()
{
	static FGenericPlatformChunkInstall Singleton;
	return &Singleton;
}

FLinearColor FGenericPlatformMisc::GetScreenPixelColor(const struct FVector2D& InScreenPos, float InGamma)
{ 
	return FLinearColor::Black;
}

void GenericPlatformMisc_GetProjectFilePathGameDir(FString& OutGameDir)
{
	// Here we derive the game path from the project file location.
	FString BasePath = FPaths::GetPath(FPaths::GetProjectFilePath());
	FPaths::NormalizeFilename(BasePath);
	BasePath = FFileManagerGeneric::DefaultConvertToRelativePath(*BasePath);
	OutGameDir = FString::Printf(TEXT("%s/"), *BasePath);
}

void GenericPlatformMisc_GetAppUserGameDir(FString& OutGameDir)
{
	// Without a game name, use the application settings directory in rocket
	FString AppSettingsDir = FPlatformProcess::ApplicationSettingsDir();
	FPaths::NormalizeFilename(AppSettingsDir);
	AppSettingsDir = FFileManagerGeneric::DefaultConvertToRelativePath(*AppSettingsDir);
	if (AppSettingsDir.EndsWith(TEXT("/")) == false)
	{
		AppSettingsDir.AppendChar(TEXT('/'));
	}

	AppSettingsDir += TEXT("Engine/");

	OutGameDir = AppSettingsDir;
}

const TCHAR* FGenericPlatformMisc::GameDir()
{
	static FString GameDir = TEXT("");

	// track if last time we called this function the .ini was ready and had fixed the GGameName case
	static bool bWasIniReady = false;
	bool bIsIniReady = GConfig && GConfig->IsReadyForUse();
	if (bWasIniReady != bIsIniReady)
	{
		GameDir = TEXT("");
		bWasIniReady = bIsIniReady;
	}

	// try using the override game dir if it exists, which will override all below logic
	if (GameDir.Len() == 0)
	{
		GameDir = OverrideGameDir;
				}

	if (GameDir.Len() == 0)
				{
		if (FPlatformProperties::IsProgram())
			{
				// monolithic, game-agnostic executables, the ini is in Engine/Config/Platform
				GameDir = FString::Printf(TEXT("../../../Engine/Programs/%s/"), GGameName);
			}
		else
		{
			if (FPaths::IsProjectFilePathSet())
			{
				GenericPlatformMisc_GetProjectFilePathGameDir(GameDir);
			}
			else if ( FApp::HasGameName() )
			{
				if (FPlatformProperties::IsMonolithicBuild() == false)
				{
					// No game project file, but has a game name, use the game folder next to the working directory
					GameDir = FString::Printf(TEXT("../../../%s/"), GGameName);
					FString GameBinariesDir = GameDir / TEXT("Binaries/");
					if (FPlatformFileManager::Get().GetPlatformFile().DirectoryExists(*GameBinariesDir) == false)
					{
						// The game binaries folder was *not* found
						// 
						FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Failed to find game directory: %s\n"), *GameDir);

						// Use the uprojectdirs
						FUProjectInfoHelper::FillProjectInfo();
						FString GameProjectFile = FUProjectInfoHelper::GetProjectForGame(GGameName);
						if (GameProjectFile.IsEmpty() == false)
						{
							// We found a project folder for the game
							FPaths::SetProjectFilePath(GameProjectFile);
							GameDir = FPaths::GetPath(GameProjectFile);
							if (GameDir.EndsWith(TEXT("/")) == false)
							{
								GameDir += TEXT("/");
							}
						}
					}
				}
				else
				{
#if !PLATFORM_DESKTOP
					GameDir = FString::Printf(TEXT("../../../%s/"), GGameName);
#else
					// This assumes the game executable is in <GAME>/Binaries/<PLATFORM>
					GameDir = TEXT("../../");

					// Determine a relative path that includes the game folder itself, if possible...
					FString LocalBaseDir = FString(FPlatformProcess::BaseDir());
					FString LocalRootDir = FPaths::RootDir();
					FString BaseToRoot = LocalRootDir;
					FPaths::MakePathRelativeTo(BaseToRoot, *LocalBaseDir);
					FString LocalGameDir = LocalBaseDir / TEXT("../../");
					FPaths::CollapseRelativeDirectories(LocalGameDir);
					FPaths::MakePathRelativeTo(LocalGameDir, *(FPaths::RootDir()));
					LocalGameDir = BaseToRoot / LocalGameDir;
					if (LocalGameDir.EndsWith(TEXT("/")) == false)
					{
						LocalGameDir += TEXT("/");
					}

					FString CheckLocal = FPaths::ConvertRelativePathToFull(LocalGameDir);
					FString CheckGame = FPaths::ConvertRelativePathToFull(GameDir);
					if (CheckLocal == CheckGame)
					{
						GameDir = LocalGameDir;
					}

					if (GameDir.EndsWith(TEXT("/")) == false)
					{
						GameDir += TEXT("/");
					}
#endif
				}
			}
			else if ( FRocketSupport::IsRocket() ) 
			{
				GenericPlatformMisc_GetAppUserGameDir(GameDir);
			}
			else
			{
				// Not a program, no project file, no game name, not rocket... use the engine dir
				GameDir = FPaths::EngineDir();
			}
		}
	}

	return *GameDir;
}

uint32 FGenericPlatformMisc::GetStandardPrintableKeyMap(uint16* KeyCodes, FString* KeyNames, uint32 MaxMappings, bool bMapUppercaseKeys, bool bMapLowercaseKeys)
{
	uint32 NumMappings = 0;

#define ADDKEYMAP(KeyCode, KeyName)		if (NumMappings<MaxMappings) { KeyCodes[NumMappings]=KeyCode; KeyNames[NumMappings]=KeyName; ++NumMappings; };

	ADDKEYMAP( '0', TEXT("Zero") );
	ADDKEYMAP( '1', TEXT("One") );
	ADDKEYMAP( '2', TEXT("Two") );
	ADDKEYMAP( '3', TEXT("Three") );
	ADDKEYMAP( '4', TEXT("Four") );
	ADDKEYMAP( '5', TEXT("Five") );
	ADDKEYMAP( '6', TEXT("Six") );
	ADDKEYMAP( '7', TEXT("Seven") );
	ADDKEYMAP( '8', TEXT("Eight") );
	ADDKEYMAP( '9', TEXT("Nine") );

	// we map both upper and lower
	if (bMapUppercaseKeys)
	{
		ADDKEYMAP( 'A', TEXT("A") );
		ADDKEYMAP( 'B', TEXT("B") );
		ADDKEYMAP( 'C', TEXT("C") );
		ADDKEYMAP( 'D', TEXT("D") );
		ADDKEYMAP( 'E', TEXT("E") );
		ADDKEYMAP( 'F', TEXT("F") );
		ADDKEYMAP( 'G', TEXT("G") );
		ADDKEYMAP( 'H', TEXT("H") );
		ADDKEYMAP( 'I', TEXT("I") );
		ADDKEYMAP( 'J', TEXT("J") );
		ADDKEYMAP( 'K', TEXT("K") );
		ADDKEYMAP( 'L', TEXT("L") );
		ADDKEYMAP( 'M', TEXT("M") );
		ADDKEYMAP( 'N', TEXT("N") );
		ADDKEYMAP( 'O', TEXT("O") );
		ADDKEYMAP( 'P', TEXT("P") );
		ADDKEYMAP( 'Q', TEXT("Q") );
		ADDKEYMAP( 'R', TEXT("R") );
		ADDKEYMAP( 'S', TEXT("S") );
		ADDKEYMAP( 'T', TEXT("T") );
		ADDKEYMAP( 'U', TEXT("U") );
		ADDKEYMAP( 'V', TEXT("V") );
		ADDKEYMAP( 'W', TEXT("W") );
		ADDKEYMAP( 'X', TEXT("X") );
		ADDKEYMAP( 'Y', TEXT("Y") );
		ADDKEYMAP( 'Z', TEXT("Z") );
	}

	if (bMapLowercaseKeys)
	{
		ADDKEYMAP( 'a', TEXT("A") );
		ADDKEYMAP( 'b', TEXT("B") );
		ADDKEYMAP( 'c', TEXT("C") );
		ADDKEYMAP( 'd', TEXT("D") );
		ADDKEYMAP( 'e', TEXT("E") );
		ADDKEYMAP( 'f', TEXT("F") );
		ADDKEYMAP( 'g', TEXT("G") );
		ADDKEYMAP( 'h', TEXT("H") );
		ADDKEYMAP( 'i', TEXT("I") );
		ADDKEYMAP( 'j', TEXT("J") );
		ADDKEYMAP( 'k', TEXT("K") );
		ADDKEYMAP( 'l', TEXT("L") );
		ADDKEYMAP( 'm', TEXT("M") );
		ADDKEYMAP( 'n', TEXT("N") );
		ADDKEYMAP( 'o', TEXT("O") );
		ADDKEYMAP( 'p', TEXT("P") );
		ADDKEYMAP( 'q', TEXT("Q") );
		ADDKEYMAP( 'r', TEXT("R") );
		ADDKEYMAP( 's', TEXT("S") );
		ADDKEYMAP( 't', TEXT("T") );
		ADDKEYMAP( 'u', TEXT("U") );
		ADDKEYMAP( 'v', TEXT("V") );
		ADDKEYMAP( 'w', TEXT("W") );
		ADDKEYMAP( 'x', TEXT("X") );
		ADDKEYMAP( 'y', TEXT("Y") );
		ADDKEYMAP( 'z', TEXT("Z") );
	}

	ADDKEYMAP( ';', TEXT("Semicolon") );
	ADDKEYMAP( '=', TEXT("Equals") );
	ADDKEYMAP( ',', TEXT("Comma") );
	ADDKEYMAP( '-', TEXT("Underscore") );
	ADDKEYMAP( '.', TEXT("Period") );
	ADDKEYMAP( '/', TEXT("Slash") );
	ADDKEYMAP( '`', TEXT("Tilde") );
	ADDKEYMAP( '[', TEXT("LeftBracket") );
	ADDKEYMAP( '\\', TEXT("Backslash") );
	ADDKEYMAP( ']', TEXT("RightBracket") );
	ADDKEYMAP( '\'', TEXT("Quote") );
	ADDKEYMAP( ' ', TEXT("SpaceBar") );

	return NumMappings;
}

const TCHAR* FGenericPlatformMisc::GetUBTPlatform()
{
	return TEXT( PREPROCESSOR_TO_STRING(UBT_COMPILED_PLATFORM) );
}

const TCHAR* FGenericPlatformMisc::GetDefaultDeviceProfileName()
{
	return TEXT("Default");
}

void FGenericPlatformMisc::SetOverrideGameDir(const FString& InOverrideDir)
{
	OverrideGameDir = InOverrideDir;
}

int32 FGenericPlatformMisc::NumberOfCoresIncludingHyperthreads()
{
	return FPlatformMisc::NumberOfCores();
}

int32 FGenericPlatformMisc::NumberOfWorkerThreadsToSpawn()
{
	static int32 MaxGameThreads = 4;
	static int32 MaxThreads = 16;

	int32 NumberOfCores = FPlatformMisc::NumberOfCores();
	int32 MaxWorkerThreadsWanted = (IsRunningGame() || IsRunningDedicatedServer() || IsRunningClientOnly()) ? MaxGameThreads : MaxThreads;
	// need to spawn at least one worker thread (see FTaskGraphImplementation)
	return FMath::Max(FMath::Min(NumberOfCores - 1, MaxWorkerThreadsWanted), 1);
}

void FGenericPlatformMisc::GetValidTargetPlatforms(class TArray<class FString>& TargetPlatformNames)
{
	// by default, just return the running PlatformName as the only TargetPlatform we support
	TargetPlatformNames.Add(FPlatformProperties::PlatformName());
}

FString FGenericPlatformMisc::GetDefaultLocale()
{
#if UE_ENABLE_ICU
	icu::Locale ICUDefaultLocale = icu::Locale::getDefault();
	return FString(ICUDefaultLocale.getName());
#else
	return TEXT("en");
#endif
}