
// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_SwitchInteger.generated.h"

UCLASS(MinimalAPI)
class UK2Node_SwitchInteger : public UK2Node_Switch
{
	GENERATED_UCLASS_BODY()

	/** Set the starting index for the node */
	UPROPERTY(EditAnywhere, Category=PinOptions, meta=(NoSpinbox=true))
	int32 StartIndex;

#if WITH_EDITOR
	// UObject interface
	void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End of UObject interface

	// UEdGraphNode interface
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }
	// End of UEdGraphNode interface

	// UK2Node_Switch Interface
	virtual FString GetUniquePinName() OVERRIDE;
	virtual FString GetPinNameGivenIndex(int32 Index) OVERRIDE;
	virtual void ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& OldPins) OVERRIDE;
	virtual const FString& GetPinType(const UEdGraphSchema_K2* Schema) const OVERRIDE { return Schema->PC_Int; }
	// End of UK2Node_Switch Interface

protected:
	virtual void CreateCasePins() OVERRIDE;
	virtual void CreateSelectionPin() OVERRIDE;
	virtual void RemovePin(UEdGraphPin* TargetPin) OVERRIDE;
#endif 
};
