// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_Root.generated.h"

UCLASS()
class UBehaviorTreeGraphNode_Root : public UBehaviorTreeGraphNode
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=BehaviorTree)
	class UBlackboardData* BlackboardAsset;

	virtual void PostPlacedNewNode() OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual bool CanDuplicateNode() const OVERRIDE { return false; }
	virtual bool CanUserDeleteNode() const OVERRIDE { return false; }
	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const OVERRIDE;

	virtual void PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual FString	GetDescription() const OVERRIDE;

	/** notify behavior tree about blackboard change */
	void UpdateBlackboard();
};
