// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"


/**
 * The public interface to this module.  In most cases, this interface is only public to sibling modules 
 * within this plugin.
 */
class IIntroTutorials : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline IIntroTutorials& Get()
	{
		return FModuleManager::LoadModuleChecked< IIntroTutorials >( "IntroTutorials" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "IntroTutorials" );
	}

	/**
	 * Adds a tutorial association for a type of asset (will be summoned the first time an asset editor is opened for the specified asset class)
	 * @param AssetClass		Asset type to register a tutorial for
	 * @param TutorialDocPath	The path to the tutorial doc (must be rooted in the Shared directory, e.g., "Shared/Tutorials/InPersonaAnimEditorTutorial")
	 * @param TutorialHasBeenSeenSettingName	The setting name to store whether this editor has been seen or not yet (stored in GEditorGameAgnosticIni in the [IntroTutorials] section)
	 * @param SurveyGUIDString	Text representation of a GUID, used to uniquely identify this tutorial if required for a survey.
	 */
	virtual void RegisterTutorialForAssetEditor(UClass* AssetClass, const FString& TutorialDocPath, const FString& TutorialHasBeenSeenSettingName, const FString& SurveyGUIDString)=0;

	/**
	 * Removes the tutorial association for the specified class
	 * @param AssetClass		Asset type to unregister the tutorial from
	 */
	virtual void UnregisterTutorialForAssetEditor(UClass* AssetClass)=0;
};

