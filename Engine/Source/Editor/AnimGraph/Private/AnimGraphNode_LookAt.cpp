// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_LookAt.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_LookAt

#define LOCTEXT_NAMESPACE "AnimGraph_LookAt"

UAnimGraphNode_LookAt::UAnimGraphNode_LookAt(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FText UAnimGraphNode_LookAt::GetControllerDescription() const
{
	return LOCTEXT("LookAtNode", "Look At");
}

FString UAnimGraphNode_LookAt::GetKeywords() const
{
	return TEXT("Look At, Follow, Trace, Track");
}

FString UAnimGraphNode_LookAt::GetTooltip() const
{
	return LOCTEXT("AnimGraphNode_LookAt_Tooltip", "This node allow a bone to trace or follow another bone").ToString();
}

FText UAnimGraphNode_LookAt::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDescription"), GetControllerDescription());
	Args.Add(TEXT("BoneName"), FText::FromName(Node.BoneToModify.BoneName));

	if(TitleType == ENodeTitleType::ListView)
	{
		return FText::Format(LOCTEXT("AnimGraphNode_LookAt_Title", "{ControllerDescription} - Bone: {BoneName}"), Args);
	}
	else
	{
		return FText::Format(LOCTEXT("AnimGraphNode_LookAt_Title", "{ControllerDescription}\nBone: {BoneName}"), Args);
	}
}

void UAnimGraphNode_LookAt::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{
	// this is not accurate debugging
	// since i can't access the transient data of run-time instance, I have to calculate data from other way
	// this technically means a frame delay, but it would be sufficient for debugging purpose. 
	FVector BoneLocation = SkelMeshComp->GetSocketLocation(Node.BoneToModify.BoneName);
	FVector TargetLocation = (Node.LookAtBone.BoneName != NAME_None)? SkelMeshComp->GetSocketLocation(Node.LookAtBone.BoneName) : Node.LookAtLocation;

	DrawWireStar(PDI, TargetLocation, 5.f, FLinearColor(1, 0, 0), SDPG_Foreground);
	DrawDashedLine(PDI, BoneLocation, TargetLocation, FLinearColor(1, 1, 0), 2.f, SDPG_Foreground);
}

#undef LOCTEXT_NAMESPACE
