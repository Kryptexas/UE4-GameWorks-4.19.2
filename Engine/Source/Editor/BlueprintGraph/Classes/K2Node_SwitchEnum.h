// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Switch.h"
#include "NodeDependingOnEnumInterface.h"
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
	virtual class UEnum* GetEnum() const override { return Enum; }
	virtual bool ShouldBeReconstructedAfterEnumChanged() const override {return true;}
	// End of INodeDependingOnEnumInterface

	// UEdGraphNode interface
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const override;
	virtual bool ShouldShowNodeProperties() const override { return true; }	
	// End of UEdGraphNode interface

	// UK2Node_Switch Interface
	virtual FString GetUniquePinName() override;
	virtual const FString& GetPinType(const UEdGraphSchema_K2* Schema) const override { return Schema->PC_Byte; }
	virtual void AddPinToSwitchNode() override;
	virtual void RemovePinFromSwitchNode(UEdGraphPin* TargetPin) override;
	virtual ERedirectType DoPinsMatchForReconstruction(const UEdGraphPin* NewPin, int32 NewPinIndex, const UEdGraphPin* OldPin, int32 OldPinIndex) const override;
	// End of UK2Node_Switch Interface

	/** Bind the switch to a named enum */
	void SetEnum(UEnum* InEnum);

protected:
	/** Helper method to set-up pins */
	virtual void CreateCasePins() override;
	/** Helper method to set-up correct selection pin */
	virtual void CreateSelectionPin() override;
	
	/** Don't support removing pins from an enum */
	virtual void RemovePin(UEdGraphPin* TargetPin) override {}
};
