// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Switch.h"
#include "K2Node_SwitchString.generated.h"

UCLASS(MinimalAPI)
class UK2Node_SwitchString : public UK2Node_Switch
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=PinOptions)
	TArray<FString> PinNames;

	UPROPERTY(EditAnywhere, Category=PinOptions)
	uint32 bIsCaseSensitive : 1;

	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface

	// UEdGraphNode interface
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }
	// End of UEdGraphNode interface

	// UK2Node_Switch Interface
	BLUEPRINTGRAPH_API virtual void AddPinToSwitchNode() override;
	virtual FString GetUniquePinName() override;
	virtual const FString& GetPinType(const UEdGraphSchema_K2* Schema) const override { return Schema->PC_String; }
	// End of UK2Node_Switch Interface

	virtual FString GetPinNameGivenIndex(int32 Index) override;

protected:
	virtual void CreateSelectionPin() override;
	virtual void CreateCasePins() override;
	virtual void RemovePin(UEdGraphPin* TargetPin) override;
};
