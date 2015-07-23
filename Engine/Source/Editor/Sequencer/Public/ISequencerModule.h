// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "ISequencer.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "Toolkits/IToolkit.h"


class ISequencerObjectBindingManager;
class IToolkitHost;
class UActorAnimation;


namespace SequencerMenuExtensionPoints
{
	static const FName AddTrackMenu_PropertiesSection("AddTrackMenu_PropertiesSection");
}


/** A delegate which will create an auto-key handler. */
DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<class FMovieSceneTrackEditor>, FOnCreateTrackEditor, TSharedRef<ISequencer>);

/** A delegate that is executed when adding menu content. */
DECLARE_DELEGATE_RetVal_OneParam(TSharedRef<SWidget>, FOnGetAddMenuContent, TSharedRef<ISequencer>);


/**
 * Sequencer view parameters.
 */
struct FSequencerViewParams
{
	/** The initial time range of the view. */
	TRange<float> InitalViewRange;

	/** Initial Scrub Position. */
	float InitialScrubPosition;

	FOnGetAddMenuContent OnGetAddMenuContent;

	/** Unique name for the sequencer. */
	FString UniqueName;

	FSequencerViewParams(FString InName = FString())
		: InitalViewRange(0.0f, 5.0f)
		, InitialScrubPosition(0.0f)
		, UniqueName(MoveTemp(InName))
	{ }
};


/**
 * Sequencer initialization parameters.
 */
struct FSequencerInitParams
{
	/** The animation being edited. */
	UMovieSceneAnimation* Animation;

	/** The asset editor created for this (if any) */
	TSharedPtr<IToolkitHost> ToolkitHost;

	/** View parameters */
	FSequencerViewParams ViewParams;

	/** Whether or not sequencer should be edited within the level editor */
	bool bEditWithinLevelEditor;
};


/**
 * Interface for the Sequencer module.
 */
class ISequencerModule
	: public IModuleInterface
{
public:

	/**
	 * Create a new instance of a standalone sequencer that can be added to other UIs.
	 *
	 * @param InitParams Initialization parameters.
	 * @return The new sequencer object.
	 */
	virtual TSharedRef<ISequencer> CreateSequencer(const FSequencerInitParams& InitParams) = 0;

	/** 
	 * Registers a delegate that will create an editor for a track in each sequencer.
	 *
	 * @param InOnCreateTrackEditor	Delegate to register.
	 * @return A handle to the newly-added delegate.
	 */
	virtual FDelegateHandle RegisterTrackEditor_Handle(FOnCreateTrackEditor InOnCreateTrackEditor) = 0;

	/** 
	 * Unregisters a previously registered delegate for creating a track editor
	 *
	 * @param InHandle	Handle to the delegate to unregister
	 */
	virtual void UnRegisterTrackEditor_Handle(FDelegateHandle InHandle) = 0;

	/**
	 * Get the extensibility manager for menus.
	 *
	 * @return Menu extensibility manager.
	 */
	virtual TSharedPtr<FExtensibilityManager> GetMenuExtensibilityManager() const = 0;

	/**
	 * Get the extensibility manager for toolbars.
	 *
	 * @return Toolbar extensibility manager.
	 */
	virtual TSharedPtr<FExtensibilityManager> GetToolBarExtensibilityManager() const = 0;
};
