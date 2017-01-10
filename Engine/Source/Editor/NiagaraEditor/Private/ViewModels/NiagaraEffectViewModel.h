// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"

#include "NiagaraParameterEditMode.h"
#include "EditorUndoClient.h"
#include "GCObject.h"

class UNiagaraEffect;
class UNiagaraComponent;
class UNiagaraSequence;
class UMovieSceneNiagaraEmitterTrack;
struct FNiagaraVariable;
class FNiagaraEmitterHandleViewModel;
class FNiagaraEffectScriptViewModel;
class FNiagaraEffectInstance;
class ISequencer;
class FAssetData;

/** Defines options for the niagara effect view model */
struct FNiagaraEffectViewModelOptions
{
	/** The edit mode for parameters in the view model. */
	ENiagaraParameterEditMode ParameterEditMode;

	/** Whether or not the user can remove emitters from the timeline. */
	bool bCanRemoveEmittersFromTimeline;

	/** Whether or not the user can rename emitters from the timeline. */
	bool bCanRenameEmittersFromTimeline;
};

/** A view model for viewing and editing a UNiagaraEffect. */
class FNiagaraEffectViewModel : public TSharedFromThis<FNiagaraEffectViewModel>, public FGCObject, public FEditorUndoClient
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnEmitterHandleViewModelsChanged);

public:
	/** Creates a new view model with the supplied effect and effect instance. */
	FNiagaraEffectViewModel(UNiagaraEffect& InEffect, FNiagaraEffectViewModelOptions InOptions);

	~FNiagaraEffectViewModel();

	/** Gets an array of the view models for the emitter handles owned by this effect. */
	const TArray<TSharedRef<FNiagaraEmitterHandleViewModel>>& GetEmitterHandleViewModels();

	/** Gets the view model for the effect script. */
	TSharedRef<FNiagaraEffectScriptViewModel> GetEffectScriptViewModel();

	/** Gets a niagara component for previewing the simulated effect. */
	UNiagaraComponent* GetPreviewComponent();

	/** Gets the sequencer for this effect for displaying the timeline. */
	TSharedPtr<ISequencer> GetSequencer();

	/** Adds a new emitter to the effect from an emitter asset data. */
	void AddEmitterFromAssetData(const FAssetData& AssetData);

	/** Duplicates the selected emitter in this effect. */
	void DuplicateEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDuplicate);

	/** Deletes the selected emitter from the effect. */
	void DeleteEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDelete);

	/** Gets a multicast delegate which is called any time the array of emitter handle view models changes. */
	FOnEmitterHandleViewModelsChanged& OnEmitterHandleViewModelsChanged();

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	//~ FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

private:
	/** Sets up the preview component and effect instance. */
	void SetupPreviewComponentAndInstance();

	/** Reinitializes all effect instances, and rebuilds emitter handle view models and tracks. */
	void RefreshAll();

	/** Reinitializes all active effect instances using this effect. */
	void ReinitEffectInstances();

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

	/** Resets the effect instance to initial conditions. */
	void ResetEffect();

	/** Called whenever a property on the emitter handle changes. */
	void EmitterHandlePropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Called whenever a property on the emitter changes. */
	void EmitterPropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Called whenever a parameter value on an emitter changes. */
	void EmitterParameterValueChanged(const FNiagaraVariable* ChangedVariable);

	/** Called whenever a parameter value on the effect changes. */
	void EffectParameterValueChanged(const FNiagaraVariable* ChangedVariable);

	/** Called when the bindings for the effect script parameters changes. */
	void EffectParameterBindingsChanged();

	/** Called whenever the data in the sequence is changed. */
	void SequencerDataChanged();

	/** Called whenever the global time in the sequencer is changed. */
	void SequencerTimeChanged();

	/** Called whenever the effect instance is initialized. */
	void EffectInstanceInitialized();

private:
	/** The effect being viewed and edited by this view model. */
	UNiagaraEffect& Effect;

	/** The edit mode for parameters in this view model. */
	ENiagaraParameterEditMode ParameterEditMode;

	/** The component used for previewing the effect in a viewport. */
	UNiagaraComponent* PreviewComponent;

	/** The effect instance which is simulating the effect. */
	TSharedPtr<FNiagaraEffectInstance> EffectInstance;

	/** The effect instance which is simulating the effect being viewed and edited by this view model. */
	//FNiagaraEffectInstance& EffectInstance;

	/** The view models for the emitter handles owned by the effect. */
	TArray<TSharedRef<FNiagaraEmitterHandleViewModel>> EmitterHandleViewModels;

	/** The view model for the effect script. */
	TSharedRef<FNiagaraEffectScriptViewModel> EffectScriptViewModel;

	/** A niagara sequence for displaying this effect in the sequencer timeline. */
	UNiagaraSequence *NiagaraSequence;

	/** The sequencer instance viewing and editing the niagara sequence. */
	TSharedPtr<ISequencer> Sequencer;

	/** The previous play status for sequencer */
	EMovieScenePlayerStatus::Type PreviousStatus;

	/** Whether or not the user can remove emitters from the timeline. */
	bool bCanRemoveEmittersFromTimeline;

	/** Whether or not the user can rename emitters from the timeline. */
	bool bCanRenameEmittersFromTimeline;

	/** A multicast delegate which is called any time the array of emitter handle view models changes. */
	FOnEmitterHandleViewModelsChanged OnEmitterHandleViewModelsChangedDelegate;

	/** A flag for preventing reentrancy when syncrhonizing sequencer data. */
	bool bUpdatingFromSequencerDataChange;
};