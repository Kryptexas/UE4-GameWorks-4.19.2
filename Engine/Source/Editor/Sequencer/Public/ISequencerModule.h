// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"
#include "ISequencer.h"

/**
 * A delegate which will create an auto-key handler
 */
DECLARE_DELEGATE_RetVal_OneParam( TSharedRef<class FMovieSceneTrackEditor>, FOnCreateTrackEditor, TSharedRef<ISequencer> );


/**
 * The public interface of SequencerModule
 */
class ISequencerModule : public IModuleInterface
{

public:

	/**
	 * Creates a new instance of a Sequencer, the editor for MovieScene assets
	 *
	 * @param	Mode					Mode that this editor should operate in
	 * @param	InitToolkitHost			When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit			The object to start editing
	 *
	 * @return	Interface to the new editor
	 */
	virtual TSharedPtr<ISequencer> CreateSequencer( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit ) = 0;

	/** 
	 * Registers a delegate that will create an editor for a track in each sequencer 
	 *
	 * @param InOnCreateTrackEditor	Delegate to register
	 */
	virtual void RegisterTrackEditor( FOnCreateTrackEditor InOnCreateTrackEditor ) = 0;

	/** 
	 * Unregisters a previously registered delegate for creating a track editor
	 *
	 * @param InOnCreateTrackEditor	Delegate to unregister
	 */
	virtual void UnRegisterTrackEditor( FOnCreateTrackEditor InOnCreateTrackEditor ) = 0;
};

