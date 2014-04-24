// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_Fabrik.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_Fabrik 

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_Fabrik::UAnimGraphNode_Fabrik(const FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
}

FText UAnimGraphNode_Fabrik::GetControllerDescription() const
{
	return LOCTEXT("Fabrik", "FABRIK");
}

FText UAnimGraphNode_Fabrik::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return GetControllerDescription();
}

#undef LOCTEXT_NAMESPACE