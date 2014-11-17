// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Decorators/BTDecorator_Optional.h"

UBTDecorator_Optional::UBTDecorator_Optional(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = TEXT("Optional");
	bNotifyProcessed = true;

	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;
}

void UBTDecorator_Optional::OnNodeProcessed(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type& NodeResult)
{
	NodeResult = EBTNodeResult::Optional;
}

#if WITH_EDITOR

FName UBTDecorator_Optional::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Optional.Icon");
}

#endif	// WITH_EDITOR