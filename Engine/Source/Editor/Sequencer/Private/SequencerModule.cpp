// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "ModuleManager.h"
#include "Sequencer.h"
#include "Toolkits/ToolkitManager.h"
#include "SequencerCommands.h"
#include "SequencerAssetEditor.h"

/**
 * SequencerModule implementation (private)
 */
class FSequencerModule : public ISequencerModule
{

	/** ISequencerModule interface */
	virtual TSharedPtr<ISequencer> CreateSequencer( UMovieScene* RootMovieScene ) OVERRIDE
	{
		TSharedRef< FSequencer > Sequencer = MakeShareable(new FSequencer);
		Sequencer->InitSequencer( RootMovieScene, nullptr, TrackEditorDelegates, false );
		return Sequencer;
	}
	
	virtual TSharedPtr<ISequencer> CreateSequencerAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UMovieScene* InRootMovieScene, bool bEditWithinLevelEditor ) OVERRIDE
	{
		TSharedRef< FSequencerAssetEditor > SequencerAssetEditor = MakeShareable(new FSequencerAssetEditor);
		SequencerAssetEditor->InitSequencerAssetEditor( Mode, InitToolkitHost, InRootMovieScene, TrackEditorDelegates, bEditWithinLevelEditor );
		return SequencerAssetEditor->GetSequencerInterface();
	}

	virtual void RegisterTrackEditor( FOnCreateTrackEditor InOnCreateTrackEditor ) OVERRIDE
	{
		TrackEditorDelegates.AddUnique( InOnCreateTrackEditor );
	}

	virtual void UnRegisterTrackEditor( FOnCreateTrackEditor InOnCreateTrackEditor ) OVERRIDE
	{
		TrackEditorDelegates.Remove( InOnCreateTrackEditor );
	}

	virtual void StartupModule() OVERRIDE
	{
		FSequencerCommands::Register();
	}

	virtual void ShutdownModule() OVERRIDE
	{
		FSequencerCommands::Unregister();
	}
private:
	/** List of auto-key handler delegates sequencers will execute when they are created */
	TArray< FOnCreateTrackEditor > TrackEditorDelegates;
};



IMPLEMENT_MODULE( FSequencerModule, Sequencer );
