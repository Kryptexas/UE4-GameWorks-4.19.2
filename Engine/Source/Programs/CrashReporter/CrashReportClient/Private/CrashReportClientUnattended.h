// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CrashUpload.h"

/**
 * Implementation of the crash report client used for unattended uploads
 */
class FCrashReportClientUnattended
{
public:
	/**
	 * Set up uploader object
	 * @param ReportDirectory Full path to report to upload
	 */
	explicit FCrashReportClientUnattended(const FString& ReportDirectory);

private:
	/**
	 * Update received every second
	 * @param DeltaTime Time since last update, unused
	 * @return Whether the updates should continue
	 */
	bool Tick(float DeltaTime);

	/**
	 * Begin calling Tick once a second
	 */
	void StartTicker();

	/** Complete path of report directory - must be first (see constructor) */
	FString ReportDirectory;

	/** Object that uploads report files to the server */
	FCrashUpload Uploader;
};
