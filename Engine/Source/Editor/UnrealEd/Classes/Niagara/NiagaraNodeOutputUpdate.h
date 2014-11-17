// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeOutputUpdate.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeOutputUpdate : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	// End EdGraphNode interface
};

