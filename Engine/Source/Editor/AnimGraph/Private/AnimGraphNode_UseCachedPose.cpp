// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

/////////////////////////////////////////////////////
// UAnimGraphNode_UseCachedPose

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_UseCachedPose::UAnimGraphNode_UseCachedPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_UseCachedPose::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_UseCachedPose_Tooltip", "References an animation tree elsewhere in the blueprint, which will be evaluated at most once per frame.").ToString();
}

FText UAnimGraphNode_UseCachedPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("CachePoseName"), FText::FromString(NameOfCache));
	return FText::Format(LOCTEXT("AnimGraphNode_UseCachedPose_Title", "Use cached pose '{CachePoseName}'"), Args);
}

FString UAnimGraphNode_UseCachedPose::GetNodeNativeTitle(ENodeTitleType::Type TitleType) const
{
	// Do not setup this function for localization, intentionally left unlocalized!
	return FString::Printf(TEXT("Use cached pose '%s'"), *NameOfCache);
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

			TSharedPtr<FEdGraphSchemaAction_K2NewNode> UseCachedPoseAction = FK2ActionMenuBuilder::AddNewNodeAction(ContextMenuBuilder, GetNodeCategory(), UseCachedPose->GetNodeTitle(ENodeTitleType::ListView), UseCachedPose->GetTooltip(), 0, UseCachedPose->GetKeywords());
			UseCachedPoseAction->NodeTemplate = UseCachedPose;
			UseCachedPoseAction->SearchTitle = UseCachedPose->GetNodeSearchTitle();
		}
	}
}

#undef LOCTEXT_NAMESPACE