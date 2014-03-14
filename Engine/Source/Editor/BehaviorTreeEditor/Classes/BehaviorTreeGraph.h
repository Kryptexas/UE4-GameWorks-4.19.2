// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraph.generated.h"

UCLASS()
class UBehaviorTreeGraph : public UEdGraph
{
	GENERATED_UCLASS_BODY()

	enum EDebuggerFlags
	{
		ClearDebuggerFlags,
		SkipDebuggerFlags,
	};

	void UpdateBlackboardChange();
	void UpdateAsset(EDebuggerFlags DebuggerFlags);
	void UpdateAbortHighlight(struct FAbortDrawHelper& Mode0, struct FAbortDrawHelper& Mode1);
	void CreateBTFromGraph(class UBehaviorTreeGraphNode* RootEdNode);
	void UpdatePinConnectionTypes();
};

