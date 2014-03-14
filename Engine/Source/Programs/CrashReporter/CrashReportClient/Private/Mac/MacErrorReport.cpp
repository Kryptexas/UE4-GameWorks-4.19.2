// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

#include "MacErrorReport.h"

FMacErrorReport::FMacErrorReport(const FString& Directory)
	: FGenericErrorReport(Directory)
{
}

FString FMacErrorReport::FindCrashedAppName() const
{
	TArray<uint8> Data;
	if(FFileHelper::LoadFileToArray(Data, *(ReportDirectory / TEXT("Report.wer"))))
	{
		CFStringRef CFString = CFStringCreateWithBytes(NULL, Data.GetData(), Data.Num(), kCFStringEncodingUTF16LE, true);
		FString FileData((NSString*)CFString);
		CFRelease(CFString);
		
		static const TCHAR AppPathLineStart[] = TEXT("AppPath=");
		static const int AppPathIdLength = ARRAY_COUNT(AppPathLineStart) - 1;
		int32 AppPathStart = FileData.Find(AppPathLineStart);
		if(AppPathStart >= 0)
		{
			FString PathData = FileData.Mid(AppPathStart + AppPathIdLength);
			int32 LineEnd = -1;
			if(PathData.FindChar( TCHAR('\r'), LineEnd ))
			{
				PathData = PathData.Left(LineEnd);
			}
			if(PathData.FindChar( TCHAR('\n'), LineEnd ))
			{
				PathData = PathData.Left(LineEnd);
			}
			return FPaths::GetCleanFilename(PathData);
		}
	}
	else
	{
		UE_LOG(LogStreaming, Error,	TEXT("Failed to read file '%s' error."),*(ReportDirectory / TEXT("Report.wer")));
	}
	return "";
}

FString FMacErrorReport::FindMostRecentErrorReport()
{
	auto& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	auto DirectoryModifiedTime = FDateTime::MinValue();
	FString RecentReportDirectory;
	auto ReportFinder = MakeDirectoryVisitor([&](const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
		if (bIsDirectory)
		{
			auto TimeStamp = PlatformFile.GetTimeStamp(FilenameOrDirectory);
			if (TimeStamp > DirectoryModifiedTime)
			{
				RecentReportDirectory = FilenameOrDirectory;
				DirectoryModifiedTime = TimeStamp;
			}
		}
		return true;
	});

	FString AllReportsDirectory = FPaths::GameAgnosticSavedDir() / TEXT("Crashes");
	
	PlatformFile.IterateDirectory(
		*AllReportsDirectory,
		ReportFinder);

	return RecentReportDirectory;
}
