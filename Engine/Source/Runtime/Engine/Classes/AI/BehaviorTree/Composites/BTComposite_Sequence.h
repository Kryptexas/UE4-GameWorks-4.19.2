// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "BTComposite_Sequence.generated.h"

UCLASS()
class UBTComposite_Sequence : public UBTCompositeNode
{
	GENERATED_UCLASS_BODY()

	int32 GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const;

#if WITH_EDITOR
	virtual bool CanAbortLowerPriority() const OVERRIDE;
#endif
};
