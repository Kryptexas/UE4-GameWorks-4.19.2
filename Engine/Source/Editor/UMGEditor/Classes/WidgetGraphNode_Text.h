// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetGraphNode_Text.generated.h"

UCLASS(MinimalAPI)
class UWidgetGraphNode_Text : public UWidgetGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FWidgetNode_Text Node;

	// UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	/*
	// UAnimGraphNode_Base interface
	virtual void CustomizePinData(UEdGraphPin* Pin, FName SourcePropertyName, int32 ArrayIndex) const OVERRIDE;
	// End of UAnimGraphNode_Base interface
	*/
};
