// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Decorators/BTDecorator_ForceSuccess.h"

UBTDecorator_ForceSuccess::UBTDecorator_ForceSuccess(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = TEXT("Force Success");
	bNotifyProcessed = true;

	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

void UBTDecorator_ForceSuccess::OnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult)
{
	NodeResult = EBTNodeResult::Succeeded;
}
