// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphNode_Constraint.h"
#include "CompilerResultsLog.h"
#include "AnimNodeEditModes.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_Constraint

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_Constraint::UAnimGraphNode_Constraint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAnimGraphNode_Constraint::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.BoneToModify.BoneName) == INDEX_NONE)
	{
		if (Node.BoneToModify.BoneName == NAME_None)
		{
			MessageLog.Warning(*LOCTEXT("NoBoneSelectedToModify", "@@ - You must pick a bone to modify").ToString(), this);
		}
		else
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneToModify.BoneName));

			FText Msg = FText::Format(LOCTEXT("NoBoneFoundToModify", "@@ - Bone {BoneName} not found in Skeleton"), Args);

			MessageLog.Warning(*Msg.ToString(), this);
		}
	}

	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

FText UAnimGraphNode_Constraint::GetControllerDescription() const
{
	return LOCTEXT("Constraint", "Constraint");
}

FText UAnimGraphNode_Constraint::GetTooltipText() const
{
	return LOCTEXT("AnimGraphNode_Constraint_Tooltip", "Constraint to another joint per transform component");
}

FText UAnimGraphNode_Constraint::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle) && (Node.BoneToModify.BoneName == NAME_None))
	{
		return GetControllerDescription();
	}
	else 
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
		Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneToModify.BoneName));

		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_Constraint_ListTitle", "{ControllerDescription} - {BoneName}"), Args), this);
	}
	return CachedNodeTitles[TitleType];
}

void UAnimGraphNode_Constraint::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{
	if (SkelMeshComp)
	{
		if (FAnimNode_Constraint* ActiveNode = GetActiveInstanceNode<FAnimNode_Constraint>(SkelMeshComp->GetAnimInstance()))
		{
			ActiveNode->ConditionalDebugDraw(PDI, SkelMeshComp);
		}
	}
}

void UAnimGraphNode_Constraint::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		static const FName NAME_ConstraintSetup = GET_MEMBER_NAME_CHECKED(FAnimNode_Constraint, ConstraintSetup);
		const FName PropName = PropertyChangedEvent.Property->GetFName();

		if (PropName == NAME_ConstraintSetup)
		{
			int32 CurrentNum = Node.ConstraintWeights.Num();
			Node.ConstraintWeights.SetNumZeroed(Node.ConstraintSetup.Num());
			int32 NewNum = Node.ConstraintWeights.Num();

			// we want to fill up 1, not 0. 
			for (int32 Index = CurrentNum; Index < NewNum; ++Index)
			{
				Node.ConstraintWeights[Index] = 1.f;
			}

			ReconstructNode();
		}
	}
}
#undef LOCTEXT_NAMESPACE
