// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "NiagaraCommon.h"
#include "NiagaraEditorCommon.h"
#include "NiagaraNode.h"
#include "NiagaraNodeOutput.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeOutput : public UNiagaraNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category = Output)
	TArray<FNiagaraVariable> Outputs;

public:

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin EdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual bool CanUserDeleteNode() const override { return false; }
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	//~ End EdGraphNode Interface

	/** Notifies the node that it's output variables have been modified externally. */
	void NotifyOutputVariablesChanged();

	virtual void Compile(class INiagaraCompiler* Compiler, TArray<int32>& OutputExpressions)override;
protected:
	virtual int32 CompileInputPin(INiagaraCompiler* Compiler, UEdGraphPin* Pin) override;

};

