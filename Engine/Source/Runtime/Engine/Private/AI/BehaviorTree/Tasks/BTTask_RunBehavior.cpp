// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTTask_RunBehavior::UBTTask_RunBehavior(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Run Behavior";
}

EBTNodeResult::Type UBTTask_RunBehavior::ExecuteTask(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	const bool bPushed = OwnerComp->PushInstance(BehaviorAsset);
	return bPushed ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
}

FString UBTTask_RunBehavior::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *GetNameSafe(BehaviorAsset));
}
