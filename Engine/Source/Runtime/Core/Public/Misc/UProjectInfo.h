// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//@NOTE: This is more or less exactly mirrored in UnrealBuildTool.UProjectInfo
// The UBT one contains more functionality as it needs to match build Targets to UProjects.

/** Helper struct for tracking UProject information */
struct FUProjectInfo
{
	/** Name of the project file */
	FString ProjectName;
	/** Folder the project file is located in */
	FString ProjectFileFolder;
};

class FUProjectInfoHelper
{
public:
	/** Fill in the project info */
	static CORE_API void FillProjectInfo();

	/** 
	 *	Get the project path for the given game name.
	 *
	 *	@param	InGameName		The game of interest
	 *
	 *	@return	FString			The path to the UProject file, empty if not found
	 */
	static CORE_API FString GetProjectForGame(const TCHAR* InGameName);

protected:
	/** True if project info has already been filled... */
	static bool bProjectInfoFilled;
	/** Map of relative or complete project file names to the project info */
	static TMap<FString, FUProjectInfo> ProjectInfoDictionary;
	/** Map of short project file names to the relative or complete project file name */
	static TMap<FString, FString> ShortProjectNameDictionary;
};
