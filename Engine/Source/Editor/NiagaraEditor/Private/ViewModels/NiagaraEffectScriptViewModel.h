// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraScriptViewModel.h"

class FNiagaraScriptViewModel;
class UNiagaraEffect;

/** View model which manages the effect parameters, effect script, and effect parameter bindings. */
class FNiagaraEffectScriptViewModel : public FNiagaraScriptViewModel
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnParameterBindingsChanged);

public:
	FNiagaraEffectScriptViewModel(UNiagaraEffect& InEffect);

	~FNiagaraEffectScriptViewModel();

	/** Refreshes the emitter nodes in the effect script due to data changes. */
	void RefreshEmitterNodes();

	/** Gets a multicast delegate which is called whenever the parameter bindins on the effect script change. */
	FOnParameterBindingsChanged& OnParameterBindingsChanged();

	/** Called whenever a parameter value on the effect changes. */
	void EffectParameterValueChanged(FGuid ParameterId);


private:
	/** Synchronizes the effect parameters, and parameter bindings with the state of the graph. */
	void SynchronizeParametersAndBindingsWithGraph();

	/** Called whenever the graph for the effect script changes. */
	void OnGraphChanged(const struct FEdGraphEditAction& InAction);

private:
	/** The effect who's script is getting viewed and edited by this view model. */
	UNiagaraEffect& Effect;

	/** A handle to the on graph changed delegate. */
	FDelegateHandle OnGraphChangedHandle;

	/** A multicast delegate which is called whenever the parameter bindins on the effect script change. */
	FOnParameterBindingsChanged OnParameterBindingsChangedDelegate;
};