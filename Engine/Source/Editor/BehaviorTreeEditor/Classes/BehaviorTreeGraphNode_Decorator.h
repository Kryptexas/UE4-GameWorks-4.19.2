// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_Decorator.generated.h"

UCLASS()
class UBehaviorTreeGraphNode_Decorator : public UBehaviorTreeGraphNode
{
	GENERATED_UCLASS_BODY()

	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	void CollectDecoratorData(TArray<class UBTDecorator*>& NodeInstances, TArray<struct FBTDecoratorLogic>& Operations) const;

	virtual bool IsSubNode() const OVERRIDE;
};
