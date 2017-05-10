// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AcquiredResources.h"
#include "CoreMinimal.h"
#include "Layout/Visibility.h"


class AActor;
class FExtender;
class FMenuBuilder;
class FSequencer;
class FUICommandList;
class ILevelViewport;
class ISequencer;
class SViewportTransportControls;
class ULevel;


struct FLevelEditorSequencerIntegrationOptions
{
	FLevelEditorSequencerIntegrationOptions()
		: bRequiresLevelEvents(true)
		, bRequiresActorEvents(false)
		, bCanRecord(false)
	{}

	bool bRequiresLevelEvents : 1;
	bool bRequiresActorEvents : 1;
	bool bCanRecord : 1;
};


class SEQUENCER_API FLevelEditorSequencerIntegration
{
public:

	static FLevelEditorSequencerIntegration& Get();

	void Initialize();

	void AddSequencer(TSharedRef<ISequencer> InSequencer, const FLevelEditorSequencerIntegrationOptions& Options);

	void OnSequencerReceivedFocus(TSharedRef<ISequencer> InSequencer);

	void RemoveSequencer(TSharedRef<ISequencer> InSequencer);

	void SetViewportTransportControlsVisibility(bool bVisible);

	bool GetViewportTransportControlsVisibility() const;

private:

	/** Called before the world is going to be saved. The sequencer puts everything back to its initial state. */
	void OnPreSaveWorld(uint32 SaveFlags, UWorld* World);

	/** Called after the world has been saved. The sequencer updates to the animated state. */
	void OnPostSaveWorld(uint32 SaveFlags, UWorld* World, bool bSuccess);

	/** Called after a level has been added */
	void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);

	/** Called after a level has been removed */
	void OnLevelRemoved(ULevel* InLevel, UWorld* InWorld);

	/** Called after a new level has been created. The sequencer editor mode needs to be enabled. */
	void OnNewCurrentLevel();

	/** Called after a map has been opened. The sequencer editor mode needs to be enabled. */
	void OnMapOpened(const FString& Filename, bool bLoadAsTemplate);

	/** Called when new actors are dropped in the viewport. */
	void OnNewActorsDropped(const TArray<UObject*>& DroppedObjects, const TArray<AActor*>& DroppedActors);

	/** Called when viewport tab content changes. */
	void OnTabContentChanged();

	/** Called before a PIE session begins. */
	void OnPreBeginPIE(bool bIsSimulating);

	/** Called after a PIE session ends. */
	void OnEndPIE(bool bIsSimulating);

	/** Called after PIE session ends and maps have been cleaned up */
	void OnEndPlayMap();

	/** Handles the actor selection changing externally .*/
	void OnActorSelectionChanged( UObject* );

	/** Called via UEditorEngine::GetActorRecordingStateEvent to check to see whether we need to record actor state */
	void GetActorRecordingState( bool& bIsRecording ) const;

	/** Called when sequencer has been evaluated */
	void OnSequencerEvaluated();

	void OnPropertyEditorOpened();

	TSharedRef<FExtender> GetLevelViewportExtender(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> InActors);

	TSharedRef<FExtender> OnExtendLevelEditorViewMenu(const TSharedRef<FUICommandList> CommandList);

	void RecordSelectedActors();

	EVisibility GetTransportControlVisibility(TSharedPtr<ILevelViewport> LevelViewport) const;
	
	/** Create a menu entry we can use to toggle the transport controls */
	void CreateTransportToggleMenuEntry(FMenuBuilder& MenuBuilder);

private:

	void ActivateSequencerEditorMode();
	void AddLevelViewportMenuExtender();
	void ActivateDetailKeyframeHandler();
	void AttachTransportControlsToViewports();
	void DetachTransportControlsFromViewports();
	void BindLevelEditorCommands();

private:

	void IterateAllSequencers(TFunctionRef<void(FSequencer&, const FLevelEditorSequencerIntegrationOptions& Options)>) const;

	FLevelEditorSequencerIntegration();

private:

	friend SViewportTransportControls;
	
	struct FSequencerAndOptions
	{
		TWeakPtr<FSequencer> Sequencer;
		FLevelEditorSequencerIntegrationOptions Options;
		FAcquiredResources AcquiredResources;
	};
	TArray<FSequencerAndOptions> BoundSequencers;

	/** A map of all the transport controls to viewports that this sequencer has made */
	struct FTransportControl
	{
		TWeakPtr<ILevelViewport> Viewport;
		TSharedPtr<SViewportTransportControls> Widget;
	};
	TArray<FTransportControl> TransportControls;

	FAcquiredResources AcquiredResources;

	TSharedPtr<class FDetailKeyframeHandlerWrapper> KeyFrameHandler;
};
