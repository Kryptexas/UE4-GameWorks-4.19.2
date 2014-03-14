// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_SequencePlayer.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_SequencePlayer : public UAnimGraphNode_Base
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_SequencePlayer Node;

	// Sync group settings for this player.  Sync groups keep related animations with different lengths synchronized.
	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimationGroupReference SyncGroup;

	// UEdGraphNode interface
	virtual FLinearColor GetNodeTitleColor() const OVERRIDE;
	virtual FString GetTooltip() const OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	// End of UEdGraphNode interface

	// UAnimGraphNode_Base interface
	virtual void GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const OVERRIDE;
	virtual void ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog) OVERRIDE;
	virtual void PreloadRequiredAssets() OVERRIDE;		
	virtual void BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog) OVERRIDE;
	virtual bool DoesSupportTimeForTransitionGetter() const OVERRIDE;
	virtual UAnimationAsset* GetAnimationAsset() const OVERRIDE;
	virtual const TCHAR* GetTimePropertyName() const OVERRIDE;
	virtual UScriptStruct* GetTimePropertyStruct() const OVERRIDE;
	virtual void GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& ComplexAnims, TArray<UAnimSequence*>& AnimationSequences) const OVERRIDE;
	virtual void ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ComplexAnimsMap, const TMap<UAnimSequence*, UAnimSequence*>& AnimSequenceMap) OVERRIDE;
	// End of UAnimGraphNode_Base interface

	// UK2Node interface
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;
	// End of UK2Node interface

private:
	static FString GetTitleGivenAssetInfo(const FString& AssetName, bool bKnownToBeAdditive);
};
