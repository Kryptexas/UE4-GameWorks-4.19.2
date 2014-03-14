// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_Service.generated.h"

UCLASS()
class UBehaviorTreeGraphNode_Service : public UBehaviorTreeGraphNode
{
	GENERATED_UCLASS_BODY()

	virtual FString GetNodeTitle(ENodeTitleType::Type TitleType) const OVERRIDE;
	/** gets icon resource name for title bar */
	virtual FName GetNameIcon() const OVERRIDE;
	virtual void AllocateDefaultPins() OVERRIDE;
	virtual bool IsSubNode() const OVERRIDE;
};

