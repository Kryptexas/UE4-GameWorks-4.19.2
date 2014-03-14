// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_Loop::UBTDecorator_Loop(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Loop";
	NumLoops = 3;
	bNotifyActivation = true;
	
	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

void UBTDecorator_Loop::OnNodeActivation(FBehaviorTreeSearchData& SearchData) const
{
	FBTLoopDecoratorMemory* DecoratorMemory = GetNodeMemory<FBTLoopDecoratorMemory>(SearchData);
	FBTCompositeMemory* ParentMemory = GetParentNode()->GetNodeMemory<FBTCompositeMemory>(SearchData);
	if (ParentMemory->CurrentChild != ChildIndex)
	{
		// initialize counter if it's first activation
		DecoratorMemory->RemainingExecutions = NumLoops;
	}

	if (!bInfiniteLoop)
	{
		DecoratorMemory->RemainingExecutions--;
	}

	// set child selection overrides
	const bool bShouldLoop = bInfiniteLoop || (DecoratorMemory->RemainingExecutions > 0);
	if (bShouldLoop)
	{
		GetParentNode()->SetChildOverride(SearchData, ChildIndex);
	}
}

FString UBTDecorator_Loop::GetStaticDescription() const
{
	// basic info: infinite / num loops
	return bInfiniteLoop ? 
		FString::Printf(TEXT("%s: infinite"), *Super::GetStaticDescription()) :
		FString::Printf(TEXT("%s: %d loops"), *Super::GetStaticDescription(), NumLoops);
}

void UBTDecorator_Loop::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	if (!bInfiniteLoop)
	{
		FBTLoopDecoratorMemory* DecoratorMemory = (FBTLoopDecoratorMemory*)NodeMemory;
		Values.Add(FString::Printf(TEXT("remaining: %d"), DecoratorMemory->RemainingExecutions));
	}
}

uint16 UBTDecorator_Loop::GetInstanceMemorySize() const
{
	return sizeof(FBTLoopDecoratorMemory);
}
