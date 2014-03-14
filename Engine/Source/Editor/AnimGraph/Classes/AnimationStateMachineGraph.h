// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AnimationStateMachineGraph.generated.h"

UCLASS(MinimalAPI)
class UAnimationStateMachineGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	// Entry node within the state machine
	UPROPERTY()
	class UAnimStateEntryNode* EntryNode;

	// Parent instance node
	UPROPERTY()
	class UAnimGraphNode_StateMachineBase* OwnerAnimGraphNode;

#if WITH_EDITOR
#endif
};

