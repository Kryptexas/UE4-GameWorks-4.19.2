// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_Cooldown::UBTDecorator_Cooldown(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Cooldown";
	CoolDownTime = 5.0f;
	
	// aborting child nodes doesn't makes sense, cooldown starts after leaving this branch
	bAllowAbortNone = false;
	bAllowAbortLowerPri = false;
	bAllowAbortChildNodes = false;

	bNotifyTick = false;
	bNotifyDeactivation = true;
}

void UBTDecorator_Cooldown::PostLoad()
{
	Super::PostLoad();
	bNotifyTick = (FlowAbortMode != EBTFlowAbortMode::None);
}

bool UBTDecorator_Cooldown::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const 
{
	FBTCooldownDecoratorMemory* DecoratorMemory = (FBTCooldownDecoratorMemory*)NodeMemory;
	const float TimePassed = (OwnerComp->GetWorld()->GetTimeSeconds() - DecoratorMemory->LastUseTimestamp);
	return TimePassed >= CoolDownTime;
}

void UBTDecorator_Cooldown::InitializeMemory(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	FBTCooldownDecoratorMemory* DecoratorMemory = (FBTCooldownDecoratorMemory*)NodeMemory;
	DecoratorMemory->LastUseTimestamp = -FLT_MAX;
}

void UBTDecorator_Cooldown::OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) const
{
	FBTCooldownDecoratorMemory* DecoratorMemory = GetNodeMemory<FBTCooldownDecoratorMemory>(SearchData);
	DecoratorMemory->LastUseTimestamp = SearchData.OwnerComp->GetWorld()->GetTimeSeconds();
	DecoratorMemory->RequestedRestart = false;
}

void UBTDecorator_Cooldown::TickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	FBTCooldownDecoratorMemory* DecoratorMemory = (FBTCooldownDecoratorMemory*)NodeMemory;
	if (!DecoratorMemory->RequestedRestart)
	{
		const float TimePassed = (OwnerComp->GetWorld()->GetTimeSeconds() - DecoratorMemory->LastUseTimestamp);
		if (TimePassed >= CoolDownTime)
		{
			DecoratorMemory->RequestedRestart = true;
			OwnerComp->RequestExecution(this);
		}
	}
}

FString UBTDecorator_Cooldown::GetStaticDescription() const
{
	// basic info: result after time
	return FString::Printf(TEXT("%s: lock for %.1fs after execution and return %s"), *Super::GetStaticDescription(),
		CoolDownTime, *UBehaviorTreeTypes::DescribeNodeResult(EBTNodeResult::Failed));
}

void UBTDecorator_Cooldown::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	FBTCooldownDecoratorMemory* DecoratorMemory = (FBTCooldownDecoratorMemory*)NodeMemory;
	const float TimePassed = OwnerComp->GetWorld()->GetTimeSeconds() - DecoratorMemory->LastUseTimestamp;
	
	if (TimePassed < CoolDownTime)
	{
		Values.Add(FString::Printf(TEXT("%s in %ss"),
			(FlowAbortMode == EBTFlowAbortMode::None) ? TEXT("unlock") : TEXT("restart"),
			*FString::SanitizeFloat(CoolDownTime - TimePassed)));
	}
}

uint16 UBTDecorator_Cooldown::GetInstanceMemorySize() const
{
	return sizeof(FBTCooldownDecoratorMemory);
}
