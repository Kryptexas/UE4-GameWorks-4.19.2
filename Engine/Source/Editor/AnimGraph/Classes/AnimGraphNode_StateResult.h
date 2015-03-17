// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "AnimGraphNode_Root.h"
#include "AnimGraphNode_StateResult.generated.h"

UCLASS(MinimalAPI)
class UAnimGraphNode_StateResult : public UAnimGraphNode_Root
{
	GENERATED_BODY()
public:
	ANIMGRAPH_API UAnimGraphNode_StateResult(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
