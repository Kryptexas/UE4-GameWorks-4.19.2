// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTComposite_Sequence::UBTComposite_Sequence(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Sequence";
	OptionalDeactivationResult = EBTNodeResult::Succeeded;

	OnNextChild.BindUObject(this, &UBTComposite_Sequence::GetNextChildHandler);
}

int32 UBTComposite_Sequence::GetNextChildHandler(struct FBehaviorTreeSearchData& SearchData, int32 PrevChild, EBTNodeResult::Type LastResult) const
{
	// failure = quit
	int32 NextChildIdx = BTSpecialChild::ReturnToParent;

	if (PrevChild == BTSpecialChild::NotInitialized)
	{
		// newly activated: start from first
		NextChildIdx = 0;
	}
	else if ((LastResult == EBTNodeResult::Succeeded || LastResult == EBTNodeResult::Optional) && (PrevChild + 1) < GetChildrenNum())
	{
		// success = choose next child
		NextChildIdx = PrevChild + 1;
	}

	return NextChildIdx;
}

#if WITH_EDITOR

bool UBTComposite_Sequence::CanAbortLowerPriority() const
{
	// don't allow aborting lower priorities, as it breaks sequence order and doesn't makes sense
	return false;
}

#endif
