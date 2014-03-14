// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_SimpleParallel.generated.h"

UCLASS()
class UBehaviorTreeGraphNode_SimpleParallel : public UBehaviorTreeGraphNode_Composite
{
	GENERATED_UCLASS_BODY()

	virtual void AllocateDefaultPins() OVERRIDE;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const OVERRIDE;
};
