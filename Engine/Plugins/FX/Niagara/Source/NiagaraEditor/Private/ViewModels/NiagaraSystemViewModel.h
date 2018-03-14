// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"

#include "EditorUndoClient.h"
#include "GCObject.h"
#include "NiagaraCurveOwner.h"
#include "TNiagaraViewModelManager.h"
#include "ISequencer.h"

#include "TickableEditorObject.h"
#include "ISequencerModule.h"

class UNiagaraSystem;
class UNiagaraComponent;
class UNiagaraSequence;
class UMovieSceneNiagaraEmitterTrack;
struct FNiagaraVariable;
struct FNiagaraEmitterHandle;
class FNiagaraEmitterHandleViewModel;
class FNiagaraSystemScriptViewModel;
class FNiagaraSystemInstance;
class ISequencer;
struct FAssetData;
class UNiagaraSystemEditorData;
struct FRichCurve;

/** Defines different editing modes for this system view model. */
enum class ENiagaraSystemViewModelEditMode
{
	/** A system asset is being edited.  This assumes that emitters should be inheriting from a base version and that emitter editing will be restricted. */
	SystemAsset,
	/** An emitter asset is being edited.  In this mode the system scripts will not be editable and all emitter editing options are available. */
	EmitterAsset,
};

/** Defines options for the niagara System view model */
struct FNiagaraSystemViewModelOptions
{
	FNiagaraSystemViewModelOptions();

	/** Whether or not the user can edit emitters from the timeline. */
	bool bCanModifyEmittersFromTimeline;

	/** A delegate which is used to generate the content for the add menu in sequencer. */
	FOnGetAddMenuContent OnGetSequencerAddMenuContent;

	/** Whether or not we use the system's execution state to drive when we reset the timeline*/
	bool bUseSystemExecStateForTimelineReset;

	/** Whether or not the system represented by this view model can be automatically compiled.  True by default. */
	bool bCanAutoCompile;

	/** Whether or not the system represented by this view model can be simulated. True by default. */
	bool bCanSimulate;

	/** Gets the current editing mode for this system. */
	ENiagaraSystemViewModelEditMode EditMode;
};

/** A view model for viewing and editing a UNiagaraSystem. */
class FNiagaraSystemViewModel 
	: public TSharedFromThis<FNiagaraSystemViewModel>
	, public FGCObject
	, public FEditorUndoClient
	, public FTickableEditorObject
	, public TNiagaraViewModelManager<UNiagaraSystem, FNiagaraSystemViewModel>
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnEmitterHandleViewModelsChanged);

	DECLARE_MULTICAST_DELEGATE(FOnCurveOwnerChanged);

	DECLARE_MULTICAST_DELEGATE(FOnSelectedEmitterHandlesChanged);

	DECLARE_MULTICAST_DELEGATE(FOnPostSequencerTimeChange)

	DECLARE_MULTICAST_DELEGATE(FOnSystemCompiled);

