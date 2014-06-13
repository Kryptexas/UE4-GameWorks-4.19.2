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
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	// End EdGraphNode interface
};

