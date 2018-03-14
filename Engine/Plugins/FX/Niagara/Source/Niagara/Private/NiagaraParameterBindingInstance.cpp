// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraParameterBindingInstance.h"
#include "NiagaraTypes.h"

FNiagaraParameterBindingInstance::FNiagaraParameterBindingInstance(const FNiagaraVariable& InSource, FNiagaraVariable& InDestination)
	: Source(InSource)
	, Destination(InDestination)
{
}

void FNiagaraParameterBindingInstance::Tick()
{
	check(Source.GetSizeInBytes() == Destination.GetSizeInBytes());
	Source.CopyTo(Destination.GetData());
}