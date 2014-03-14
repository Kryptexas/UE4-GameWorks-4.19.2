// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_MeshRefPose

UAnimGraphNode_MeshRefPose::UAnimGraphNode_MeshRefPose(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_MeshRefPose::GetNodeCategory() const
{
	return TEXT("Identity");
}

FLinearColor UAnimGraphNode_MeshRefPose::GetNodeTitleColor() const
{
	return FColor(200, 100, 100);
}

FString UAnimGraphNode_MeshRefPose::GetTooltip() const
{
	return TEXT("Returns mesh space reference pose.");
}

FString UAnimGraphNode_MeshRefPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return FString::Printf(TEXT("Mesh Space Ref Pose"));
}

void UAnimGraphNode_MeshRefPose::CreateOutputPins()
{
	const UAnimationGraphSchema* Schema = GetDefault<UAnimationGraphSchema>();
	CreatePin(EGPD_Output, Schema->PC_Struct, TEXT(""), FComponentSpacePoseLink::StaticStruct(), /*bIsArray=*/ false, /*bIsReference=*/ false, TEXT("ComponentPose"));
}