// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

#include "CompilerResultsLog.h"
#include "GraphEditorActions.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_SequenceEvaluator

UAnimGraphNode_SequenceEvaluator::UAnimGraphNode_SequenceEvaluator(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UAnimGraphNode_SequenceEvaluator::PreloadRequiredAssets()
{
	PreloadObject(Node.Sequence);

	Super::PreloadRequiredAssets();
}

void UAnimGraphNode_SequenceEvaluator::GetAllAnimationSequencesReferred(TArray<UAnimationAsset*>& ComplexAnims, TArray<UAnimSequence*>& AnimationSequences) const
{
	if(Node.Sequence)
	{
		HandleAnimReferenceCollection(Node.Sequence, ComplexAnims, AnimationSequences);
	}
}

void UAnimGraphNode_SequenceEvaluator::ReplaceReferredAnimations(const TMap<UAnimationAsset*, UAnimationAsset*>& ComplexAnimsMap, const TMap<UAnimSequence*, UAnimSequence*>& AnimSequenceMap)
{
	HandleAnimReferenceReplacement(Node.Sequence, ComplexAnimsMap, AnimSequenceMap);
}

FString UAnimGraphNode_SequenceEvaluator::GetTooltip() const
{
	if ((Node.Sequence != NULL) && Node.Sequence->IsValidAdditive())
	{
		return FString::Printf(TEXT("Evaluate %s (additive)"), *(Node.Sequence->GetPathName()));
	}
	else
	{
		return FString::Printf(TEXT("Evaluate %s"), *(Node.Sequence->GetPathName()));
	}
}

FString UAnimGraphNode_SequenceEvaluator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (Node.Sequence && Node.Sequence->IsValidAdditive())
	{
		return FString::Printf(TEXT("Evaluate %s (additive)"), (Node.Sequence != NULL) ? *(Node.Sequence->GetName()) : TEXT("(None)"));
	}
	else
	{
		return FString::Printf(TEXT("Evaluate %s"), (Node.Sequence != NULL) ? *(Node.Sequence->GetName()) : TEXT("(None)"));
	}
}

void UAnimGraphNode_SequenceEvaluator::GetMenuEntries(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	// Intentionally empty; you can drop down a regular sequence player and convert into a sequence evaluator in the right-click menu.
}

void UAnimGraphNode_SequenceEvaluator::ValidateAnimNodeDuringCompilation(class USkeleton* ForSkeleton, class FCompilerResultsLog& MessageLog)
{
	if (Node.Sequence == NULL)
	{
		MessageLog.Error(TEXT("@@ references an unknown sequence"), this);
	}
	else
	{
		USkeleton * SeqSkeleton = Node.Sequence->GetSkeleton();
		if (SeqSkeleton&& // if anim sequence doesn't have skeleton, it might be due to anim sequence not loaded yet, @todo: wait with anim blueprint compilation until all assets are loaded?
			!SeqSkeleton->IsCompatible(ForSkeleton))
		{
			MessageLog.Error(TEXT("@@ references sequence that uses different skeleton @@"), this, SeqSkeleton);
		}
	}
}

void UAnimGraphNode_SequenceEvaluator::GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const
{
	if (!Context.bIsDebugging)
	{
		// add an option to convert to a regular sequence player
		Context.MenuBuilder->BeginSection("AnimGraphNodeSequenceEvaluator", NSLOCTEXT("A3Nodes", "SequenceEvaluatorHeading", "Sequence Evaluator"));
		{
			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().OpenRelatedAsset);
			Context.MenuBuilder->AddMenuEntry(FGraphEditorCommands::Get().ConvertToSeqPlayer);
		}
		Context.MenuBuilder->EndSection();
	}
}

bool UAnimGraphNode_SequenceEvaluator::DoesSupportTimeForTransitionGetter() const
{
	return true;
}

UAnimationAsset* UAnimGraphNode_SequenceEvaluator::GetAnimationAsset() const 
{
	return Node.Sequence;
}

const TCHAR* UAnimGraphNode_SequenceEvaluator::GetTimePropertyName() const 
{
	return TEXT("ExplicitTime");
}

UScriptStruct* UAnimGraphNode_SequenceEvaluator::GetTimePropertyStruct() const 
{
	return FAnimNode_SequenceEvaluator::StaticStruct();
}