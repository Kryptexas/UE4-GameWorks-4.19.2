// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTComposite_Selector::UBTComposite_Selector(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Selector";

	OptionalDeactivationResult = EBTNodeResult::Failed;
	OnNextChild.BindUObject(this, &UBTComposite_Selector::GetNextChildHandler);
}

int32 UBTComposite_Selector::GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const
{
	// success = quit
	int32 NextChildIdx = BTSpecialChild::ReturnToParent;

	if (PrevChild == BTSpecialChild::NotInitialized)
	{
		// newly activated: start from first
		NextChildIdx = 0;
	}
	else if ((LastResult == EBTNodeResult::Failed || LastResult == EBTNodeResult::Optional) && (PrevChild + 1) < GetChildrenNum())
	{
		// failed = choose next child
		NextChildIdx = PrevChild + 1;
	}

	return NextChildIdx;
}
