// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_Base.h"
#include "AnimGraphNode_Slot.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_Slot : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_Slot Node;

	// Begin UEdGraphNode interface.
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End UEdGraphNode interface.

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const OVERRIDE;
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) OVERRIDE;
	// End of UAnimGraphNode_Base interface
};
