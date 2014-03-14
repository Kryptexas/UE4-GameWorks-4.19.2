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

	// no point in waiting, since we don't care about meta data anymore
	DelayedInitialize();
}

void UBTTask_BlueprintBase::DelayedInitialize()
{
	UClass* StopAtClass = UBTTask_BlueprintBase::StaticClass();
	BlueprintNodeHelpers::CollectPropertyData(this, StopAtClass, PropertyData);

	PropertyMemorySize = BlueprintNodeHelpers::GetPropertiesMemorySize(PropertyData);
}

void UBTTask_BlueprintBase::PostInitProperties()
{
	Super::PostInitProperties();
	NodeName = BlueprintNodeHelpers::GetNodeName(this);
}

EBTNodeResult::Type UBTTask_BlueprintBase::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	CurrentCallResult = bImplementsReceiveExecute ? EBTNodeResult::InProgress : EBTNodeResult::Failed;
	if (bImplementsReceiveExecute)
	{
		CurrentCallOwner = OwnerComp;
		bStoreFinishResult = true;

		// can't use const functions with blueprints
		UBTTask_BlueprintBase* MyNode = (UBTTask_BlueprintBase*)this;

		MyNode->CopyPropertiesFromMemory(NodeMemory);
		MyNode->ReceiveExecute(CurrentCallOwner->GetOwner());
		CopyPropertiesToMemory(NodeMemory);

		CurrentCallOwner = NULL;
		bStoreFinishResult = false;
	}

	return CurrentCallResult;
}

EBTNodeResult::Type UBTTask_BlueprintBase::AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	CurrentCallResult = bImplementsReceiveAbort ? EBTNodeResult::InProgress : EBTNodeResult::Aborted;
	if (bImplementsReceiveAbort)
	{
		CurrentCallOwner = OwnerComp;
		bStoreFinishResult = true;

		// can't use const functions with blueprints
		UBTTask_BlueprintBase* MyNode = (UBTTask_BlueprintBase*)this;

		MyNode->CopyPropertiesFromMemory(NodeMemory);
		MyNode->ReceiveAbort(CurrentCallOwner->GetOwner());
		CopyPropertiesToMemory(NodeMemory);

		CurrentCallOwner = NULL;
		bStoreFinishResult = false;
	}

	return CurrentCallResult;
}

void UBTTask_BlueprintBase::TickTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	// skip flag, will be handled by bNotifyTick

	// can't use const functions with blueprints
	UBTTask_BlueprintBase* MyNode = (UBTTask_BlueprintBase*)this;
	CurrentCallOwner = OwnerComp;

	MyNode->CopyPropertiesFromMemory(NodeMemory);
	MyNode->ReceiveTick(CurrentCallOwner->GetOwner(), DeltaSeconds);
	CopyPropertiesToMemory(NodeMemory);

	CurrentCallOwner = NULL;
}

void UBTTask_BlueprintBase::FinishExecute(UBTTask_BlueprintBase* NodeOwner, bool bSuccess)
{
	if (NodeOwner && NodeOwner->CurrentCallOwner)
	{
		EBTNodeResult::Type NodeResult(bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
		if (NodeOwner->bStoreFinishResult)
		{
			NodeOwner->CurrentCallResult = NodeResult;
		}
		else
		{
			NodeOwner->FinishLatentTask(NodeOwner->CurrentCallOwner, NodeResult);
		}
	}
}

void UBTTask_BlueprintBase::FinishAbort(UBTTask_BlueprintBase* NodeOwner)
{
	if (NodeOwner && NodeOwner->CurrentCallOwner)
	{
		EBTNodeResult::Type NodeResult(EBTNodeResult::Aborted);
		if (NodeOwner->bStoreFinishResult)
		{
			NodeOwner->CurrentCallResult = NodeResult;
		}
		else
		{
			NodeOwner->FinishLatentAbort(NodeOwner->CurrentCallOwner);
		}
	}
}

void UBTTask_BlueprintBase::SetFinishOnMessage(UBTTask_BlueprintBase* NodeOwner, FName MessageName)
{
	if (NodeOwner && NodeOwner->CurrentCallOwner)
	{
		NodeOwner->CurrentCallOwner->RegisterMessageObserver(NodeOwner, MessageName);
	}
}

void UBTTask_BlueprintBase::SetFinishOnMessageWithId(UBTTask_BlueprintBase* NodeOwner, FName MessageName, int32 RequestID)
{
	if (NodeOwner && NodeOwner->CurrentCallOwner)
	{
		NodeOwner->CurrentCallOwner->RegisterMessageObserver(NodeOwner, MessageName, RequestID);
	}
}

uint16 UBTTask_BlueprintBase::GetInstanceMemorySize() const
{
	return Super::GetInstanceMemorySize() + PropertyMemorySize;
}

void UBTTask_BlueprintBase::CopyPropertiesToMemory(uint8* NodeMemory) const
{
	if (PropertyMemorySize > 0)
	{
		BlueprintNodeHelpers::CopyPropertiesToContext(PropertyData, (uint8*)this, NodeMemory + Super::GetInstanceMemorySize());
	}
}

void UBTTask_BlueprintBase::CopyPropertiesFromMemory(const uint8* NodeMemory)
{
	if (PropertyMemorySize > 0)
	{
		BlueprintNodeHelpers::CopyPropertiesFromContext(PropertyData, (uint8*)this, (uint8*)NodeMemory + Super::GetInstanceMemorySize());
	}
}

void UBTTask_BlueprintBase::StartUsingExternalEvent(AActor* OwningActor)
{
	int32 InstanceIdx = INDEX_NONE;

	const bool bFound = BlueprintNodeHelpers::FindNodeOwner(OwningActor, this, CurrentCallOwner, InstanceIdx);
	if (bFound)
	{
		uint8* NodeMemory = CurrentCallOwner->GetNodeMemory(this, InstanceIdx);
		if (NodeMemory)
		{
			CopyPropertiesFromMemory(NodeMemory);
		}
	}
	else
	{
		UE_VLOG(OwningActor, LogBehaviorTree, Error, TEXT("Unable to find owning behavior tree for StartUsingExternalEvent!"));
	}
}

void UBTTask_BlueprintBase::StopUsingExternalEvent()
{
	if (CurrentCallOwner != NULL)
	{
		const int32 InstanceIdx = CurrentCallOwner->FindInstanceContainingNode(this);
		uint8* NodeMemory = CurrentCallOwner->GetNodeMemory(this, InstanceIdx);

		if (NodeMemory)
		{
			CopyPropertiesToMemory(NodeMemory);
		}
			
		CurrentCallOwner = NULL;
	}
}

FString UBTTask_BlueprintBase::GetStaticDescription() const
{
	FString ReturnDesc = Super::GetStaticDescription();
	if (bShowPropertyDetails)
	{
		UClass* StopAtClass = UBTTask_BlueprintBase::StaticClass();
		FString PropertyDesc = BlueprintNodeHelpers::CollectPropertyDescription(this, StopAtClass, PropertyData);
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
	if (PropertyMemorySize > 0)
	{
		UClass* StopAtClass = UBTTask_BlueprintBase::StaticClass();
		BlueprintNodeHelpers::DescribeRuntimeValues(this, StopAtClass, PropertyData, NodeMemory + Super::GetInstanceMemorySize(), Verbosity, Values);
	}
}
