// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTDecorator_Optional.generated.h"

/**
 *  Override result of execution, so parent composite will always move to next child
 */

UCLASS(HideCategories=(Condition))
class UBTDecorator_Optional : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

protected:

	virtual void OnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const;
};
