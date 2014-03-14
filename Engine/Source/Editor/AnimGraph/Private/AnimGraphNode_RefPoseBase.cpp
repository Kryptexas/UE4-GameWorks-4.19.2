// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_RefPoseBase

UAnimGraphNode_RefPoseBase::UAnimGraphNode_RefPoseBase(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_RefPoseBase::GetNodeCategory() const
{
	return TEXT("Identity");
}

FLinearColor UAnimGraphNode_RefPoseBase::GetNodeTitleColor() const
{
	if ( Node.RefPoseType == EIT_Additive )
	{
		return FLinearColor(0.10f, 0.60f, 0.12f);
	}
	else
	{
		return FColor(200, 100, 100);
	}
}