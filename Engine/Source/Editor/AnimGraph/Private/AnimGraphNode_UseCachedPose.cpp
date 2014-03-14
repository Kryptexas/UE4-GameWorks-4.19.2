// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "GraphEditorActions.h"
#include "ScopedTransaction.h"
#include "K2ActionMenuBuilder.h" // for FK2ActionMenuBuilder::AddNewNodeAction()

/////////////////////////////////////////////////////
// UAnimGraphNode_UseCachedPose

UAnimGraphNode_UseCachedPose::UAnimGraphNode_UseCachedPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_UseCachedPose::GetTooltip() const
{
	return TEXT("References an animation tree elsewhere in the blueprint, which will be evaluated at most once per frame.");
}

FString UAnimGraphNode_UseCachedPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
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
		}
	}
}