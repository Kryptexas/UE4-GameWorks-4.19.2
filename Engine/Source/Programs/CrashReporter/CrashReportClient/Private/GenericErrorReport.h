// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Helper that works with Windows Error Reports
 */
class FGenericErrorReport
{
public:
	/**
	 * Default constructor: creates a report with no files
	 */
	FGenericErrorReport()
	{
	}

	/**
	 * Discover all files in the crash report directory
	 * @param Directory Full path to directory containing the report
	 */
	explicit FGenericErrorReport(const FString& Directory);

	/**
	 * Write the provided comment into the error report
	 * @param UserComment Information provided by the user to add to the report
	 * @return Whether the comment was successfully written to the report
	 */
	bool SetUserComment(const FText& UserComment);

	/**
	 * Provide full paths to all the report files
	 * @return List of paths
	 */
	TArray<FString> GetFilesToUpload() const;

	/**
	 * Load the WER XML file for this report
	 * @note This is Windows specific and so shouldn't really be part of the public interface, but currently the server
	 * is Windows-specific in its checking of reports, so this is needed.
	 * @param OutBuffer Buffer to load the file into
	 * @return Whether finding and loading the file succeeded
	 */
	bool LoadWindowsReportXmlFile(TArray<uint8>& OutBuffer) const;

	/**
	 * @param Description (exception and callstack) to fill in if succesful
	 * @return Whether the file was found and succesfully read
	 */
	bool TryReadDiagnosticsFile(FText& OutReportDescription);

protected:
	/**
	 * Look throught the list of report files to find one with the given extension
	 * @return Whether a file with the extension was found
	 */
	bool FindFirstReportFileWithExtension(FString& OutFilename, const TCHAR* Extension) const;

	/** Full path to the directory the report files are in */
	FString ReportDirectory;

	/** List of leaf filenames of all the files in the report folder */
	TArray<FString> ReportFilenames;
};
