// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()
#include "AnimGraphNode_SaveCachedPose.h"
#include "AnimGraphNode_UseCachedPose.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintActionFilter.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_UseCachedPose

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_UseCachedPose::UAnimGraphNode_UseCachedPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UAnimGraphNode_UseCachedPose::PostLoad()
{
	Super::PostLoad();

	// If there is no SaveCachedPose node set but there is a NameOfCache available, we may be updating to the new system
	// Go through and find the cached node, if possible.
	if(!SaveCachedPoseNode.IsValid() && !NameOfCache.IsEmpty())
	{
		TArray<UEdGraph*> AllAnimationGraphs;
		GetGraph()->GetAllChildrenGraphs(AllAnimationGraphs);
		AllAnimationGraphs.Add(GetGraph());

		for(UEdGraph* Graph : AllAnimationGraphs)
		{
			// Get a list of all save cached pose nodes
			TArray<UAnimGraphNode_SaveCachedPose*> CachedPoseNodes;
			Graph->GetNodesOfClass(CachedPoseNodes);

			// Go through all the nodes and find one with a title that matches ours
			for (auto NodeIt = CachedPoseNodes.CreateIterator(); NodeIt; ++NodeIt)
			{
				if((*NodeIt)->CacheName == NameOfCache)
				{
					SaveCachedPoseNode = *NodeIt;
					break;
				}
			}
		}
	}
}

FText UAnimGraphNode_UseCachedPose::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_UseCachedPose_Tooltip", "References an animation tree elsewhere in the blueprint, which will be evaluated at most once per frame.");
}

FText UAnimGraphNode_UseCachedPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	if(SaveCachedPoseNode.IsValid())
	{
		NameOfCache = SaveCachedPoseNode->CacheName;
	}
	Args.Add(TEXT("CachePoseName"), FText::FromString(NameOfCache));
	return FText::Format(LOCTEXT("AnimGraphNode_UseCachedPose_Title", "Use cached pose '{CachePoseName}'"), Args);
}

FString UAnimGraphNode_UseCachedPose::GetNodeCategory() const
{
	return TEXT("Cached Poses");
}

void UAnimGraphNode_UseCachedPose::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	//@TODO: Check the type of the from pin to make sure it's a pose
	if ((ContextMenuBuilder.FromPin == NULL) || (ContextMenuBuilder.FromPin->Direction == EGPD_Input))
	{
		// Get a list of all save cached pose nodes
		TArray<UAnimGraphNode_SaveCachedPose*> CachedPoseNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UAnimGraphNode_SaveCachedPose>(FBlueprintEditorUtils::FindBlueprintForGraphChecked(ContextMenuBuilder.CurrentGraph), /*out*/ CachedPoseNodes);

		// Offer a use node for each of them
		for (auto NodeIt = CachedPoseNodes.CreateIterator(); NodeIt; ++NodeIt)
		{
			UAnimGraphNode_UseCachedPose* UseCachedPose = NewObject<UAnimGraphNode_UseCachedPose>();
			UseCachedPose->NameOfCache = (*NodeIt)->CacheName;
			UseCachedPose->SaveCachedPoseNode = *NodeIt;

			TSharedPtr<FEdGraphSchemaAction_K2NewNode> UseCachedPoseAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, GetNodeCategory(), UseCachedPose->GetNodeTitle(ENodeTitleType::ListView), UseCachedPose->GetTooltipText().ToString(), 0, UseCachedPose->GetKeywords());
			UseCachedPoseAction->NodeTemplate = UseCachedPose;
		}
	}
}


void UAnimGraphNode_UseCachedPose::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool bIsTemplateNode, FString CacheNodeName, UAnimGraphNode_SaveCachedPose* SaveCachePoseNode)
	{
		UAnimGraphNode_UseCachedPose* UseCachedPose = CastChecked<UAnimGraphNode_UseCachedPose>(NewNode);
		// we use an empty CacheName in GetNodeTitle() to relay the proper menu title
		UseCachedPose->SaveCachedPoseNode = SaveCachePoseNode;
	};


	UObject const* ActionKey = ActionRegistrar.GetActionKeyFilter();

	if(UBlueprint const* Blueprint = Cast<UBlueprint>(ActionKey))
	{
		// Get a list of all save cached pose nodes
		TArray<UAnimGraphNode_SaveCachedPose*> CachedPoseNodes;
		FBlueprintEditorUtils::GetAllNodesOfClass<UAnimGraphNode_SaveCachedPose>(Blueprint, /*out*/ CachedPoseNodes);

		// Offer a use node for each of them
		for (auto NodeIt = CachedPoseNodes.CreateIterator(); NodeIt; ++NodeIt)
		{
			UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda, (*NodeIt)->CacheName, *NodeIt);

			ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
		}
	}
}

bool UAnimGraphNode_UseCachedPose::IsActionFilteredOut(class FBlueprintActionFilter const& Filter)
{
	bool bIsFilteredOut = false;
	if(SaveCachedPoseNode.IsValid())
	{
		FBlueprintActionContext const& FilterContext = Filter.Context;
		for(UBlueprint* Blueprint : FilterContext.Blueprints)
		{
			if(SaveCachedPoseNode->GetBlueprint() != Blueprint)
			{
				bIsFilteredOut = true;
				break;
			}
		}
	}
	return bIsFilteredOut;
}

#undef LOCTEXT_NAMESPACE