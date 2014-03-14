// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeOutputUpdate.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeOutputUpdate : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End EdGraphNode interface
};

