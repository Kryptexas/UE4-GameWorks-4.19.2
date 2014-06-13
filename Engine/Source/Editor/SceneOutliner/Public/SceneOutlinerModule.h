// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "ISceneOutliner.h"
#include "SceneOutlinerInitializationOptions.h"


/**
 * Implements the Scene Outliner module.
 */
class FSceneOutlinerModule
	: public IModuleInterface
{
public:

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
		const FOnActorPicked& OnActorPickedDelegate ) const;

	/**
	 * Creates a scene outliner widget
	 *
	 * @param	InitOptions						Programmer-driven configuration for this widget instance
	 * @param	MakeContentMenuWidgetDelegate	Optional delegate to execute to build a context menu when right clicking on actors
	 * @param	OnItemPickedDelegate			Optional callback when an item is selected in 'picking' mode
	 *
	 * @return	New scene outliner widget
	 */
	virtual TSharedRef< class ISceneOutliner > CreateSceneOutliner(
		const FSceneOutlinerInitializationOptions& InitOptions,
		const FOnSceneOutlinerItemPicked& OnItemPickedDelegate ) const;

public:

	// IModuleInterface interface

	virtual void StartupModule() { }
	virtual void ShutdownModule() { }
};
