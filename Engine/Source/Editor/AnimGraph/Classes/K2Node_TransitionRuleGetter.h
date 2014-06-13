// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "K2Node_TransitionRuleGetter.generated.h"

UENUM()
namespace ETransitionGetter
{
	enum Type
	{
		AnimationAsset_GetCurrentTime,
		AnimationAsset_GetLength,
		AnimationAsset_GetCurrentTimeFraction,
		AnimationAsset_GetTimeFromEnd,
		AnimationAsset_GetTimeFromEndFraction,
		CurrentState_ElapsedTime,
		CurrentState_GetBlendWeight,
		CurrentTransitionDuration,
		ArbitraryState_GetBlendWeight
	};
}


UCLASS(MinimalAPI)
class UK2Node_TransitionRuleGetter : public UK2Node
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	TEnumAsByte<ETransitionGetter::Type> GetterType;

	UPROPERTY()
	class UAnimGraphNode_Base* AssociatedAnimAssetPlayerNode;

	UPROPERTY()
	UObject* AssociatedStateNode;

	// UEdGraphNode interface
	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetNodeNativeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FString GetTooltip() const override;
	virtual bool ShowPaletteIconOnNode() const override{ return false; }
	// End of UEdGraphNode interface

	// @todo document
	ANIMGRAPH_API UEdGraphPin* GetOutputPin() const;

	// @todo document
	ANIMGRAPH_API static FText GetFriendlyName(ETransitionGetter::Type TypeID);
};

