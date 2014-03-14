// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_ApplyAdditive.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_ApplyAdditive : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_ApplyAdditive Node;

#if WITH_EDITOR
	// UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const OVERRIDE;
	// End of UAnimGraphNode_Base interface
#endif
};
