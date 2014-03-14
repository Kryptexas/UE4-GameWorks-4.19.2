// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "CrashReportClientApp.h"

#include "GenericErrorReport.h"
#include "XmlFile.h"
#include "CrashDebugHelperModule.h"
#include "CrashReportUtil.h"

// ----------------------------------------------------------------
// Helpers

namespace
{
	/** Enum specifying a particular part of a crash report text file */
	namespace EReportSection
	{
		enum Type
		{
			CallStack,
			SourceContext,
			Other
		};
	}
}


// ----------------------------------------------------------------
// FGenericErrorReport

FGenericErrorReport::FGenericErrorReport(const FString& Directory)
	: ReportDirectory(Directory)
{
	auto FilenamesVisitor = MakeDirectoryVisitor([this](const TCHAR* FilenameOrDirectory, bool bIsDirectory) {
		if (!bIsDirectory)
		{
			ReportFilenames.Push(FPaths::GetCleanFilename(FilenameOrDirectory));
		}
		return true;
	});
	FPlatformFileManager::Get().GetPlatformFile().IterateDirectory(*ReportDirectory, FilenamesVisitor);

}

bool FGenericErrorReport::SetUserComment(const FText& UserComment)
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

TArray<FString> FGenericErrorReport::GetFilesToUpload() const
{
	TArray<FString> FilesToUpload;

	for (const auto& Filename: ReportFilenames)
	{
		FilesToUpload.Push(ReportDirectory / Filename);
	}
	return FilesToUpload;
}

bool FGenericErrorReport::LoadWindowsReportXmlFile(TArray<uint8>& OutBuffer) const
{
	// Find .xml file
	FString XmlFilename;
	if (!FindFirstReportFileWithExtension(XmlFilename, TEXT(".xml")))
	{
		return false;
	}

	return FFileHelper::LoadFileToArray(OutBuffer, *(ReportDirectory / XmlFilename));
}

bool FGenericErrorReport::TryReadDiagnosticsFile(FText& OutReportDescription)
{
	FString FileContent;
	if (!FFileHelper::LoadFileToString(FileContent, *(ReportDirectory / GDiagnosticsFilename)))
	{
		// No diagnostics file
		return false;
	}

	FString Exception;
	TArray<FString> Callstack;

	static const TCHAR CallStackStartKey[] = TEXT("<CALLSTACK START>");
	static const TCHAR CallStackEndKey[] = TEXT("<CALLSTACK END>");
	static const TCHAR SourceContextStartKey[] = TEXT("<SOURCE START>");
	static const TCHAR SourceContextEndKey[] = TEXT("<SOURCE END>");
	static const TCHAR ExceptionLineStart[] = TEXT("Exception was ");
	auto ReportSection = EReportSection::Other;
	const TCHAR* Stream = *FileContent;
	FString Line;
	while (FParse::Line(&Stream, Line, true /* don't treat |s as new-lines! */))
	{
		switch (ReportSection)
		{
		default:
			check(false);

		case EReportSection::CallStack:
			if (Line.StartsWith(CallStackEndKey))
			{
				ReportSection = EReportSection::Other;
			}
			else
			{
				Callstack.Push(Line);
			}
			break;
		case EReportSection::SourceContext:
			if (Line.StartsWith(SourceContextEndKey))
			{
				ReportSection = EReportSection::Other;
			}
			else
			{
				// Not doing anything with this at the moment
			}
			break;
		case EReportSection::Other:
			if (Line.StartsWith(CallStackStartKey))
			{
				ReportSection = EReportSection::CallStack;
			}
			else if (Line.StartsWith(SourceContextStartKey))
			{
				ReportSection = EReportSection::SourceContext;
			}
			else if (Line.StartsWith(ExceptionLineStart))
			{
				// Not subtracting 1 from the array count so it gobbles the initial quote
				Exception = Line.RightChop(ARRAY_COUNT(ExceptionLineStart)).LeftChop(1);
			}
			break;
		}
	}
	OutReportDescription = FormatReportDescription(Exception, Callstack);
	return true;
}

bool FGenericErrorReport::FindFirstReportFileWithExtension(FString& OutFilename, const TCHAR* Extension) const
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

