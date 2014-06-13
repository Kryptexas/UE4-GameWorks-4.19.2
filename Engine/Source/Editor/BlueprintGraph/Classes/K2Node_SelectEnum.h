// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_Select.h"
#include "K2Node_SelectEnum.generated.h"

UCLASS(MinimalAPI, deprecated)
class UDEPRECATED_K2Node_SelectEnum : public UK2Node_Select
{
	GENERATED_UCLASS_BODY()

	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const override;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	// End UK2Node interface

	// Begin UK2Node_Select interface
	virtual void GetOptionPins(TArray<UEdGraphPin*>& OptionPins) const override;
	virtual bool CanAddOptionPinToNode() const override { return false; }
	virtual bool CanRemoveOptionPinToNode() const override { return false; }
	// End UK2Node_Select interface

	// Bind the option to a named enum 
	BLUEPRINTGRAPH_API void SetEnum(UEnum* InEnum) override;
};

