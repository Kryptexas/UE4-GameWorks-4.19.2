// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"

UNiagaraNode::UNiagaraNode(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

UNiagaraGraph* UNiagaraNode::GetNiagaraGraph()
{
	return CastChecked<UNiagaraGraph>(GetGraph());
}

class UNiagaraScriptSource* UNiagaraNode::GetSource()
{
	return GetNiagaraGraph()->GetSource();
}
