// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "GraphEditorActions.h"
#include "CompilerResultsLog.h"
#include "AnimGraphNode_BlendSpaceEvaluator.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_BlendSpaceEvaluator

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_BlendSpaceEvaluator::UAnimGraphNode_BlendSpaceEvaluator(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_BlendSpaceEvaluator::GetTooltip() const
{
	return FString::Printf(TEXT("Blendspace Evaluator'%s'"), (Node.BlendSpace != NULL) ? *(Node.BlendSpace->GetPathName()) : TEXT("(None)"));
}

FText UAnimGraphNode_BlendSpaceEvaluator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const FText BlendSpaceName((Node.BlendSpace != NULL) ? FText::FromString(Node.BlendSpace->GetName()) : LOCTEXT("None", "(None)"));
	
	if (TitleType == ENodeTitleType::ListView)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("BlendSpaceName"), BlendSpaceName);
		return FText::Format(LOCTEXT("BlendSpaceEvaluatorListTitle", "Blendspace Evaluator '{BlendSpaceName}'"), Args);
	}
	else
	{
		FFormatNamedArguments TitleArgs;
		TitleArgs.Add(TEXT("BlendSpaceName"), BlendSpaceName);
		FText Title = FText::Format(LOCTEXT("BlendSpaceEvaluatorFullTitle", "{BlendSpaceName}\nBlendspace Evaluator"), TitleArgs);

		if ((TitleType == ENodeTitleType::FullTitle) && (SyncGroup.GroupName != NAME_None))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Title"), Title);
			Args.Add(TEXT("SyncGroupName"), FText::FromName(SyncGroup.GroupName));
			Title = FText::Format(LOCTEXT("BlendSpaceNodeGroupSubtitle", "{Title}\nSync group {SyncGroupName}"), Args);
		}

		return Title;
	}
}

void UAnimGraphNode_BlendSpaceEvaluator::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// Intentionally empty so that we don't get duplicate blend space entries.
	// You can convert a regular blend space player to an evaluator via the right click context menu
}

void UAnimGraphNode_BlendSpaceEvaluator::ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog)
{
	if (Node.BlendSpace == NULL)
	{
		MessageLog.Error(TEXT("@@ references an unknown blend space"), this);
	}
	else 
	{
		USkeleton * BlendSpaceSkeleton = Node.BlendSpace->GetSkeleton();
		if (BlendSpaceSkeleton&& // if blend space doesn't have skeleton, it might be due to blend space not loaded yet, @todo: wait with anim blueprint compilation until all assets are loaded?
			!BlendSpaceSkeleton->IsCompatible(ForSkeleton))
		{
			MessageLog.Error(TEXT("@@ references blendspace that uses different skeleton @@"), this, BlendSpaceSkeleton);
		}
	}
}

void UAnimGraphNode_BlendSpaceEvaluator::BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	Node.GroupIndex = AnimBlueprint->FindOrAddGroup(SyncGroup.GroupName);
	Node.GroupRole = SyncGroup.GroupRole;
}

void UAnimGraphNode_BlendSpaceEvaluator::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	if (!Context.bIsDebugging)
	{
		// add an option to convert to single frame
		Context.MenuBuilder->BeginSection("AnimGraphNodeBlendSpacePlayer", NSLOCTEXT("A3Nodes", "BlendSpaceHeading", "Blend Space"));
		{
			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().OpenRelatedAsset);
			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().ConvertToBSPlayer);
		}
		Context.MenuBuilder->EndSection();
	}
}

bool UAnimGraphNode_BlendSpaceEvaluator::DoesSupportTimeForTransitionGetter() const
{
	return true;
}

UAnimationAsset* UAnimGraphNode_BlendSpaceEvaluator::GetAnimationAsset() const 
{
	return Node.BlendSpace;
}

const TCHAR* UAnimGraphNode_BlendSpaceEvaluator::GetTimePropertyName() const 
{
	return TEXT("InternalTimeAccumulator");
}

UScriptStruct* UAnimGraphNode_BlendSpaceEvaluator::GetTimePropertyStruct() const 
{
	return FAnimNode_BlendSpaceEvaluator::StaticStruct();
}

#undef LOCTEXT_NAMESPACE