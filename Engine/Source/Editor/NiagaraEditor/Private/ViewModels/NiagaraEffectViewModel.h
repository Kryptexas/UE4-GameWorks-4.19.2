// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IMovieScenePlayer.h"

#include "NiagaraParameterEditMode.h"
#include "EditorUndoClient.h"
#include "GCObject.h"
#include "NiagaraCurveOwner.h"
#include "TNiagaraViewModelManager.h"
#include "ISequencer.h"

class UNiagaraEffect;
class UNiagaraComponent;
class UNiagaraSequence;
class UMovieSceneNiagaraEmitterTrack;
struct FNiagaraVariable;
class FNiagaraEmitterHandleViewModel;
class FNiagaraEffectScriptViewModel;
class FNiagaraEffectInstance;
class ISequencer;
struct FAssetData;
class INiagaraParameterCollectionViewModel;

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
class FNiagaraEffectViewModel : public TSharedFromThis<FNiagaraEffectViewModel>, public FGCObject, public FEditorUndoClient, public TNiagaraViewModelManager<UNiagaraEffect, FNiagaraEffectViewModel>
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnEmitterHandleViewModelsChanged);

	DECLARE_MULTICAST_DELEGATE(FOnCurveOwnerChanged);

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

	/** Gets the curve owner for the effect represented by this view model, for use with the curve editor widget. */
	FNiagaraCurveOwner& GetCurveOwner();

	/** Adds a new emitter to the effect from an emitter asset data. */
	void AddEmitterFromAssetData(const FAssetData& AssetData);

	/** Duplicates the selected emitter in this effect. */
	void DuplicateEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDuplicate);

	/** Deletes the selected emitter from the effect. */
	void DeleteEmitter(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleToDelete);

	/** Gets a multicast delegate which is called any time the array of emitter handle view models changes. */
	FOnEmitterHandleViewModelsChanged& OnEmitterHandleViewModelsChanged();

	/** Gets a delegate which is called any time the data in the curve owner is changed internally by this view model. */
	FOnCurveOwnerChanged& OnCurveOwnerChanged();

	//~ FGCObject interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	//~ FEditorUndoClient interface
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override { PostUndo(bSuccess); }

	void ResynchronizeAllHandles();

private:
	/** Reinitializes all effect instances, and rebuilds emitter handle view models and tracks. */
	void RefreshAll();

	/** Sets up the preview component and effect instance. */
	void SetupPreviewComponentAndInstance();

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

	/** Reinitializes the effect instance to initial conditions - rebuilds all data sets and bindings (slow) */
	void ReInitializeEffect();

	/** Resets and rebuilds the data in the curve owner. */
	void ResetCurveData();

	/** Called whenever a property on the emitter handle changes. */
	void EmitterHandlePropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Called whenever a property on the emitter changes. */
	void EmitterPropertyChanged(TSharedRef<FNiagaraEmitterHandleViewModel> EmitterHandleViewModel);

	/** Called whenever a parameter value on an emitter changes. */
	void EmitterParameterValueChanged(FGuid ParamterId);

	/** Called whenever the parameter selection in one of parameter editors changes. */
	void ParameterSelectionChanged();

	/** Called whenever a parameter value on the effect changes. */
	void EffectParameterValueChanged(FGuid ParamterId);

	/** Called whenever the collection of parameters changes.*/
	void EffectParameterCollectionChanged();

	/** Called when the bindings for the effect script parameters changes. */
	void EffectParameterBindingsChanged();

	/** Called when a script is compiled */
	void ScriptCompiled();

	/** Handles when a curve in the curve owner is changed by the curve editor. */
	void CurveChanged(TSharedPtr<INiagaraParameterCollectionViewModel> CollectionViewModel, FGuid ParameterId);

	/** Called whenever the data in the sequence is changed. */
	void SequencerDataChanged(EMovieSceneDataChangeType DataChangeType);

	/** Called whenever the global time in the sequencer is changed. */
	void SequencerTimeChanged();

	/** Called whenever the effect instance is initialized. */
	void EffectInstanceInitialized();

	/** Called whenever the effect's bindings to emitter parameters have changed and we need to synchronize the UI editability state.*/
	void SynchronizeEffectDrivenParametersUI();

private:
	/** The effect being viewed and edited by this view model. */
	UNiagaraEffect& Effect;

	/** The edit mode for parameters in this view model. */
	ENiagaraParameterEditMode ParameterEditMode;

	/** The component used for previewing the effect in a viewport. */
	UNiagaraComponent* PreviewComponent;

	/** The effect instance which is simulating the effect. */
	TSharedPtr<FNiagaraEffectInstance> EffectInstance;

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

	/** A multicase delegate which is called when the contents of the curve owner is changed internally by this view model. */
	FOnCurveOwnerChanged OnCurveOwnerChangedDelegate;

	/** A flag for preventing reentrancy when syncrhonizing sequencer data. */
	bool bUpdatingFromSequencerDataChange;

	/** A curve owner implementation for curves in a niagara effect. */
	FNiagaraCurveOwner CurveOwner;

	TNiagaraViewModelManager<UNiagaraEffect, FNiagaraEffectViewModel>::Handle RegisteredHandle;
};