// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../BlueprintNodeHelpers.h"

UBTTask_BlueprintBase::UBTTask_BlueprintBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	UClass* StopAtClass = UBTTask_BlueprintBase::StaticClass();
	bImplementsReceiveTick = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveTick"), this, StopAtClass);
	bImplementsReceiveExecute = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveExecute"), this, StopAtClass);
	bImplementsReceiveAbort = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveAbort"), this, StopAtClass);

	bNotifyTick = bImplementsReceiveTick;
	bShowPropertyDetails = true;

	// all blueprint based nodes must create instances
	bCreateNodeInstance = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		BlueprintNodeHelpers::CollectPropertyData(this, StopAtClass, PropertyData);
	}
}

void UBTTask_BlueprintBase::PostInitProperties()
{
	Super::PostInitProperties();
	NodeName = BlueprintNodeHelpers::GetNodeName(this);
}

EBTNodeResult::Type UBTTask_BlueprintBase::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	CurrentCallResult = bImplementsReceiveExecute ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
	if (bImplementsReceiveExecute)
	{
		bStoreFinishResult = true;

		ReceiveExecute(OwnerComp->GetOwner());

		bStoreFinishResult = false;
	}

	return CurrentCallResult;
}

EBTNodeResult::Type UBTTask_BlueprintBase::AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	CurrentCallResult = bImplementsReceiveAbort ? EBTNodeResult::InProgress : EBTNodeResult::Aborted;
	if (bImplementsReceiveAbort)
	{
		bStoreFinishResult = true;

		ReceiveAbort(OwnerComp->GetOwner());

		bStoreFinishResult = false;
	}

	return CurrentCallResult;
}

void UBTTask_BlueprintBase::TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) 
{
	// skip flag, will be handled by bNotifyTick

	ReceiveTick(OwnerComp->GetOwner(), DeltaSeconds);
}

void UBTTask_BlueprintBase::FinishExecute(bool bSuccess)
{
	UBehaviorTreeComponent* OwnerComp = Cast<UBehaviorTreeComponent>(GetOuter());
	EBTNodeResult::Type NodeResult(bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);

	if (bStoreFinishResult)
	{
		CurrentCallResult = NodeResult;
	}
	else if (OwnerComp)
	{
		FinishLatentTask(OwnerComp, NodeResult);
	}
}

void UBTTask_BlueprintBase::FinishAbort()
{
	UBehaviorTreeComponent* OwnerComp = Cast<UBehaviorTreeComponent>(GetOuter());
	EBTNodeResult::Type NodeResult(EBTNodeResult::Aborted);

	if (bStoreFinishResult)
	{
		CurrentCallResult = NodeResult;
	}
	else if (OwnerComp)
	{
		FinishLatentAbort(OwnerComp);
	}
}

void UBTTask_BlueprintBase::SetFinishOnMessage(FName MessageName)
{
	UBehaviorTreeComponent* OwnerComp = Cast<UBehaviorTreeComponent>(GetOuter());
	if (OwnerComp)
	{
		OwnerComp->RegisterMessageObserver(this, MessageName);
	}
}

void UBTTask_BlueprintBase::SetFinishOnMessageWithId(FName MessageName, int32 RequestID)
{
	UBehaviorTreeComponent* OwnerComp = Cast<UBehaviorTreeComponent>(GetOuter());
	if (OwnerComp)
	{
		OwnerComp->RegisterMessageObserver(this, MessageName, RequestID);
	}
}

FString UBTTask_BlueprintBase::GetStaticDescription() const
{
	FString ReturnDesc = Super::GetStaticDescription();

	UBTTask_BlueprintBase* CDO = (UBTTask_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (bShowPropertyDetails && CDO)
	{
		UClass* StopAtClass = UBTTask_BlueprintBase::StaticClass();
		FString PropertyDesc = BlueprintNodeHelpers::CollectPropertyDescription(this, StopAtClass, CDO->PropertyData);
		if (PropertyDesc.Len())
		{
			ReturnDesc += TEXT(":\n\n");
			ReturnDesc += PropertyDesc;
		}
	}

	return ReturnDesc;
}

void UBTTask_BlueprintBase::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	UBTTask_BlueprintBase* CDO = (UBTTask_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (CDO && CDO->PropertyData.Num())
	{
		BlueprintNodeHelpers::DescribeRuntimeValues(this, CDO->PropertyData, Values);
	}
}
