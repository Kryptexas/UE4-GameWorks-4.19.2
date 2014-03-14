// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../BlueprintNodeHelpers.h"

UBTService_BlueprintBase::UBTService_BlueprintBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	UClass* StopAtClass = UBTService_BlueprintBase::StaticClass();
	bImplementsReceiveTick = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveTick"), this, StopAtClass);
	bImplementsReceiveActivation = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveActivation"), this, StopAtClass);
	bImplementsReceiveDeactivation = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveDeactivation"), this, StopAtClass);

	bNotifyBecomeRelevant = bImplementsReceiveActivation || bImplementsReceiveTick;
	bNotifyCeaseRelevant = bImplementsReceiveDeactivation;
	bNotifyTick = bImplementsReceiveTick;
	bShowPropertyDetails = true;

	// no point in waiting, since we don't care about meta data anymore
	DelayedInitialize();
}

void UBTService_BlueprintBase::DelayedInitialize()
{
	UClass* StopAtClass = UBTService_BlueprintBase::StaticClass();
	BlueprintNodeHelpers::CollectPropertyData(this, StopAtClass, PropertyData);

	PropertyMemorySize = BlueprintNodeHelpers::GetPropertiesMemorySize(PropertyData);
}

void UBTService_BlueprintBase::PostInitProperties()
{
	Super::PostInitProperties();
	NodeName = BlueprintNodeHelpers::GetNodeName(this);
}

void UBTService_BlueprintBase::OnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// check flag, it could be used because user wants tick
	if (bImplementsReceiveActivation)
	{
		CurrentCallOwner = OwnerComp;

		// can't use const functions with blueprints
		UBTService_BlueprintBase* MyNode = (UBTService_BlueprintBase*)this;

		MyNode->CopyPropertiesFromMemory(NodeMemory);
		MyNode->ReceiveActivation(CurrentCallOwner->GetOwner());
		CopyPropertiesToMemory(NodeMemory);

		CurrentCallOwner = NULL;
	}
}

void UBTService_BlueprintBase::OnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	// skip flag, will be handled by bNotifyCeaseRelevant

	// can't use const functions with blueprints
	UBTService_BlueprintBase* MyNode = (UBTService_BlueprintBase*)this;
	CurrentCallOwner = OwnerComp;

	MyNode->CopyPropertiesFromMemory(NodeMemory);
	MyNode->ReceiveDeactivation(CurrentCallOwner->GetOwner());
	CopyPropertiesToMemory(NodeMemory);

	CurrentCallOwner = NULL;
}

void UBTService_BlueprintBase::TickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// skip flag, will be handled by bNotifyTick

	// can't use const functions with blueprints
	UBTService_BlueprintBase* MyNode = (UBTService_BlueprintBase*)this;
	CurrentCallOwner = OwnerComp;

	MyNode->CopyPropertiesFromMemory(NodeMemory);
	MyNode->ReceiveTick(CurrentCallOwner->GetOwner(), DeltaSeconds);
	CopyPropertiesToMemory(NodeMemory);

	CurrentCallOwner = NULL;
}

uint16 UBTService_BlueprintBase::GetInstanceMemorySize() const
{
	return Super::GetInstanceMemorySize() + PropertyMemorySize;
}

void UBTService_BlueprintBase::CopyPropertiesToMemory(uint8* NodeMemory) const
{
	if (PropertyMemorySize > 0)
	{
		BlueprintNodeHelpers::CopyPropertiesToContext(PropertyData, (uint8*)this, NodeMemory + Super::GetInstanceMemorySize());
	}
}

void UBTService_BlueprintBase::CopyPropertiesFromMemory(const uint8* NodeMemory)
{
	if (PropertyMemorySize > 0)
	{
		BlueprintNodeHelpers::CopyPropertiesFromContext(PropertyData, (uint8*)this, (uint8*)NodeMemory + Super::GetInstanceMemorySize());
	}
}

void UBTService_BlueprintBase::StartUsingExternalEvent(AActor* OwningActor)
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

void UBTService_BlueprintBase::StopUsingExternalEvent()
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

FString UBTService_BlueprintBase::GetStaticDescription() const
{
	FString ReturnDesc = Super::GetStaticDescription();
	if (bShowPropertyDetails)
	{
		UClass* StopAtClass = UBTService_BlueprintBase::StaticClass();
		FString PropertyDesc = BlueprintNodeHelpers::CollectPropertyDescription(this, StopAtClass, PropertyData);
		if (PropertyDesc.Len())
		{
			ReturnDesc += TEXT(":\n\n");
			ReturnDesc += PropertyDesc;
		}
	}

	return ReturnDesc;
}

void UBTService_BlueprintBase::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	if (PropertyMemorySize > 0)
	{
		UClass* StopAtClass = UBTService_BlueprintBase::StaticClass();
		BlueprintNodeHelpers::DescribeRuntimeValues(this, StopAtClass, PropertyData, NodeMemory + Super::GetInstanceMemorySize(), Verbosity, Values);
	}
}
