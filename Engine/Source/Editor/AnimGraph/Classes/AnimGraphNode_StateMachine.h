// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AnimGraphNode_StateMachineBase.h"
#include "AnimGraphNode_StateMachine.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_StateMachine : public UAnimGraphNode_StateMachineBase
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimGraphNode_StateMachine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Runtime state machine representation (empty; populated during compilation)
	UPROPERTY(EditAnywhere, Category=Settings)
	FAnimNode_StateMachine Node;

	// UAnimGraphNode_StateMachineBase interface
	virtual FAnimNode_StateMachine& GetNode() override { return Node; }
	// End of UAnimGraphNode_StateMachineBase interface
};
