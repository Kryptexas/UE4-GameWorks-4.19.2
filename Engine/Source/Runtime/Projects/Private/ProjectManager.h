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

	bool VerifyProjectCanBeLoaded( bool bPromptIfSavedWithNewerVersionOfEngine, FText& OutFailureReason );

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
	virtual const FString& GetAutoLoadProjectFileName() OVERRIDE;
	virtual const FString& NonStaticGetProjectFileExtension() OVERRIDE;
	virtual bool GenerateNewProjectFile(const FString& NewProjectFilename, const TArray<FString>& ModuleNames, FText& OutFailReason) OVERRIDE;
	virtual bool DuplicateProjectFile(const FString& SourceProjectFilename, const FString& NewProjectFilename, FText& OutFailReason) OVERRIDE;
	virtual bool UpdateLoadedProjectFileToCurrent(const TArray<FString>* StartupModuleNames, FText& OutFailReason) OVERRIDE;
	virtual bool SignSampleProject(const FString& FilePath, const FString& Category, FText& OutFailReason) OVERRIDE;
	virtual bool QueryStatusForProject(const FString& FilePath, FProjectStatus& OutProjectStatus) const OVERRIDE;

private:
	/** The project that is currently loaded in the editor */
	TSharedPtr< FProject > CurrentlyLoadedProject;
};


