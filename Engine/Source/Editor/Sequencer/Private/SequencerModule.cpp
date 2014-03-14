// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "ModuleManager.h"
#include "Sequencer.h"
#include "Toolkits/ToolkitManager.h"
#include "SequencerCommands.h"

/**
 * SequencerModule implementation (private)
 */
class FSequencerModule : public ISequencerModule
{

	/** ISequencerModule interface */
	virtual TSharedPtr<ISequencer> CreateSequencer( const EToolkitMode::Type Mode, const TSharedPtr< IToolkitHost >& InitToolkitHost, UObject* ObjectToEdit ) OVERRIDE
	{
		TSharedRef< FSequencer > SequencerInUse = MakeShareable(new FSequencer);
		SequencerInUse->InitSequencer( Mode, InitToolkitHost, ObjectToEdit, TrackEditorDelegates );
		return SequencerInUse;
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
