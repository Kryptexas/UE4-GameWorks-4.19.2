// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BTComposite_Selector.generated.h"

UCLASS()
class UBTComposite_Selector : public UBTCompositeNode
{
	GENERATED_UCLASS_BODY()

	int32 GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const;
};
