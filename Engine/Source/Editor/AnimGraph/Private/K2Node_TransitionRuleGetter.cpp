// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "AnimationGraphSchema.h"
#include "AnimGraphNode_Base.h"
#include "AnimStateNode.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintActionFilter.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintUtilities.h"
#include "K2Node_TransitionRuleGetter.h"
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
	// @TODO: FText::Format() is slow... consider caching this tooltip like 
	//        we do for a lot of the BP nodes now (unfamiliar with this 
	//        node's purpose, so hesitant to muck with this at this time).

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

void UK2Node_TransitionRuleGetter::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	if( const UAnimBlueprint* AnimBlueprint = Cast<UAnimBlueprint>( ActionRegistrar.GetActionKeyFilter() ) )
	{
		TArray<UAnimStateNode*> States;
		FBlueprintEditorUtils::GetAllNodesOfClass(AnimBlueprint, /*out*/ States);

		for (auto StateIt = States.CreateIterator(); StateIt; ++StateIt)
		{
			UAnimStateNode* StateNode = *StateIt;
			
			auto UiSpecOverride = [](const FBlueprintActionContext& /*Context*/, const IBlueprintNodeBinder::FBindingSet& Bindings, FBlueprintActionUiSpec* UiSpecOut, UAnimStateNode* StateNode)
			{
				const FString OwnerName = StateNode->GetOuter()->GetName();
				UiSpecOut->MenuName = FText::Format(LOCTEXT("TransitionRuleGetterTitle", "Current {0} for state '{1}.{2}'"), 
														UK2Node_TransitionRuleGetter::GetFriendlyName(ETransitionGetter::ArbitraryState_GetBlendWeight), 
														FText::FromString(OwnerName), 
														FText::FromString(StateNode->GetStateName()));
			};

			auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, UAnimStateNode* StateNode)
			{
				UK2Node_TransitionRuleGetter* NewNodeTyped = CastChecked<UK2Node_TransitionRuleGetter>(NewNode);
				NewNodeTyped->AssociatedStateNode = StateNode;
				NewNodeTyped->GetterType = ETransitionGetter::ArbitraryState_GetBlendWeight;
			};

			UBlueprintNodeSpawner* Spawner = UBlueprintNodeSpawner::Create( UK2Node_TransitionRuleGetter::StaticClass(), nullptr, UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda, StateNode) );
			Spawner->DynamicUiSignatureGetter = UBlueprintNodeSpawner::FUiSpecOverrideDelegate::CreateStatic(UiSpecOverride, StateNode);
			ActionRegistrar.AddBlueprintAction( AnimBlueprint, Spawner );
		}
	}
}

FText UK2Node_TransitionRuleGetter::GetTooltipText() const
{
	return GetNodeTitle(ENodeTitleType::FullTitle);
}

bool UK2Node_TransitionRuleGetter::CanCreateUnderSpecifiedSchema(const UEdGraphSchema* Schema) const
{
	return Cast<UAnimationGraphSchema>(Schema) != NULL;
}

bool UK2Node_TransitionRuleGetter::IsActionFilteredOut(class FBlueprintActionFilter const& Filter)
{
	// only show the transition nodes if the associated state node is in every graph:
	if( AssociatedStateNode )
	{
		for( auto Blueprint : Filter.Context.Blueprints )
		{
			TArray<UAnimStateNode*> States;
			FBlueprintEditorUtils::GetAllNodesOfClass(Blueprint, /*out*/ States);

			if( !States.Contains( AssociatedStateNode ) )
			{
				return true;
			}
		}
		return false;
	}
	return true;
}

UEdGraphPin* UK2Node_TransitionRuleGetter::GetOutputPin() const
{
	return FindPinChecked(TEXT("Output"));
}

#undef LOCTEXT_NAMESPACE
