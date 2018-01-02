// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

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

	UPROPERTY(EditAnywhere, Category = If)
	TArray<FGuid> OutputVarGuids;

	//~ Begin UObject Interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostLoad() override;
	//~ End UObject Interface

	//~ Begin EdGraphNode Interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetTooltipText() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	//~ End EdGraphNode Interface

	//~ Begin UNiagaraNode Interface
	virtual void Compile(class FHlslNiagaraTranslator* Translator, TArray<int32>& Outputs) override;
	virtual bool RefreshFromExternalChanges() override;
	//~ End UNiagaraNode Interface

	/** Helper function to create a variable to add to the OutputVars and FGuid to add to OutputVarGuids. */
	FGuid AddOutput(FNiagaraTypeDefinition Type, const FName& Name);

protected:
	//~ Begin EdGraphNode Interface
	virtual void OnPinRemoved(UEdGraphPin* PinToRemove) override;
	//~ End EdGraphNode Interface

	//~ Begin UNiagaraNodeWithDynamicPins Interface
	virtual void OnNewTypedPinAdded(UEdGraphPin* NewPin) override;
	virtual void OnPinRenamed(UEdGraphPin* RenamedPin, const FString& OldName) override;
	virtual bool CanRenamePin(const UEdGraphPin* Pin) const override;
	virtual bool CanRemovePin(const UEdGraphPin* Pin) const override;
	virtual bool CanMovePin(const UEdGraphPin* Pin) const override { return false; }
	virtual bool AllowNiagaraTypeForAddPin(const FNiagaraTypeDefinition& InType) override;
	//~ End UNiagaraNodeWithDynamicPins Interface
};