public:
	/** Creates a new view model with the supplied System and System instance. */
	FNiagaraSystemViewModel(UNiagaraSystem& InSystem, FNiagaraSystemViewModelOptions InOptions);

	~FNiagaraSystemViewModel();

	/** Gets an array of the view models for the emitter handles owned by this System. */
	const TArray<TSharedRef<FNiagaraEmitterHandleViewModel>>& GetEmitterHandleViewModels();

	/** Gets the view model for the System script. */
	TSharedRef<FNiagaraSystemScriptViewModel> GetSystemScriptViewModel();

	/** Gets a niagara component for previewing the simulated System. */
	UNiagaraComponent* GetPreviewComponent();

	/** Gets the sequencer for this System for displaying the timeline. */
	TSharedPtr<ISequencer> GetSequencer();

	/** Gets the curve owner for the System represented by this view model, for use with the curve editor widget. */
	FNiagaraCurveOwner& GetCurveOwner();

	/** Get access to the underlying system*/
	UNiagaraSystem& GetSystem() { return System; }

	/** Gets whether or not emitters can be added from the timeline. */
	bool GetCanModifyEmittersFromTimeline() const;

	/** Gets the current editing mode for this system view model. */
	ENiagaraSystemViewModelEditMode GetEditMode() const;

	/** Adds a new emitter to the System from an emitter asset data. */
	void AddEmitterFromAssetData(const FAssetData& AssetData);

	/** Adds a new emitter to the System. */
	void AddEmitter(UNiagaraEmitter& Emitter);

	/** Duplicates the selected emitter in this System. */
	void DuplicateEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDuplicate);

	/** Deletes the selected emitter from the System. */
	void DeleteEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDelete);

	/** Deletes the emitters with the supplied ids from the system */
	void DeleteEmitters(TSet<FGuid> EmitterHandleIdsToDelete);

	/** Gets a multicast delegate which is called any time the array of emitter handle view models changes. */
	FOnEmitterHandleViewModelsChanged& OnEmitterHandleViewModelsChanged();

	/** Gets a delegate which is called any time the data in the curve owner is changed internally by this view model. */
	FOnCurveOwnerChanged& OnCurveOwnerChanged();

	/** Gets a multicast delegate which is called whenever the selected emitter handles changes. */
	FOnSelectedEmitterHandlesChanged& OnSelectedEmitterHandlesChanged();
	
	/** Gets a multicast delegate which is called whenever we've received and handled a sequencer time update.*/
	FOnPostSequencerTimeChange& OnPostSequencerTimeChanged();

	/** Gets a multicast delegate which is called whenever the system has been compiled. */
	FOnSystemCompiled& OnSystemCompiled();

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	//~ FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

	// ~ FTickableEditorObject
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	/** Resets the System instance to initial conditions. */
	void ResetSystem();

	/** Reinitializes the System instance to initial conditions - rebuilds all data sets and resets data interfaces. */
	void ReInitializeSystemInstances();

	/** Compiles the spawn and update scripts. */
	void CompileSystem(bool bForce);

	/* Get the latest status of this view-model's script compilation.*/
	ENiagaraScriptCompileStatus GetLatestCompileStatus();

	/** Gets the ids for the currently selected emitter handles. */
	const TArray<FGuid>& GetSelectedEmitterHandleIds();

	/** Sets the currently selected emitter handles by id. */
	void SetSelectedEmitterHandlesById(TArray<FGuid> InSelectedEmitterHandleIds);

	/** Sets the currently selected emitter handle by id. */
	void SetSelectedEmitterHandleById(FGuid InSelectedEmitterHandleId);

	/** Gets the currently selected emitter handles. */
	void GetSelectedEmitterHandles(TArray<TSharedRef<FNiagaraEmitterHandleViewModel>>& OutSelectedEmitterHanldles);

	/** Gets editor specific data which can be stored per system.  If this data hasn't been created the default version will be returned. */
	const UNiagaraSystemEditorData& GetEditorData() const;

	/** Gets editor specific data which is stored per system.  If this data hasn't been created then it will be created. */
	UNiagaraSystemEditorData& GetOrCreateEditorData();
	
	/** Act as if the system has been fully destroyed although references might persist.*/
	void Cleanup();

	/** Reinitializes all System instances, and rebuilds emitter handle view models and tracks. */
	void RefreshAll();

	/** Called to notify the system view model that one of the data objects in the system was modified. */
	void NotifyDataObjectChanged(UObject* ChangedObject);

	/** Updates all selected emitter's fixed bounds with their current dynamic bounds. */
	void UpdateEmitterFixedBounds();

	/** Whether it is possible to isolate an emitter. */
	bool CanExecuteToggleIsolation();

	/** Isolates the selected emitters and sets all the others to not be isolated. */
	void IsolateSelectedEmitters();

	/** Toggle the isolate state of the selected emitters. */
	void IsolateEmitterToggle();

	/** Whether all the selected emitters are isolated. */
	bool IsEmitterIsolationActive();

