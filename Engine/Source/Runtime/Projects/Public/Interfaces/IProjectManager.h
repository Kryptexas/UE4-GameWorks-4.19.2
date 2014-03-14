// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


#include "IPluginManagerShared.h"

/**
 * Simple data structure that is filled when querying information about projects
 */
class FProjectStatus
{
public:

	/** The name of this project */
	FString Name;

	/** The UI category of this project */
	FString Category;

	/** True if this project is a sample provided by epic */
	bool bSignedSampleProject;

	/** True if this project is up to date with the current engine version */
	bool bUpToDate;

	FProjectStatus()
		: bSignedSampleProject(false)
		, bUpToDate(false)
	{}
};

/**
 * ProjectAndPluginManager manages available code and content extensions (both loaded and not loaded.)
 */
class IProjectManager
{

public:

	/**
	 * Static: Access singleton instance
	 *
	 * @return	Reference to the singleton object
	 */
	static PROJECTS_API IProjectManager& Get();

	/**
	 * Loads the specified project file.
	 *
	 * @param ProjectFile - The project file to load.
	 *
	 * @return true if the project was loaded successfully, false otherwise.
	 *
	 * @see GetLoadedGameProjectFile
	 */
	virtual bool LoadProjectFile( const FString& ProjectFile ) = 0;

	/**
	 * Loads all modules for the currently loaded project in the specified loading phase
	 *
	 * @param	LoadingPhase	Which loading phase we're loading modules from.  Only modules that are configured to be
	 *							loaded at the specified loading phase will be loaded during this call.
	 */
	virtual bool LoadModulesForProject( const ELoadingPhase::Type LoadingPhase ) = 0;

	/**
	 * Gets the name of the text file that contains the most recently loaded filename.
	 *
	 * This is NOT the name of the recently loaded .uproject file.
	 *
	 * @return File name.
	 */
	virtual const FString& GetAutoLoadProjectFileName() = 0;

	/**
	 * Gets the extension for game project files.
	 *
	 * @return File extension.
	 */
	virtual const FString& NonStaticGetProjectFileExtension() = 0;

	/**
	 * Generates a new project file and saves it to disk at the specified location
	 * 
	 * @param NewProjectFilename	The filename of the file to be written out
	 * @param StartupModuleNames	The list of modules to be loaded at startup
	 * @param OutFailReason			When returning false, this provides a display reason why the file could not be created.
	 *
	 * @return true when the file was successfully written
	 */
	virtual bool GenerateNewProjectFile(const FString& NewProjectFilename, const TArray<FString>& StartupModuleNames, FText& OutFailReason) = 0;

	/**
	 * Generates a new project file based on a source file and saves it to disk at the specified location
	 * 
	 * @param SourceProjectFilename	The filename of the file to duplicate
	 * @param NewProjectFilename	The filename of the file to be written out
	 * @param OutFailReason			When returning false, this provides a display reason why the file could not be created.
	 *
	 * @return true when the file was successfully written
	 */
	virtual bool DuplicateProjectFile(const FString& SourceProjectFilename, const FString& NewProjectFilename, FText& OutFailReason) = 0;

	/**
	 * Updates the loaded game project file to the current version and saves it to disk
	 * 
	 * @param StartupModuleNames	When specified, replaces the existing module names with these new ones			
	 * @param OutFailReason			When returning false, this provides a display reason why the file could not be created.
	 *
	 * @return true if the file was successfully updated and saved to disk
	 */
	virtual bool UpdateLoadedProjectFileToCurrent(const TArray<FString>* StartupModuleNames, FText& OutFailReason) = 0;

	/**
	 * Sets the project's EpicSampleNameHash (based on its filename) and category, then saves the file to disk.
	 * This marks the project as a sample and fixes its filename so that
	 * it isn't mistaken for a sample if a copy of the file is made.
	 *
	 * @param FilePath				The filepath where the sample project is stored.
	 * @param Category				Category to place the sample in
	 * @param OutFailReason			When returning false, this provides a display reason why the file could not be created.
	 *
	 * @return true if the file was successfully updated and saved to disk
	 */
	virtual bool SignSampleProject(const FString& FilePath, const FString& Category, FText& OutFailReason) = 0;

	/**
	 * Gets status about the specified project
	 *
	 * @param FilePath				The filepath where the project is stored.
	 * @param OutProjectStatus		The status for the project.
	 *
	 * @return	 true if the file was successfully open and read
	 */
	virtual bool QueryStatusForProject(const FString& FilePath, FProjectStatus& OutProjectStatus) const = 0;

	/** Helper functions to reduce the syntax complexity of commonly used functions */
	static const FString& GetProjectFileExtension() { return Get().NonStaticGetProjectFileExtension(); }
};