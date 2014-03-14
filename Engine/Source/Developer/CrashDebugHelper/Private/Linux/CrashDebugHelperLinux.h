// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FCrashDebugHelperLinux : public ICrashDebugHelper
{
public:
	FCrashDebugHelperLinux();
	virtual ~FCrashDebugHelperLinux();

	/**
	 *	Parse the given crash dump, determining EngineVersion of the build that produced it - if possible. 
	 *
	 *	@param	InCrashDumpName		The crash dump file to process
	 *	@param	OutCrashDebugInfo	The crash dump info extracted from the file
	 *
	 *	@return	bool				true if successful, false if not
	 */
	virtual bool ParseCrashDump(const FString& InCrashDumpName, FCrashDebugInfo& OutCrashDebugInfo) OVERRIDE;

	virtual bool SyncAndDebugCrashDump(const FString& InCrashDumpName) OVERRIDE;

	/**
	 *	Parse the given crash dump, and generate a report. 
	 *
	 *	@param	InCrashDumpName		The crash dump file to process
	 *
	 *	@return	bool				true if successful, false if not
	 */
	virtual bool CreateMinidumpDiagnosticReport( const FString& InCrashDumpName ) OVERRIDE;
};

typedef FCrashDebugHelperLinux FCrashDebugHelper;
