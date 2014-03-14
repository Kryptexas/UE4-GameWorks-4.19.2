// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTService_BlackboardBase::UBTService_BlackboardBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "BlackboardBase";

	// empty KeySelector = allow everything
}

void UBTService_BlackboardBase::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);
	BlackboardKey.CacheSelectedKey(GetBlackboardAsset());
}
