// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_SwitchEnum.generated.h"

UCLASS(MinimalAPI)
class UK2Node_SwitchEnum : public UK2Node_Switch, public INodeDependingOnEnumInterface
{
	GENERATED_UCLASS_BODY()

	/** Name of the enum being switched on */
	UPROPERTY()
	UEnum* Enum;

	/** List of the current entries in the enum */
	UPROPERTY()
	TArray<FName> EnumEntries;

	/** List of the current entries in the enum */
	UPROPERTY()
	TArray<FString> EnumFriendlyNames;

	// INodeDependingOnEnumInterface
	virtual class UEnum* GetEnum() const OVERRIDE { return Enum; }
	virtual bool ShouldBeReconstructedAfterEnumChanged() const OVERRIDE {return true;}
	// End of INodeDependingOnEnumInterface

#if WITH_EDITOR
	// UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual bool ShouldShowNodeProperties() const OVERRIDE { return true; }	
	// End of UEdGraphNode interface

	// UK2Node_Switch Interface
	virtual FString GetUniquePinName() OVERRIDE;
	virtual const FString& GetPinType(const UEdGraphSchema_K2* Schema) const OVERRIDE { return Schema->PC_Byte; }
	virtual void AddPinToSwitchNode() OVERRIDE;
	virtual void RemovePinFromSwitchNode(UEdGraphPin* TargetPin) OVERRIDE;
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const OVERRIDE;
	// End of UK2Node_Switch Interface

	/** Bind the switch to a named enum */
	void SetEnum(UEnum* InEnum);

protected:
	/** Helper method to set-up pins */
	virtual void CreateCasePins() OVERRIDE;
	/** Helper method to set-up correct selection pin */
	virtual void CreateSelectionPin() OVERRIDE;
	
	/** Don't support removing pins from an enum */
	virtual void RemovePin(UEdGraphPin* TargetPin) OVERRIDE {}
#endif 
};
