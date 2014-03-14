// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_Task.generated.h"


UCLASS()
class UBehaviorTreeGraphNode_Task : public UBehaviorTreeGraphNode
{
	GENERATED_UCLASS_BODY()

	virtual void AllocateDefaultPins() OVERRIDE;
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const OVERRIDE;
	/** Gets a list of actions that can be done to this particular node */
	virtual void GetContextMenuActions(const FGraphNodeContextMenuBuilder& Context) const OVERRIDE;

	virtual bool CanPlaceBreakpoints() const OVERRIDE { return true; }
};
