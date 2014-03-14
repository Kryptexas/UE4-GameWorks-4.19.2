// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_LocalToComponentSpace.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_LocalToComponentSpace : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_ConvertLocalToComponentSpace Node;

	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const OVERRIDE;
	virtual void CreateOutputPins() OVERRIDE;
	virtual void PostProcessPinName(const UEdGraphPin* Pin, FString& DisplayName) const OVERRIDE;
	// End of UAnimGraphNode_Base interface
};
