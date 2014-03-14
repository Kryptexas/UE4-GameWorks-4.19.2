// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_SequenceEvaluator.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_SequenceEvaluator : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_SequenceEvaluator Node;

	// UEdGraphNode interface
	FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	FString GetTooltip() const OVERRIDE;
	// End of UEdGraphNode

	// UAnimGraphNode_Base interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog) OVERRIDE;
	virtual void PreloadRequiredAssets() OVERRIDE;	
	// Interface to support transition getter
	virtual bool DoesSupportTimeForTransitionGetter() const OVERRIDE;
	virtual UAnimationAsset* GetAnimationAsset() const OVERRIDE;
	virtual const TCHAR* GetTimePropertyName() const OVERRIDE;
	virtual UScriptStruct* GetTimePropertyStruct() const OVERRIDE;
	virtual void GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& ComplexAnims, TArray<UAnimSequence*>& AnimationSequences) const OVERRIDE;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ComplexAnimsMap, const TMap<UAnimSequence*, UAnimSequence*>& AnimSequenceMap) OVERRIDE;
	// End of UAnimGraphNode_Base

	// UK2Node interface
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	// End of UK2Node interface
};
