// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "K2Node.h"
#include "K2Node_DeadClass.generated.h"

UCLASS(MinimalAPI)
class UK2Node_DeadClass : public UK2Node
{
	GENERATED_BODY()
public:
	BLUEPRINTGRAPH_API UK2Node_DeadClass(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