private:

	/** Sets up the preview component and System instance. */
	void SetupPreviewComponentAndInstance();

	/** Rebuilds the emitter handle view models. */
	void RefreshEmitterHandleViewModels();

	/** Rebuilds the sequencer tracks. */
	void RefreshSequencerTracks();

	/** Gets the sequencer emitter track for the supplied emitter handle view model. */
	UMovieSceneNiagaraEmitterTrack* GetTrackForHandleViewModel(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Refreshes sequencer track data from it's emitter handle. */
	void RefreshSequencerTrack(UMovieSceneNiagaraEmitterTrack* EmitterTrack);

	/** Sets up the sequencer for this emitter. */
	void SetupSequencer();

	/** Gets the content for the add menu in sequencer. */
	void GetSequencerAddMenuContent(FMenuBuilder& MenuBuilder, TSharedRef<ISequencer> Sequencer);

	/** Kills all system instances using the system being displayed by this view model. */
	void KillSystemInstances();

	/** Resets and rebuilds the data in the curve owner. */
	void ResetCurveData();

	/** Updates the compiled versions of data interfaces when their sources change. */
	void UpdateCompiledDataInterfaces(UNiagaraDataInterface* ChangedDataInterface);

	/** Called whenever a property on the emitter handle changes. */
	void EmitterHandlePropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Called whenever the name on an emitter handle changes. */
	void EmitterHandleNameChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Called whenever a property on the emitter changes. */
	void EmitterPropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Called when a script is compiled */
	void ScriptCompiled();

	/** Handles when a curve in the curve owner is changed by the curve editor. */
	void CurveChanged(FRichCurve* ChangedCurve, UObject* CurveOwnerObject);

	/** Called whenever the data in the sequence is changed. */
	void SequencerDataChanged(EMovieSceneDataChangeType DataChangeType);

	/** Called whenever the global time in the sequencer is changed. */
	void SequencerTimeChanged();

	/** Called whenever the track selection in sequencer changes. */
	void SequencerTrackSelectionChanged(TArray<UMovieSceneTrack*> SelectedTracks);

	/** Called whenever the section selection in sequencer changes. */
	void SequencerSectionSelectionChanged(TArray<UMovieSceneSection*> SelectedSections);

	/** Updates the current emitter handle selection base on the sequencer selection. */
	void UpdateEmitterHandleSelectionFromSequencer();

	/** Updates the sequencer selection based on the current emitter handle selection. */
	void UpdateSequencerFromEmitterHandleSelection();

	/** Called when the system instance on the preview component changes. */
	void PreviewComponentSystemInstanceChanged();

	/** Called whenever the System instance is initialized. */
	void SystemInstanceInitialized();

	/** Called whenever the System instance is reset.*/
	void SystemInstanceReset();

	/** Duplicates a set of emitters and refreshes everything.*/
	void DuplicateEmitters(TSet<FGuid> EmitterHandleIdsToDuplicate);

private:
	/** The System being viewed and edited by this view model. */
	UNiagaraSystem& System;

	/** The component used for previewing the System in a viewport. */
	UNiagaraComponent* PreviewComponent;

	/** The system instance currently simulating this system if available. */
	FNiagaraSystemInstance* SystemInstance;

	/** The view models for the emitter handles owned by the System. */
	TArray<TSharedRef<FNiagaraEmitterHandleViewModel>> EmitterHandleViewModels;

	/** The view model for the System script. */
	TSharedRef<FNiagaraSystemScriptViewModel> SystemScriptViewModel;

	/** A niagara sequence for displaying this System in the sequencer timeline. */
	UNiagaraSequence *NiagaraSequence;

	/** The sequencer instance viewing and editing the niagara sequence. */
	TSharedPtr<ISequencer> Sequencer;

	/** Flag which indicates we are setting the sequencer time directly in an internal operation. */
	bool bSettingSequencerTimeDirectly;

	/** The previous play status for sequencer timeline. */
	EMovieScenePlayerStatus::Type PreviousSequencerStatus;

	/** The previous time for the sequencer timeline. */
	float PreviousSequencerTime;

	/** Whether or not the user can edit emitters from the timeline. */
	bool bCanModifyEmittersFromTimeline;

	/** Whether or not we use the system's execution state to drive when we reset the timeline*/
	bool bUseSystemExecStateForTimelineReset;

	/** Whether or not the system represented by this view model can be automatically compiled. */
	bool bCanAutoCompile;

	/** Whether or not the system requires a compilation*/
	bool bForceAutoCompileOnce;

	/** Whether or not the system represented by this view model can be simulated. */
	bool bCanSimulate;

	/** The current editing mode for this view model. */
	ENiagaraSystemViewModelEditMode EditMode;

	/** A delegate which is used to generate the content for the add menu in sequencer. */
	FOnGetAddMenuContent OnGetSequencerAddMenuContent;

	/** A multicast delegate which is called any time the array of emitter handle view models changes. */
	FOnEmitterHandleViewModelsChanged OnEmitterHandleViewModelsChangedDelegate;

	/** A multicase delegate which is called when the contents of the curve owner is changed internally by this view model. */
	FOnCurveOwnerChanged OnCurveOwnerChangedDelegate;

	/** A multicast delegate which is called whenever the selected emitter changes. */
	FOnSelectedEmitterHandlesChanged OnSelectedEmitterHandlesChangedDelegate;

	/** A multicast delegate which is called whenever we've received and handled a sequencer time update.*/
	FOnPostSequencerTimeChange OnPostSequencerTimeChangeDelegate;

	/** A multicast delegate which is called whenever the system has been compiled. */
	FOnSystemCompiled OnSystemCompiledDelegate;

	/** A flag for preventing reentrancy when syncrhonizing sequencer data. */
	bool bUpdatingFromSequencerDataChange;

	/** A flag for preventing reentrancy when synchronizing system selection with sequencer selection */
	bool bUpdatingSystemSelectionFromSequencer;

	/** A flag for preventing reentrancy when synchronizing sequencer selection with system selection */
	bool bUpdatingSequencerSelectionFromSystem;

	/** A curve owner implementation for curves in a niagara System. */
	FNiagaraCurveOwner CurveOwner;

	/** The ids for the currently selected emitter handles. */
	TArray<FGuid> SelectedEmitterHandleIds;

	TNiagaraViewModelManager<UNiagaraSystem, FNiagaraSystemViewModel>::Handle RegisteredHandle;
};