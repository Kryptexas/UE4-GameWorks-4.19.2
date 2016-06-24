// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "GraphEditorActions.h"
#include "AnimGraphNode_PoseByName.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_PoseByName

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_PoseByName::UAnimGraphNode_PoseByName(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAnimGraphNode_PoseByName::PreloadRequiredAssets()
{
	PreloadObject(Node.PoseAsset);

	Super::PreloadRequiredAssets();
}


void UAnimGraphNode_PoseByName::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& AnimationAssets) const
{
	if(Node.PoseAsset)
	{
		HandleAnimReferenceCollection(Node.PoseAsset, AnimationAssets);
	}
}

void UAnimGraphNode_PoseByName::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& AnimAssetReplacementMap)
{
	HandleAnimReferenceReplacement(Node.PoseAsset, AnimAssetReplacementMap);
}

FText UAnimGraphNode_PoseByName::GetTooltipText() const
{
	// FText::Format() is slow, so we utilize the cached list title
	return GetNodeTitle(ENodeTitleType::ListView);
}

FText UAnimGraphNode_PoseByName::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (Node.PoseAsset == nullptr)
	{
		return LOCTEXT("PoseByName_TitleNONE", "Pose (None)");
	}
	// @TODO: don't know enough about this node type to comfortably assert that
	//        the CacheName won't change after the node has spawned... until
	//        then, we'll leave this optimization off
	else //if (CachedNodeTitle.IsOutOfDate(this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("PoseAssetName"), FText::FromString(Node.PoseAsset->GetName()));
		Args.Add(TEXT("PoseName"), FText::FromString(Node.PoseName.ToString()));

		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitle.SetCachedText(FText::Format(LOCTEXT("PoseByName_Title", "{PoseAssetName} : {PoseName}"), Args), this);
	}

	return CachedNodeTitle;
}

// void UAnimGraphNode_PoseByName::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
// {
// 	// Intentionally empty; you can drop down a regular sequence player and convert into a sequence evaluator in the right-click menu.
// }

void UAnimGraphNode_PoseByName::SetAnimationAsset(UAnimationAsset* Asset)
{
	if (UPoseAsset* PoseAsset =  Cast<UPoseAsset>(Asset))
	{
		Node.PoseAsset = PoseAsset;
	}
}

void UAnimGraphNode_PoseByName::ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog)
{
	if (Node.PoseAsset == NULL)
	{
		MessageLog.Error(TEXT("@@ references an unknown sequence"), this);
	}
	else
	{
		USkeleton* SeqSkeleton = Node.PoseAsset->GetSkeleton();
		if (SeqSkeleton&& // if anim sequence doesn't have skeleton, it might be due to anim sequence not loaded yet, @todo: wait with anim blueprint compilation until all assets are loaded?
			!SeqSkeleton->IsCompatible(ForSkeleton))
		{
			MessageLog.Error(TEXT("@@ references sequence that uses different skeleton @@"), this, SeqSkeleton);
		}
	}
}

// potentially in the future, give options for the pose name?
// void UAnimGraphNode_PoseByName::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
// {
// 	if (!Context.bIsDebugging)
// 	{
// 		// add an option to convert to a regular sequence player
// 		Context.MenuBuilder->BeginSection("AnimGraphNodePoseByName", NSLOCTEXT("A3Nodes", "PoseByNameHeading", "Sequence Evaluator"));
// 		{
// 			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().OpenRelatedAsset);
// 			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().ConvertToSeqPlayer);
// 		}
// 		Context.MenuBuilder->EndSection();
// 	}
// }

bool UAnimGraphNode_PoseByName::DoesSupportTimeForTransitionGetter() const
{
	return false;
}

UAnimationAsset* UAnimGraphNode_PoseByName::GetAnimationAsset() const 
{
	return Node.PoseAsset;
}

void UAnimGraphNode_PoseByName::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	if (!Context.bIsDebugging)
	{
		// add an option to convert to single frame
		Context.MenuBuilder->BeginSection("AnimGraphNodePoseByName", LOCTEXT("PoseByNameHeading", "Pose By Name"));
		{
			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().ConvertToPoseBlender);
		}
		Context.MenuBuilder->EndSection();
	}
}
#undef LOCTEXT_NAMESPACE