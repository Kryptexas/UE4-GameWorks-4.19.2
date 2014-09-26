// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AITestSuitePrivatePCH.h"
#include "BehaviorTree/TestBTDecorator_CantExecute.h"

UTestBTDecorator_CantExecute::UTestBTDecorator_CantExecute(const FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = TEXT("Can't Exexcute");

	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

bool UTestBTDecorator_CantExecute::CalculateRawConditionValue(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	return false;
}
