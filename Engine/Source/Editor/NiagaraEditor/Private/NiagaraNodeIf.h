// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraNodeWithDynamicPins.h"
#include "EdGraphSchema_Niagara.h"
#include "NiagaraNodeIf.generated.h"

UCLASS(MinimalAPI)
class UNiagaraNodeIf : public UNiagaraNodeWithDynamicPins
{
	GENERATED_UCLASS_BODY()

public:

	/** Outputs of this branch. */
	UPROPERTY(EditAnywhere, Category=If)
	TArray<FNiagaraVariable> OutputVars;

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	//~ End UObject Interface

	//~ Begin EdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	//~ End EdGraphNode Interface

	//~ Begin UNiagaraNode Interface
	virtual void Compile(class INiagaraCompiler* Compiler, TArray<int32>& Outputs) override;
	virtual bool RefreshFromExternalChanges() override;
	//~ End UNiagaraNode Interface

protected:
	//~ Begin EdGraphNode Interface
	virtual void OnPinRemoved(UEdGraphPin* PinToRemove) override;
	//~ End EdGraphNode Interface

	//~ Begin UNiagaraNodeWithDynamicPins Interface
	virtual void OnNewTypedPinAdded(UEdGraphPin* NewPin) override;
	virtual void OnPinRenamed(UEdGraphPin* RenamedPin) override;
	virtual bool CanRenamePin(const UEdGraphPin* Pin) const override;
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const override;
	//~ End UNiagaraNodeWithDynamicPins Interface
};
