// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SharedPointer.h"
#include "Delegate.h"
#include "NiagaraParameterEditMode.h"
#include "SlateEnums.h"
#include "SlateTypes.h"

class UNiagaraEffect;
struct FNiagaraEmitterHandle;
struct FNiagaraSimulation;
class FNiagaraEmitterViewModel;

/** The view model for the FNiagaraEmitterEditorWidget. */
class FNiagaraEmitterHandleViewModel : public TSharedFromThis<FNiagaraEmitterHandleViewModel>
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnPropertyChanged);
public:
	/** Creates a new emitter editor view model with the supplied emitter handle and simulation. */
	FNiagaraEmitterHandleViewModel(FNiagaraEmitterHandle* InEmitterHandle, FNiagaraSimulation* InSimulation, UNiagaraEffect& InOwningEffect, ENiagaraParameterEditMode InParameterEditMode);

	/** Reuses a the emitter editor view model with the supplied emitter handle and simulation.*/
	bool Set(FNiagaraEmitterHandle* InEmitterHandle, FNiagaraSimulation* InSimulation, UNiagaraEffect& InOwningEffect, ENiagaraParameterEditMode InParameterEditMode);

	/** Sets the emitter handle.*/
	void SetEmitterHandle(FNiagaraEmitterHandle* InEmitterHandle);

	/** Sets the simulation for the emitter this handle references. */
	void SetSimulation(FNiagaraSimulation* InSimulation);

	/** Gets the id of the emitter handle. */
	FGuid GetId() const;

	/** Gets the name of the emitter handle. */
	FName GetName() const;

	/** Sets the name of the emitter handle. */
	void SetName(FName InName);

	/** Gets the text representation of the emitter handle name. */
	FText GetNameText() const;

	/** Called when the contents of the name text control is committed. */
	void OnNameTextComitted(const FText& InText, ETextCommit::Type CommitInfo);

	/** Gets whether or not this emitter handle is enabled. */
	bool GetIsEnabled() const;

	/** Sets whether or not this emitter handle is enabled. */
	void SetIsEnabled(bool bInIsEnabled);

	/** Gets the check state for the is enabled check box. */
	ECheckBoxState GetIsEnabledCheckState() const;

	/** Called when the check state of the enabled check box changes. */
	void OnIsEnabledCheckStateChanged(ECheckBoxState InCheckState);

	/** Gets the emitter handled being view and edited by this view model. */
	FNiagaraEmitterHandle* GetEmitterHandle();

	/** Gets the view model for the emitter this handle references. */
	TSharedRef<FNiagaraEmitterViewModel> GetEmitterViewModel();

	/** Compiles the spawn and update scripts. */
	void CompileScripts();

	/** Refreshes the copied emitter's graph and inputs from the source asset.  Input values
		will be preserved. */
	void RefreshFromSource();

	/** Replaces the coped emitter instance with a fresh copy of the source emitter asset.  Any changes
		to input parameters will be lost. */
	void ResetToSource();

	/** Opens the source emitter in a stand alone asset editor. */
	void OpenSourceEmitter();

	/** Gets a multicast delegate which is called any time a property on the handle changes. */
	FOnPropertyChanged& OnPropertyChanged();

private:
	/** The emitter handle being displayed and edited by this view model. */
	FNiagaraEmitterHandle* EmitterHandle;

	/** The effect which owns the handled being displayed and edited by this view model. */
	UNiagaraEffect& OwningEffect;

	/** The edit mode for parameters in this view model. */
	ENiagaraParameterEditMode ParameterEditMode;

	/** The view model for emitter this handle references. */
	TSharedRef<FNiagaraEmitterViewModel> EmitterViewModel;

	/** A multicast delegate which is called any time a property on the handle changes. */
	FOnPropertyChanged OnPropertyChangedDelegate;
};