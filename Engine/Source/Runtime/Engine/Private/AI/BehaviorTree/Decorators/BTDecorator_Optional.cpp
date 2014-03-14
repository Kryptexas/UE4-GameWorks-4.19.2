// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_Optional::UBTDecorator_Optional(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = TEXT("Optional");
	bNotifyProcessed = true;

	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

void UBTDecorator_Optional::OnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult) const
{
	NodeResult = EBTNodeResult::Optional;
}
