// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

#include "WindowsErrorReport.h"
#include "XmlFile.h"
#include "CrashDebugHelperModule.h"
#include "../CrashReportUtil.h"

#include "AllowWindowsPlatformTypes.h"
#include <ShlObj.h>
#include "HideWindowsPlatformTypes.h"

#define LOCTEXT_NAMESPACE "CrashReportClient"

namespace
{
	/** Pointer to dynamically loaded crash diagnosis module */
	FCrashDebugHelperModule* CrashHelperModule;
}

FWindowsErrorReport::FWindowsErrorReport(const FString& Directory)
	: FGenericErrorReport(Directory)
{
	CrashHelperModule = &FModuleManager::LoadModuleChecked<FCrashDebugHelperModule>(FName("CrashDebugHelper"));
}

FWindowsErrorReport::~FWindowsErrorReport()
{
	CrashHelperModule->ShutdownModule();
}

FText FWindowsErrorReport::DiagnoseReport() const
{
	// Should check if there are local PDBs before doing anything
	auto CrashDebugHelper = CrashHelperModule->Get();
	if (!CrashDebugHelper)
	{
		// Not localized: should never be seen
		return FText::FromString(TEXT("Failed to load CrashDebugHelper"));
	}

	FString DumpFilename;
	if (!FindFirstReportFileWithExtension(DumpFilename, TEXT(".dmp")))
	{
		if (!FindFirstReportFileWithExtension(DumpFilename, TEXT(".mdmp")))
		{
			return LOCTEXT("MinidumpNotFound", "No minidump found for this crash");
		}
	}

	// Note: at time of writing this function always returns false, regardless of success
	CrashDebugHelper->CreateMinidumpDiagnosticReport(ReportDirectory / DumpFilename);

	// There's a callstack, so write it out to save the server trying to do it
	CrashDebugHelper->CrashInfo.GenerateReport(ReportDirectory / GDiagnosticsFilename);

	const auto& Exception = CrashDebugHelper->CrashInfo.Exception;
	return FormatReportDescription(Exception.ExceptionString, Exception.CallStackString);
}

FString FWindowsErrorReport::FindCrashedAppName() const
{
	TArray<uint8> FileData;
	if(!FFileHelper::LoadFileToArray(FileData, *(ReportDirectory / TEXT("Report.wer"))))
	{
		return "";
	}

	// Look backwards for AppPath= line
	auto Data = reinterpret_cast<TCHAR*>(FileData.GetData());
	TCHAR* Line = nullptr;
	for (int Index = FileData.Num() / sizeof(TCHAR); Index != 0; --Index)
	{
		auto& Char = Data[Index - 1];
		if (Char != '\n' && Char != '\r')
		{
			Line = &Char;
			continue;
		}
		
		// Zero terminate line for conversion to string below
		Char = 0;
		if (!Line)
		{
			continue;
		}

		static const TCHAR AppPathLineStart[] = TEXT("AppPath=");
		static const int AppPathIdLength = ARRAY_COUNT(AppPathLineStart) - 1;
		if (0 == FCString::Strncmp(Line, AppPathLineStart, AppPathIdLength))
		{
			return FPaths::GetCleanFilename(Line + AppPathIdLength);
		}
		Line = nullptr;
	}
	return "";
}

FString FWindowsErrorReport::FindMostRecentErrorReport()
{
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	auto DirectoryModifiedTime = FDateTime::MinValue();
	FString ReportDirectory;
	auto ReportFinder = MakeDirectoryVisitor([&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
		if (bIsDirectory)
		{
			auto TimeStamp = PlatformFile.GetTimeStamp(FilenameOrDirectory);
			if (TimeStamp > DirectoryModifiedTime)
			{
				ReportDirectory = FilenameOrDirectory;
				DirectoryModifiedTime = TimeStamp;
			}
		}
		return true;
	});

	TCHAR LocalAppDataPath[MAX_PATH];
	SHGetFolderPath(0, CSIDL_LOCAL_APPDATA, NULL, 0, LocalAppDataPath);

	PlatformFile.IterateDirectory(
		*(FString(LocalAppDataPath) / TEXT("Microsoft/Windows/WER/ReportQueue")),
		ReportFinder);

	return ReportDirectory;
}

#undef LOCTEXT_NAMESPACE
