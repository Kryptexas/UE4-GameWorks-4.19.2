// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once


/**
 * Info about a single project
 */
class FProjectInfo : public FProjectOrPluginInfo
{
public:

	//////////////////////////////////////////////////////////////////////////////////////////////////////
	// Everything Below this line is serialized. If you add or remove properties, you must update
	// FProject::PerformAdditionalSerialization and FProject::PerformAdditionalDeserialization
	//////////////////////////////////////////////////////////////////////////////////////////////////////

	/** A hash that is used to determine if the project was forked from a sample */
	uint32 EpicSampleNameHash;

	/** Array of platforms that this project is targeting */
	TArray<FName> TargetPlatforms;

	/** Project info constructor */
	FProjectInfo()
		: FProjectOrPluginInfo()
		, EpicSampleNameHash(0)
	{
	}
};


/**
 * Instance of a project in memory
 */
class FProject : public FProjectOrPlugin
{
public:
	/** Constructor */
	FProject();
	FProject( const FProjectInfo& InitProjectInfo );

	/**
	 * Gets whether the project is a sample project signed with the given filename.
	 *
	 * @param FilePath	The filepath from which the project was loaded.
	 * @return true if the project has been signed with a filename matching the FilePath param.
	 */
	bool IsSignedSampleProject(const FString& FilePath) const;

	/**
	 * Sets the project's EpicSampleNameHash based on a filename, and sets its category.
	 * This marks the project as a sample and fixes its filename so that
	 * it isn't mistaken for a sample if a copy of the file is made.
	 *
	 * @param FilePath	The filepath where the sample project is stored.
	 */
	void SignSampleProject(const FString& FilePath, const FString &Category);

	/**
	 * Update the list of supported target platforms based upon the parameters provided
	 * 
	 * @param	InPlatformName		Name of the platform to target (eg, WindowsNoEditor)
	 * @param	bIsSupported		true if the platform should be supported by this project, false if it should not
	 */
	void UpdateSupportedTargetPlatforms(const FName& InPlatformName, const bool bIsSupported);

	/** Clear the list of supported target platforms */
	void ClearSupportedTargetPlatforms();

	const FProjectInfo& GetProjectInfo() const
	{
		return ProjectInfo;
	}

protected:
	virtual FProjectOrPluginInfo& GetProjectOrPluginInfo() OVERRIDE { return ProjectInfo; }
	virtual const FProjectOrPluginInfo& GetProjectOrPluginInfo() const OVERRIDE { return ProjectInfo; }

	virtual bool PerformAdditionalDeserialization(const TSharedRef< FJsonObject >& FileObject) OVERRIDE;
	virtual void PerformAdditionalSerialization(const TSharedRef< TJsonWriter<> >& Writer) const OVERRIDE;

private:
	/** The descriptor of this project that we loaded from disk */
	FProjectInfo ProjectInfo;
};


/**
 * ProjectAndPluginManager manages available code and content extensions (both loaded and not loaded.)
 */
class FProjectManager : public IProjectManager
{
public:
	FProjectManager();

	/** IProjectManager interface */
	virtual bool LoadProjectFile( const FString& ProjectFile ) OVERRIDE;
	virtual bool LoadModulesForProject( const ELoadingPhase::Type LoadingPhase ) OVERRIDE;
	virtual bool AreProjectModulesUpToDate( ) OVERRIDE;
	virtual const FString& GetAutoLoadProjectFileName() OVERRIDE;
	virtual const FString& NonStaticGetProjectFileExtension() OVERRIDE;
	virtual bool GenerateNewProjectFile(const FString& NewProjectFilename, const TArray<FString>& ModuleNames, const FString& EngineIdentifier, FText& OutFailReason) OVERRIDE;
	virtual bool DuplicateProjectFile(const FString& SourceProjectFilename, const FString& NewProjectFilename, const FString& EngineIdentifier, FText& OutFailReason) OVERRIDE;
	virtual bool UpdateLoadedProjectFileToCurrent(const TArray<FString>* StartupModuleNames, const FString& EngineIdentifier, FText& OutFailReason) OVERRIDE;
	virtual bool SignSampleProject(const FString& FilePath, const FString& Category, FText& OutFailReason) OVERRIDE;
	virtual bool QueryStatusForProject(const FString& FilePath, FProjectStatus& OutProjectStatus) const OVERRIDE;
	virtual bool QueryStatusForCurrentProject(FProjectStatus& OutProjectStatus) const OVERRIDE;
	virtual void UpdateSupportedTargetPlatformsForProject(const FString& FilePath, const FName& InPlatformName, const bool bIsSupported) OVERRIDE;
	virtual void UpdateSupportedTargetPlatformsForCurrentProject(const FName& InPlatformName, const bool bIsSupported) OVERRIDE;
	virtual void ClearSupportedTargetPlatformsForProject(const FString& FilePath) OVERRIDE;
	virtual void ClearSupportedTargetPlatformsForCurrentProject() OVERRIDE;
	virtual FOnTargetPlatformsForCurrentProjectChangedEvent& OnTargetPlatformsForCurrentProjectChanged() OVERRIDE { return OnTargetPlatformsForCurrentProjectChangedEvent; }

private:
	static void QueryStatusForProjectImpl(const FProject& Project, const FString& FilePath, FProjectStatus& OutProjectStatus);

	/** The project that is currently loaded in the editor */
	TSharedPtr< FProject > CurrentlyLoadedProject;

	/** Delegate called when the target platforms for the current project are changed */
	FOnTargetPlatformsForCurrentProjectChangedEvent OnTargetPlatformsForCurrentProjectChangedEvent;
};


