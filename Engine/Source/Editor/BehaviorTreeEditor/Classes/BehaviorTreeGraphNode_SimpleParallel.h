// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTreeGraphNode_SimpleParallel.generated.h"

UCLASS()
class UBehaviorTreeGraphNode_SimpleParallel : public UBehaviorTreeGraphNode_Composite
{
	GENERATED_BODY()
public:
	UBehaviorTreeGraphNode_SimpleParallel(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void AllocateDefaultPins() override;
	virtual void GetPinHoverText(const UEdGraphPin& Pin, FString& HoverTextOut) const override;
};
