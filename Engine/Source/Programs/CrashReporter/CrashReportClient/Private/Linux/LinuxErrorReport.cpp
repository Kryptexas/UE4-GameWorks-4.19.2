// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

#include "LinuxErrorReport.h"
#include "XmlFile.h"
#include "CrashDebugHelperModule.h"
#include "../CrashReportUtil.h"

#define LOCTEXT_NAMESPACE "CrashReportClient"

namespace
{
	/** Pointer to dynamically loaded crash diagnosis module */
	FCrashDebugHelperModule* CrashHelperModule;
}

FLinuxErrorReport::FLinuxErrorReport(const FString& Directory)
	: ReportDirectory(Directory)
{
	CrashHelperModule = &FModuleManager::LoadModuleChecked<FCrashDebugHelperModule>(FName("CrashDebugHelper"));

	auto FilenamesVisitor = MakeDirectoryVisitor([this](const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
		if (!bIsDirectory)
		{
			ReportFilenames.Push(FPaths::GetCleanFilename(FilenameOrDirectory));
		}
		return true;
	});
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*ReportDirectory, FilenamesVisitor);
}

FLinuxErrorReport::~FLinuxErrorReport()
{
	CrashHelperModule->ShutdownModule();
}

bool FLinuxErrorReport::SetUserComment(const FText& UserComment)
{
	// Find .xml file
	FString XmlFilename;
	if (!FindFirstReportFileWithExtension(XmlFilename, TEXT(".xml")))
	{
		return false;
	}

	FString XmlFilePath = ReportDirectory / XmlFilename;
	// FXmlFile's constructor loads the file to memory, closes the file and parses the data
	FXmlFile XmlFile(XmlFilePath);
	auto DynamicSignaturesNode = XmlFile.IsValid() ?
		XmlFile.GetRootNode()->FindChildNode(TEXT("DynamicSignatures")) :
		nullptr;

	if (!DynamicSignaturesNode)
	{
		return false;
	}

	DynamicSignaturesNode->AppendChildNode(TEXT("Parameter3"), UserComment.ToString());
	// Re-save over the top
	return XmlFile.Save(XmlFilePath);
}

TArray<FString> FLinuxErrorReport::GetFilesToUpload() const
{
	TArray<FString> FilesToUpload;

	const int NumReportFiles = ReportFilenames.Num();
	UE_LOG(CrashReportClientLog, Log, TEXT("NumReportFiles = %d"), NumReportFiles);
	for (int32 Index = 0; Index != NumReportFiles; ++Index)
	{
		const auto& Filename = ReportFilenames[Index];
		UE_LOG(CrashReportClientLog, Log, TEXT("ReportFile %d = '%s'"), Index, *Filename);
		FilesToUpload.Push(ReportDirectory / Filename);
	}
	return FilesToUpload;
}

FText FLinuxErrorReport::DiagnoseReport() const
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

	const auto& Exception = CrashDebugHelper->CrashInfo.Exception;
	if (Exception.CallStackString.Num() == 0)
	{
		return FText::FromString(Exception.ExceptionString);
	}

	FString Diagnostic = Exception.ExceptionString + "\n\n";

	for (const auto& Line: Exception.CallStackString)
	{
		Diagnostic += Line + "\n";
	}

	// There's a callstack, so write it out to save the server trying to do it
	CrashDebugHelper->CrashInfo.GenerateReport(ReportDirectory / GDiagnosticsFilename);

	return FText::FromString(Diagnostic);
}

FString FLinuxErrorReport::FindCrashedAppName() const
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

bool FLinuxErrorReport::LoadWindowsReportXmlFile(TArray<uint8>& OutBuffer) const
{
	// Find .xml file
	FString XmlFilename;
	if (!FindFirstReportFileWithExtension(XmlFilename, TEXT(".xml")))
	{
		return false;
	}

	return FFileHelper::LoadFileToArray(OutBuffer, *(ReportDirectory / XmlFilename));
}

FString FLinuxErrorReport::FindMostRecentErrorReport()
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

	PlatformFile.IterateDirectory(
		*(FString(FPlatformProcess::UserDir()) / TEXT("../AppData/Local/Microsoft/Linux/WER/ReportQueue")),
		ReportFinder);

	return ReportDirectory;
}

bool FLinuxErrorReport::FindFirstReportFileWithExtension(FString& OutFilename, const TCHAR* Extension) const
{
	for (const auto& Filename: ReportFilenames)
	{
		if (Filename.EndsWith(Extension))
		{
			OutFilename = Filename;
			return true;
		}
	}
	return false;
}

#undef LOCTEXT_NAMESPACE
