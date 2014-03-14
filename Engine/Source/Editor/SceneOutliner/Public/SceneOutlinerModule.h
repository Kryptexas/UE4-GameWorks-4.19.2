// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ModuleInterface.h"

#include "ISceneOutliner.h"
#include "SceneOutlinerInitializationOptions.h"

/**
 * Scene Outliner module
 */
class FSceneOutlinerModule : public IModuleInterface
{

public:

	/**
	 * Called right after the module DLL has been loaded and the module object has been created
	 */
	virtual void StartupModule();

	/**
	 * Called before the module is unloaded, right before the module object is destroyed.
	 */
	virtual void ShutdownModule();

	/**
	 * Creates a scene outliner widget
	 *
	 * @param	InitOptions						Programmer-driven configuration for this widget instance
	 * @param	MakeContentMenuWidgetDelegate	Optional delegate to execute to build a context menu when right clicking on actors
	 * @param	OnActorPickedDelegate			Optional callback when an actor is selected in 'actor picking' mode
	 *
	 * @return	New scene outliner widget
	 */
	virtual TSharedRef< class ISceneOutliner > CreateSceneOutliner(
		const FSceneOutlinerInitializationOptions& InitOptions,
		const FOnContextMenuOpening& MakeContextMenuWidgetDelegate,
		const FOnActorPicked& OnActorPickedDelegate ) const;
};


