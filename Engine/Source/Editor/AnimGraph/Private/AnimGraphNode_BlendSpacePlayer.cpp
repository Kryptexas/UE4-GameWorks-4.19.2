// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "GraphEditorActions.h"
#include "CompilerResultsLog.h"
#include "AnimGraphNode_BlendSpacePlayer.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabaseRegistrar.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_BlendSpacePlayer

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_BlendSpacePlayer::UAnimGraphNode_BlendSpacePlayer(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_BlendSpacePlayer::GetTooltip() const
{
	return FString::Printf(TEXT("Blendspace Player '%s'"), (Node.BlendSpace != NULL) ? *(Node.BlendSpace->GetPathName()) : TEXT("(None)"));
}

FText UAnimGraphNode_BlendSpacePlayer::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	const FText BlendSpaceName((Node.BlendSpace != NULL) ? FText::FromString(Node.BlendSpace->GetName()) : LOCTEXT("None", "(None)"));
	
	if (TitleType == ENodeTitleType::ListView)
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("BlendSpaceName"), BlendSpaceName);
		return FText::Format(LOCTEXT("BlendspacePlayer", "Blendspace Player'{BlendSpaceName}'"), Args);
	}
	else
	{
		FFormatNamedArguments TitleArgs;
		TitleArgs.Add(TEXT("BlendSpaceName"), BlendSpaceName);
		FText Title = FText::Format(LOCTEXT("BlendSpacePlayerFullTitle", "{BlendSpaceName}\nBlendspace Player"), TitleArgs);

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

void UAnimGraphNode_BlendSpacePlayer::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	const bool bWantAimOffsets = false;
	GetBlendSpaceEntries(bWantAimOffsets, ContextMenuBuilder);
}

void UAnimGraphNode_BlendSpacePlayer::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	auto PostSpawnSetupLambda = [](UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, TWeakObjectPtr<UBlendSpaceBase> BlendSpace)
	{
		UAnimGraphNode_BlendSpacePlayer* BlendSpaceNode = CastChecked<UAnimGraphNode_BlendSpacePlayer>(NewNode);
		BlendSpaceNode->Node.BlendSpace = BlendSpace.Get();		
	};

	for (TObjectIterator<UBlendSpaceBase> BlendSpaceIt; BlendSpaceIt; ++BlendSpaceIt)
	{
		UBlendSpaceBase* BlendSpace = *BlendSpaceIt;

		bool const bIsAimOffset = BlendSpace->IsA(UAimOffsetBlendSpace::StaticClass()) || 
			BlendSpace->IsA(UAimOffsetBlendSpace1D::StaticClass());
		if (!bIsAimOffset)
		{
			UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(GetClass());
			check(NodeSpawner != nullptr);
			// @TODO: Need to file this one under the BlendSpace asset (so it can be cleared/updated when the asset is)
			ActionRegistrar.AddBlueprintAction(NodeSpawner);

			TWeakObjectPtr<UBlendSpaceBase> BlendSpacePtr = BlendSpace;
			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(PostSpawnSetupLambda, BlendSpacePtr);
		}
	}
}

void UAnimGraphNode_BlendSpacePlayer::ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog)
{
	if (Node.BlendSpace == NULL)
	{
		MessageLog.Error(TEXT("@@ references an unknown blend space"), this);
	}
	else
	{
		USkeleton * BlendSpaceSkeleton = Node.BlendSpace->GetSkeleton();
		if (BlendSpaceSkeleton && // if blend space doesn't have skeleton, it might be due to blend space not loaded yet, @todo: wait with anim blueprint compilation until all assets are loaded?
			!BlendSpaceSkeleton->IsCompatible(ForSkeleton))
		{
			MessageLog.Error(TEXT("@@ references blendspace that uses different skeleton @@"), this, BlendSpaceSkeleton);
		}
	}
}

void UAnimGraphNode_BlendSpacePlayer::BakeDataDuringCompilation(class FCompilerResultsLog& MessageLog)
{
	UAnimBlueprint* AnimBlueprint = GetAnimBlueprint();
	Node.GroupIndex = AnimBlueprint->FindOrAddGroup(SyncGroup.GroupName);
	Node.GroupRole = SyncGroup.GroupRole;
}

void UAnimGraphNode_BlendSpacePlayer::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	if (!Context.bIsDebugging)
	{
		// add an option to convert to single frame
		Context.MenuBuilder->BeginSection("AnimGraphNodeBlendSpaceEvaluator", NSLOCTEXT("A3Nodes", "BlendSpaceHeading", "Blend Space"));
		{
			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().OpenRelatedAsset);
			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().ConvertToBSEvaluator);
		}
		Context.MenuBuilder->EndSection();
	}
}

void UAnimGraphNode_BlendSpacePlayer::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& ComplexAnims, TArray<UAnimSequence*>& AnimationSequences) const
{
	if(Node.BlendSpace)
	{
		HandleAnimReferenceCollection(Node.BlendSpace, ComplexAnims, AnimationSequences);
	}
}

void UAnimGraphNode_BlendSpacePlayer::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ComplexAnimsMap, const TMap<UAnimSequence*, UAnimSequence*>& AnimSequenceMap)
{
	HandleAnimReferenceReplacement(Node.BlendSpace, ComplexAnimsMap, AnimSequenceMap);
}

bool UAnimGraphNode_BlendSpacePlayer::DoesSupportTimeForTransitionGetter() const
{
	return true;
}

UAnimationAsset* UAnimGraphNode_BlendSpacePlayer::GetAnimationAsset() const 
{
	return Node.BlendSpace;
}

const TCHAR* UAnimGraphNode_BlendSpacePlayer::GetTimePropertyName() const 
{
	return TEXT("InternalTimeAccumulator");
}

UScriptStruct* UAnimGraphNode_BlendSpacePlayer::GetTimePropertyStruct() const 
{
	return FAnimNode_BlendSpacePlayer::StaticStruct();
}

#undef LOCTEXT_NAMESPACE

