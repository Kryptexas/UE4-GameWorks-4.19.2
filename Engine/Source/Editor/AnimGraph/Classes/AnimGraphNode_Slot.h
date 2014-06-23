// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_Base.h"
#include "Animation/AnimNode_Slot.h"
#include "AnimGraphNode_Slot.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_Slot : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_Slot Node;

	// Begin UEdGraphNode interface.
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FString GetTooltip() const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	// End UEdGraphNode interface.

	// UAnimGraphNode_Base interface
	virtual FString GetNodeCategory() const override;
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) override;
	// End of UAnimGraphNode_Base interface
};
