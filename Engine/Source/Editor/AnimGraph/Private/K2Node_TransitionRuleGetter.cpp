// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "AnimGraphPrivatePCH.h"
#include "K2Node_TransitionRuleGetter.h"
#include "BlueprintUtilities.h"
#include "AnimGraphNode_Base.h"
#include "AnimStateNode.h"
/////////////////////////////////////////////////////
// UK2Node_TransitionRuleGetter

#define LOCTEXT_NAMESPACE "TransitionRuleGetter"

UK2Node_TransitionRuleGetter::UK2Node_TransitionRuleGetter(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UK2Node_TransitionRuleGetter::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	UEdGraphPin* OutputPin = CreatePin(EGPD_Output, Schema->PC_Float, /*PSC=*/ TEXT(""), /*PSC object=*/ NULL, /*bIsArray=*/ false, /*bIsReference=*/ false, TEXT("Output"));
	OutputPin->PinFriendlyName = GetFriendlyName(GetterType);

	PreloadObject(AssociatedAnimAssetPlayerNode);
	PreloadObject(AssociatedStateNode);

	Super::AllocateDefaultPins();
}

FText UK2Node_TransitionRuleGetter::GetFriendlyName(ETransitionGetter::Type TypeID)
{
	FText FriendlyName;
	switch (TypeID)
	{
	case ETransitionGetter::AnimationAsset_GetCurrentTime:
		FriendlyName = LOCTEXT("AnimationAssetTimeElapsed", "CurrentTime");
		break;
	case ETransitionGetter::AnimationAsset_GetLength:
		FriendlyName = LOCTEXT("AnimationAssetSequenceLength", "Length");
		break;
	case ETransitionGetter::AnimationAsset_GetCurrentTimeFraction:
		FriendlyName = LOCTEXT("AnimationAssetFractionalTimeElapsed", "CurrentTime (ratio)");
		break;
	case ETransitionGetter::AnimationAsset_GetTimeFromEnd:
		FriendlyName = LOCTEXT("AnimationAssetTimeRemaining", "TimeRemaining");
		break;
	case ETransitionGetter::AnimationAsset_GetTimeFromEndFraction:
		FriendlyName = LOCTEXT("AnimationAssetFractionalTimeRemaining", "TimeRemaining (ratio)");
		break;
	case ETransitionGetter::CurrentState_ElapsedTime:
		FriendlyName = LOCTEXT("CurrentStateElapsedTime", "Elapsed State Time");
		break;
	case ETransitionGetter::CurrentState_GetBlendWeight:
		FriendlyName = LOCTEXT("CurrentStateBlendWeight", "State's Blend Weight");
		break;
	case ETransitionGetter::ArbitraryState_GetBlendWeight:
		FriendlyName = LOCTEXT("ArbitraryStateBlendWeight", "BlendWeight");
		break;
	case ETransitionGetter::CurrentTransitionDuration:
		FriendlyName = LOCTEXT("CrossfadeDuration", "Crossfade Duration");
		break;
	default:
		ensure(false);
		break;
	}

	return FriendlyName;
}

FText UK2Node_TransitionRuleGetter::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (AssociatedAnimAssetPlayerNode != NULL)
	{
		UAnimationAsset* BoundAsset = AssociatedAnimAssetPlayerNode->GetAnimationAsset();
		if (BoundAsset)
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("BoundAsset"), FText::FromString(BoundAsset->GetName()));
			return  FText::Format(LOCTEXT("AnimationAssetInfoGetterTitle", "{BoundAsset} Asset"), Args);
		}
	}
	else if (AssociatedStateNode != NULL)
	{
		if (UAnimStateNode* State = Cast<UAnimStateNode>(AssociatedStateNode))
		{
			const FString OwnerName = State->GetOuter()->GetName();

			FFormatNamedArguments Args;
			Args.Add(TEXT("OwnerName"), FText::FromString(OwnerName));
			Args.Add(TEXT("StateName"), FText::FromString(State->GetStateName()));
			return FText::Format(LOCTEXT("StateInfoGetterTitle", "{OwnerName}.{StateName} State"), Args);
		}
	}
	else if (GetterType == ETransitionGetter::CurrentTransitionDuration)
	{
		return LOCTEXT("TransitionDuration", "Transition");
	}
	else if (GetterType == ETransitionGetter::CurrentState_ElapsedTime ||
			 GetterType == ETransitionGetter::CurrentState_GetBlendWeight)
	{
		return LOCTEXT("CurrentState", "Current State");
	}

	return Super::GetNodeTitle(TitleType);
}

FString UK2Node_TransitionRuleGetter::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	if (AssociatedAnimAssetPlayerNode != NULL)
	{
		UAnimationAsset* BoundAsset = AssociatedAnimAssetPlayerNode->GetAnimationAsset();
		if (BoundAsset)
		{
			return  FString::Printf(TEXT("%s Asset"), *BoundAsset->GetName());
		}
	}
	else if (AssociatedStateNode != NULL)
	{
		if (UAnimStateNode* State = Cast<UAnimStateNode>(AssociatedStateNode))
		{
			const FString OwnerName = State->GetOuter()->GetName();

			return FString::Printf(TEXT("%s.%s State"), *OwnerName, *(State->GetStateName()));
		}
	}
	else if (GetterType == ETransitionGetter::CurrentTransitionDuration)
	{
		return TEXT("Transition");
	}
	else if (GetterType == ETransitionGetter::CurrentState_ElapsedTime ||
		GetterType == ETransitionGetter::CurrentState_GetBlendWeight)
	{
		return TEXT("Current State");
	}

	return Super::GetNodeNativeTitle(TitleType);
}

FString UK2Node_TransitionRuleGetter::GetTooltip() const
{
	return GetNodeTitle(ENodeTitleType::FullTitle).ToString();
}

UEdGraphPin* UK2Node_TransitionRuleGetter::GetOutputPin() const
{
	return FindPinChecked(TEXT("Output"));
}

#undef LOCTEXT_NAMESPACE
