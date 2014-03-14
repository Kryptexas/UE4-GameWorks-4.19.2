// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeGetAttr.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeGetAttr : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

	/** Name of attribute we are getting */
	UPROPERTY()
	FName	AttrName;

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End EdGraphNode interface
};

