// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NiagaraScriptViewModel.h"

class FNiagaraScriptViewModel;
class UNiagaraSystem;

/** View model which manages the System script. */
class FNiagaraSystemScriptViewModel : public FNiagaraScriptViewModel
{
public:
	DECLARE_MULTICAST_DELEGATE(FOnSystemCompiled)

public:
	FNiagaraSystemScriptViewModel(UNiagaraSystem& InSystem, class FNiagaraSystemViewModel* InParent);

	~FNiagaraSystemScriptViewModel();

	/** Rebuilds the emitter nodes in the System script due to data changes. */
	void RebuildEmitterNodes();

	FOnSystemCompiled& OnSystemCompiled();

	void CompileSystem(bool bForce);

private:
	/** Called whenever the graph for the System script changes. */
	void OnGraphChanged(const struct FEdGraphEditAction& InAction);

	FNiagaraSystemViewModel* Parent;

	/** The System who's script is getting viewed and edited by this view model. */
	UNiagaraSystem& System;

	/** A handle to the on graph changed delegate. */
	FDelegateHandle OnGraphChangedHandle;

	FOnSystemCompiled OnSystemCompiledDelegate;
};