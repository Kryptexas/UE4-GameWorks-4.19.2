// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_SelectEnum.generated.h"

UCLASS(MinimalAPI, deprecated)
class UDEPRECATED_K2Node_SelectEnum : public UK2Node_Select
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	// Begin UEdGraphNode interface
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface

	// Begin UK2Node interface
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) OVERRIDE;
	// End UK2Node interface

	// Begin UK2Node_Select interface
	virtual void GetOptionPins(TArray<UEdGraphPin*>& OptionPins) const OVERRIDE;
	virtual bool CanAddOptionPinToNode() const OVERRIDE { return false; }
	virtual bool CanRemoveOptionPinToNode() const OVERRIDE { return false; }
	// End UK2Node_Select interface

	// Bind the option to a named enum 
	BLUEPRINTGRAPH_API void SetEnum(UEnum* InEnum) OVERRIDE;
#endif
};

