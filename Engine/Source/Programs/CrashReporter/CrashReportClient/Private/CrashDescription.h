// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/** Enumerates crash description versions. */
enum class ECrashDescVersions
{
	/** Introduces a new crash description format. */
	VER_1_NewCrashFormat,
};

/**
 *	Describes a unified crash, should be used by all platforms.
 *	Still lacks some of the properties, they will be added later.
 *	Must contain the same properties as ...\CrashReportServer\CrashReportCommon\CrashDescription.cs.
 *	Contains all usable information about the crash. 
 *	
 */

// @TODO yrx 2014-08-27 add samples data
struct FCrashDescription
{
	/** 
	 * Version of this crash description. 
	 */
	static const ECrashDescVersions CrashDescriptionVersion = ECrashDescVersions::VER_1_NewCrashFormat;

	/** An unique report name that this crash belongs to. */
	FString ReportName;

	/**
	 * The name of the game that crashed. (AppID)
	 * @GameName	varchar(64)
	 */
	FString GameName;

	/**
	 * The mode the game was in e.g. editor.
	 * @EngineMode	varchar(64)	
	 */
	FString EngineMode;

	/**
	 * The platform that crashed e.g. Win64.
	 * @PlatformName	varchar(64)	
	 */
	FString Platform;

	/** 
	 * Encoded engine version. (AppVersion)
	 * E.g. 4.3.0.0-2215663+UE4-Releases+4.3
	 * BuildVersion-BuiltFromCL-BranchName
	 * @EngineVersion	varchar(64)	
	 */
	FEngineVersion EngineVersion;

	/**
	 * The three component version of the app e.g. 4.4.1
	 * @BuildVersion	varchar(128)	
	 */
	FString BuildVersion;

	/**
	 * Built from changelist.
	 * @ChangeListVersion	varchar(64)	
	 */
	uint32 BuiltFromCL;

	/**
	 * The name of the branch this game was built out of.
	 * @Branch varchar(32) 
	 */
	FString BranchName;

	/**
	 * The command line of the application that crashed.
	 * @CommandLine varchar(512) 
	 */
	FString CommandLine;

	/**
	 * The base directory where the app was running.
	 * @BaseDir varchar(512) 
	 */
	FString BaseDir;

	/**
	 * The language ID the application that crashed.
	 * @LanguageExt varchar(64) 
	 */
	FString LanguageLCID;

	/** 
	 * The name of the user that caused this crash.
	 * @UserName varchar(64)  
	 */
	FString UserName;

	/**
	 * The unique ID used to identify the machine the crash occurred on.
	 * @ComputerName varchar(64) 
	 */
	FString MachineId;

	/** 
	 * The Epic account ID for the user who last used the Launcher.
	 * @EpicAccountId	varchar(64)	
	 */
	FString EpicAccountId;

	/**
	 * An array of FStrings representing the callstack of the crash.
	 * @RawCallStack	varchar(MAX)	
	 */
	TArray<FString> CallStack;

	/**
	 * An array of FStrings showing the source code around the crash.
	 * @SourceContext varchar(max) 
	 */
	TArray<FString> SourceContext;

	/**
	 * An array of FStrings representing the user description of the crash.
	 * @Description	varchar(512)	
	 */
	TArray<FString> UserDescription;

	/**
	 * The error message, can be assertion message, ensure message or message from the fatal error.
	 * @Summary	varchar(512)	
	 */
	TArray<FString> ErrorMessage;

	/**
	 * The UTC time the crash occurred.
	 * @TimeOfCrash	datetime	
	 */
	FDateTime TimeOfCrash;

	/**
	 * Whether this crash has a minidump file.
	 * @HasMiniDumpFile bit 
	 */
	bool bHasMiniDumpFile;

	/**
	 * Whether this crash has a log file.
	 * @HasLogFile bit 
	 */
	bool bHasLogFile;

	/**
	 * Whether this crash has a video file.
	 * @HasVideoFile bit 
	 */
	bool bHasVideoFile;

	/** Whether this crash contains complete data, otherwise only IDs are available. */
	bool bHasCompleteData;

	/**
	 * A default constructor.
	 */
	FCrashDescription();

	/** Initializes instance based on specified WER XML string. */
	explicit FCrashDescription( FString WERXMLFilepath );

	/** Initializes engine version from the separate components. */
	void InitializeEngineVersion();

	/** Initializes following properties: UserNAme, MachineID and EpicAccountID. */
	void InitializeIDs();

	/** Sends this crash for analytics. */
	void SendAnalytics();
};