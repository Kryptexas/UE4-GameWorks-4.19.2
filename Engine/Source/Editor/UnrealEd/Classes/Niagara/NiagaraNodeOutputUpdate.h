// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeOutputUpdate.generated.h"

UCLASS(MinimalAPI, Deprecated)
class UDEPRECATED_NiagaraNodeOutputUpdate : public UNiagaraNode
{
	GENERATED_BODY()
public:
	UNREALED_API UDEPRECATED_NiagaraNodeOutputUpdate(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

public:

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	// End EdGraphNode interface

	void Compile(class INiagaraCompiler* Compiler, TArray<FNiagaraNodeResult>& Outputs)override;
};

