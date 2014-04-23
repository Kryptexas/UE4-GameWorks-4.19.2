// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_Fabrik 

UAnimGraphNode_Fabrik::UAnimGraphNode_Fabrik(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

FString UAnimGraphNode_Fabrik::GetControllerDescription() const
{
	return TEXT("Fabrik IK");
}

FString UAnimGraphNode_Fabrik::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = *GetControllerDescription();
	return Result;
}