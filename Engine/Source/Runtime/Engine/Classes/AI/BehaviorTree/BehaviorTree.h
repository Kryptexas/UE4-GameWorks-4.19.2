// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BehaviorTree.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogBehaviorTree, Display, All);

UCLASS(DependsOn=(UBTCompositeNode, UBTTaskNode))
class ENGINE_API UBehaviorTree : public UObject
{
	GENERATED_UCLASS_BODY()

	/** root node of loaded tree */
	UPROPERTY()
	UBTCompositeNode* RootNode;

#if WITH_EDITORONLY_DATA

	/** Graph for Behavior Tree */
	UPROPERTY()
	class UEdGraph*	BTGraph;

#endif

	/** blackboard asset for this tree */
	UPROPERTY()
	UBlackboardData* BlackboardAsset;

	/** memory size required for instance of this tree */
	uint16 InstanceMemorySize;
};
