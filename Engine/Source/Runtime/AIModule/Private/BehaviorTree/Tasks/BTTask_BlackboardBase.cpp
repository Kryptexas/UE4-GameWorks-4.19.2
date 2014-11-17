// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"

UBTTask_BlackboardBase::UBTTask_BlackboardBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "BlackboardBase";

	// empty KeySelector = allow everything
}

void UBTTask_BlackboardBase::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);
	BlackboardKey.CacheSelectedKey(GetBlackboardAsset());
}
