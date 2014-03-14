// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeOp.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeOp : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

	/** Index of operation */
	UPROPERTY()
	uint8 OpIndex;

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End EdGraphNode interface

	/** Names to use for each input pin. */
	UNREALED_API static const TCHAR* InPinNames[3];
};

