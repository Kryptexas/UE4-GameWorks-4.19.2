// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ISequencerAssetEditor.h"

class FSequencerAssetEditor : public ISequencerAssetEditor
{ 
public:
	/** Constructor */
	FSequencerAssetEditor();

	/** Destructor */
	virtual ~FSequencerAssetEditor();

	/**
	 * Edits the specified object
	 *
	 * @param	Mode							Asset editing mode for this editor (standalone or world-centric)
	 * @param	InitToolkitHost					When Mode is WorldCentric, this is the level editor instance to spawn this editor within
	 * @param	ObjectToEdit					The object to edit
	 * @param	TrackEditorDelegates			Delegates to call to create auto-key handlers for this sequencer
	 * @param	bEditWithinLevelEditor			Whether or not sequencer should be edited within the level editor
	 */
	void InitSequencerAssetEditor( const EToolkitMode::Type Mode, const TSharedPtr< class IToolkitHost >& InitToolkitHost, UMovieScene* InRootMovieScene, const TArray<FOnCreateTrackEditor>& TrackEditorDelegates, bool bEditWithinLevelEditor );

	TSharedRef<ISequencer> GetSequencerInterface() const OVERRIDE { return Sequencer.ToSharedRef(); }

	/** IToolkit interface */
	virtual FName GetToolkitFName() const OVERRIDE;
	virtual FText GetBaseToolkitName() const OVERRIDE;
	virtual FString GetWorldCentricTabPrefix() const OVERRIDE;
	virtual FLinearColor GetWorldCentricTabColorScale() const OVERRIDE;

	virtual void RegisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;
	virtual void UnregisterTabSpawners(const TSharedRef<class FTabManager>& TabManager) OVERRIDE;

	void UpdateViewports(AActor* ActorToViewThrough) const;

private:
	EVisibility GetTransportControlVisibility(TWeakPtr<ILevelViewport> LevelViewport) const;
	TSharedRef<SDockTab> SpawnTab_SequencerMain(const FSpawnTabArgs& Args);	
protected:

	/** Generates and attaches transport control widgets to the main level editor viewports */
	void AttachTransportControlsToViewports();
	/** Purges all transport control widgets from the main level editor viewports */
	void DetachTransportControlsFromViewports();
	
private:
	TSharedPtr<FSequencer> Sequencer;

	/** A map of all the transport controls to viewports that this sequencer has made */
	TMap< TWeakPtr<class ILevelViewport>, TSharedPtr<class SWidget> > TransportControls;
	/**	The tab ids for all the tabs used */
	static const FName SequencerMainTabId;

};
