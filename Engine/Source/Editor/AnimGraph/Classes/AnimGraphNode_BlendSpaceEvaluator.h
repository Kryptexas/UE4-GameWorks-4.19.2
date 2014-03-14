// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNode_BlendSpaceEvaluator.h"
#include "AnimGraphNode_BlendSpaceEvaluator.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_BlendSpaceEvaluator : public UAnimGraphNode_BlendSpaceBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_BlendSpaceEvaluator Node;

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimationGroupReference SyncGroup;

	// UEdGraphNode interface
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog) OVERRIDE;
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) OVERRIDE;
	// Interface to support transition getter
	virtual bool DoesSupportTimeForTransitionGetter() const OVERRIDE;
	virtual UAnimationAsset* GetAnimationAsset() const OVERRIDE;
	virtual const TCHAR* GetTimePropertyName() const OVERRIDE;
	virtual UScriptStruct* GetTimePropertyStruct() const OVERRIDE;
	// End of UAnimGraphNode_Base interface

	// UK2Node interface
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	// End of UK2Node interface
};
