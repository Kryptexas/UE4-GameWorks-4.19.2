// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_Optional.generated.h"

UCLASS(HideCategories=(Condition), meta=(DeprecatedNode,DeprecationMessage="Please use Force Success decorator instead."))
class UBTDecorator_Optional : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

protected:

	virtual void OnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult);
};
