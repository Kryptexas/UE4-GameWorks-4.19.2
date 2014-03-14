// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "ModuleInterface.h"

/**
 * Content browser module
 */
class FGameProjectGenerationModule : public IModuleInterface
{

public:

	/**
	 * Called right after the plugin DLL has been loaded and the plugin object has been created
	 */
	virtual void StartupModule();

	/**
	 * Called before the plugin is unloaded, right before the plugin object is destroyed.
	 */
	virtual void ShutdownModule();

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline FGameProjectGenerationModule& Get()
	{
		return FModuleManager::LoadModuleChecked< FGameProjectGenerationModule >( "GameProjectGeneration" );
	}

	/** Creates the game project dialog */
	virtual TSharedRef<class SWidget> CreateGameProjectDialog(bool bAllowProjectOpening, bool bAllowProjectCreate);

	/** Creates a new class dialog for creating classes based on the passed-in class. */
	virtual TSharedRef<class SWidget> CreateNewClassDialog(class UClass* InClass);

	/** Opens a dialog to adds code files to the current project. */
	virtual void OpenAddCodeToProjectDialog();

	/** Delegate for when the AddCodeToProject dialog is opened */
	DECLARE_EVENT(FGameProjectGenerationModule, FAddCodeToProjectDialogOpenedEvent);
	FAddCodeToProjectDialogOpenedEvent& OnAddCodeToProjectDialogOpened() { return AddCodeToProjectDialogOpenedEvent; }

	/** Prompts the user to update his project file, if necessary. */
	virtual void CheckForOutOfDateGameProjectFile();

	/** Updates the currently loaded project. Returns true if the project was updated successfully or if no update was needed */
	virtual bool UpdateGameProject();

	/** Updates the current code project */
	virtual bool UpdateCodeProject(FText& OutFailReason);

	/** Gets the current projects source file count */
	virtual int32 GetProjectCodeFileCount();

private:
	FAddCodeToProjectDialogOpenedEvent AddCodeToProjectDialogOpenedEvent;

};
