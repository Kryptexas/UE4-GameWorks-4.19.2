// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../GenericErrorReport.h"

/**
 * Helper that works with Mac Error Reports
 */
class FMacErrorReport : public FGenericErrorReport
{
public:
	/**
	 * Default constructor: creates a report with no files
	 */
	FMacErrorReport()
	{
	}

	/**
	 * Discover all files in the crash report directory
	 * @param Directory Full path to directory containing the report
	 */
	explicit FMacErrorReport(const FString& Directory);

	/**
	 * Do nothing - shouldn't be called on Mac
	 * @return Dummy text
	 */
	FText DiagnoseReport() const	
	{
		return FText::FromString("No local diagnosis on Mac");
	}


	/**
	 * Get the name of the crashed app from the report
	 */
	FString FindCrashedAppName() const;

	/**
	 * Look for the most recent Mac Error Report
	 * @return Full path to the most recent report, or an empty string if none found
	 */
	static FString FindMostRecentErrorReport();
};
