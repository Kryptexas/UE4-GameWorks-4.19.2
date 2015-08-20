// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeReadDataSet.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeReadDataSet : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(EditAnywhere, Category = DataSet)
	FNiagaraDataSetID DataSet;

	UPROPERTY(EditAnywhere, Category = Variables)
	TArray<FNiagaraVariableInfo> Variables;

	// Begin UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End UObject interface

	// Begin EdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	// End EdGraphNode interface

	virtual void Compile(class INiagaraCompiler* Compiler, TArray<FNiagaraNodeResult>& Outputs)override;
};

