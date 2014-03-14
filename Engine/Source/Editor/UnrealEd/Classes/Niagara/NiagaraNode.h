// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNode.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNode : public UEdGraphNode
{
	GENERATED_UCLASS_BODY()

	/** Get the Niagara graph that owns this node */
	class UNiagaraGraph* GetNiagaraGraph();

	/** Get the source object */
	class UNiagaraScriptSource* GetSource();
};

