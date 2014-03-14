// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_SwitchString.generated.h"

UCLASS(MinimalAPI)
class UK2Node_SwitchString : public UK2Node_Switch
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=PinOptions)
	TArray<FString> PinNames;

	UPROPERTY(EditAnywhere, Category=PinOptions)
	uint32 bIsCaseSensitive : 1;

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	// End of UEdGraphNode interface

	// UK2Node_Switch Interface
	BLUEPRINTGRAPH_API virtual void AddPinToSwitchNode() OVERRIDE;
	virtual FString GetUniquePinName() OVERRIDE;
	virtual const FString& GetPinType(const UEdGraphSchema_K2* Schema) const OVERRIDE { return Schema->PC_String; }
	// End of UK2Node_Switch Interface

	virtual FString GetPinNameGivenIndex(int32 Index) OVERRIDE;

protected:
	virtual void CreateSelectionPin() OVERRIDE;
	virtual void CreateCasePins() OVERRIDE;
	virtual void RemovePin(UEdGraphPin* TargetPin) OVERRIDE;
#endif 
};
