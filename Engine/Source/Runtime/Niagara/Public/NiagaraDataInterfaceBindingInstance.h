// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraScript.h"
struct FNiagaraScriptDataInterfaceInfo;

/** A runtime instance of an effect parameter binding. */
class FNiagaraDataInterfaceBindingInstance
{
public:
	FNiagaraDataInterfaceBindingInstance(const FNiagaraScriptDataInterfaceInfo& InSource, FNiagaraScriptDataInterfaceInfo& InDestination, ENiagaraScriptUsage InUsage, bool bInDirectPointerCopy = true);

	/** Updates the data interface binding each frame. */
	bool Tick();

	/** Get the script type that this binding refers to.*/
	ENiagaraScriptUsage GetScriptUsage() const { return Usage; }

	/** Get the source's Id.*/
	FGuid GetSourceId() const { return Source.Id; }
	
	/** Get the destination's Id.*/
	FGuid GetDetinationId() const { return Destination.Id; }

private:
	/** The source variable for the binding. */
	const FNiagaraScriptDataInterfaceInfo& Source;

	/** The destination variable for the binding. */
	FNiagaraScriptDataInterfaceInfo& Destination;

	/** Script Usage*/
	ENiagaraScriptUsage Usage;

	/** Whether or not to copy the data interface directly or to use the CopyTo methods.*/
	bool bDirectPointerCopy;
};