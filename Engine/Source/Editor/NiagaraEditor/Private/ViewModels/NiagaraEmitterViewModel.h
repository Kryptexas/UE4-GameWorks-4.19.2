// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraParameterEditMode.h"
#include "NiagaraScriptViewModel.h"

class UNiagaraEmitterProperties;
class FNiagaraScriptViewModel;
class FNiagaraScriptGraphViewModel;
struct FNiagaraSimulation;
struct FNiagaraVariable;

/** The view model for the UNiagaraEmitterProperties objects */
class FNiagaraEmitterViewModel : public TSharedFromThis<FNiagaraEmitterViewModel>, public FNotifyHook, public TNiagaraViewModelManager<UNiagaraEmitterProperties, FNiagaraEmitterViewModel>
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnEmitterChanged);
	DECLARE_MULTICAST_DELEGATE(FOnPropertyChanged);
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnParameterValueChanged, FGuid);
	DECLARE_MULTICAST_DELEGATE(FOnScriptCompiled);

public:
	/** Creates a new emitter editor view model with the supplied emitter handle and simulation. */
	FNiagaraEmitterViewModel(UNiagaraEmitterProperties* InEmitter, FNiagaraSimulation* InSimulation, ENiagaraParameterEditMode ParameterEditMode);
	virtual ~FNiagaraEmitterViewModel();

	/** Reuse this view model with new parameters.*/
	bool Set(UNiagaraEmitterProperties* InEmitter, FNiagaraSimulation* InSimulation, ENiagaraParameterEditMode ParameterEditMode);

	/** Sets this view model to a different emitter. */
	void SetEmitter(UNiagaraEmitterProperties* InEmitter);

	/** Sets the current simulation for the emitter. */
	void SetSimulation(FNiagaraSimulation* InSimulation);

	/** Gets the start time for the emitter. */
	float GetStartTime() const;

	/** Sets the start time for the emitter. */
	void SetStartTime(float InStartTime);

	/** Gets the end time for the emitter. */
	float GetEndTime() const;

	/** Sets the end time for the emitter. */
	void SetEndTime(float InEndTime);

	/** Gets the number of loops for the emitter.  0 for infinite. */
	int32 GetNumLoops() const;

	/** Gets the emitter represented by this view model. */
	UNiagaraEmitterProperties* GetEmitter();

	/** Gets text representing stats for the emitter. */
	//~ TODO: Instead of a single string here, we should probably have separate controls with tooltips etc.
	FText GetStatsText() const;

	/** Gets a view model for the spawn script. */
	TSharedRef<FNiagaraScriptViewModel> GetSpawnScriptViewModel();

	/** Geta a view model for the update Script. */
	TSharedRef<FNiagaraScriptViewModel> GetUpdateScriptViewModel();

	/** Gets the width of the name column for parameter editors. */
	float GetParameterNameColumnWidth() const;

	/** Gets the width of the second column for parameter editors. */
	float GetParameterContentColumnWidth() const;

	/** Called when the name column width changes for parameter editors. */
	void ParameterNameColumnWidthChanged(float Width);

	/** Called when the second column width changes for parameter editors. */
	void ParameterContentColumnWidthChanged(float Width);

	/** Compiles the spawn and update scripts. */
	void CompileScripts();

	/* Get the latest status of this view-model's script compilation.*/
	ENiagaraScriptCompileStatus GetLatestCompileStatus();

	/** Gets a multicast delegate which is called when the emitter for this view model changes to a different emitter. */
	FOnEmitterChanged& OnEmitterChanged();

	/** Gets a delegate which is called when a property on the emitter changes. */
	FOnPropertyChanged& OnPropertyChanged();

	/** Gets a delegate which is called one of the emitter parameter values changes. */
	FOnParameterValueChanged& OnParameterValueChanged();

	/** Gets a delegate which is called one of the emitter parameter values changes. */
	FOnScriptCompiled& OnScriptCompiled();

	//~ FNotifyHook interface
	virtual void NotifyPostChange(const FPropertyChangedEvent& PropertyChangedEvent, UProperty* PropertyThatChanged) override;

	/** Handle external assets changing, which might cause us to need to refresh.*/
	bool DoesAssetSavedInduceRefresh(const FString& PackagePath, UObject* Object, bool RecurseIntoDependencies);

	bool GetDirty() const;
	void SetDirty(bool bDirty);

private:
	/** Called when the script view model node selection changes. */
	void ScriptViewModelSelectedNodesChanged(TSharedRef<FNiagaraScriptGraphViewModel> ChangedScriptEditorViewModel);

	/** Called when any of the output parameters on the spawn script changes. */
	void SpawnScriptOutputParametersChanged();

	/** Called when a parameter value on one of the scripts changes. */
	void ScriptParameterValueChanged(FGuid ParamterId);

	/** Helper method to union two distinct compiler statuses.*/
	static ENiagaraScriptCompileStatus UnionCompileStatus(const ENiagaraScriptCompileStatus& StatusA, const ENiagaraScriptCompileStatus& StatusB);
private:

	/** The text format stats display .*/
	static const FText StatsFormat;

	/** The emitter object being displayed by the control .*/
	UNiagaraEmitterProperties* Emitter;

	/** The runtime simulation for the emitter being displayed by the control */
	FNiagaraSimulation* Simulation;

	/** The view model for the spawn script. */
	TSharedRef<FNiagaraScriptViewModel> SpawnScriptViewModel;

	/** The view model for the update script. */
	TSharedRef<FNiagaraScriptViewModel> UpdateScriptViewModel;

	/** The width of the name column for parameter editors. */
	float ParameterNameColumnWidth;

	/** The width of the second column for parameter editors. */
	float ParameterContentColumnWidth;

	/** A flag to prevent reentrancy when updating selection sets. */
	bool bUpdatingSelectionInternally;

	/** A multicast delegate which is called whenever the emitter for this view model is changed to a different emitter. */
	FOnEmitterChanged OnEmitterChangedDelegate;

	/** A multicast delegate which is called whenever a property on the emitter changes. */
	FOnPropertyChanged OnPropertyChangedDelegate;

	/** A multicast delegate which is called whenever parameter value on the emitter changes. */
	FOnParameterValueChanged OnParameterValueChangedDelegate;

	FOnScriptCompiled OnScriptCompiledDelegate;

	ENiagaraScriptCompileStatus LastEventScriptStatus;

	bool bEmitterDirty;

	TNiagaraViewModelManager<UNiagaraEmitterProperties, FNiagaraEmitterViewModel>::Handle RegisteredHandle;
};