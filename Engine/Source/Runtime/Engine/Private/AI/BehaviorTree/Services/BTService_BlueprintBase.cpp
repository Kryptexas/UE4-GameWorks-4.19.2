// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "BlueprintNodeHelpers.h"

UBTService_BlueprintBase::UBTService_BlueprintBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	UClass* StopAtClass = UBTService_BlueprintBase::StaticClass();
	bImplementsReceiveTick = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveTick"), this, StopAtClass);
	bImplementsReceiveActivation = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveActivation"), this, StopAtClass);
	bImplementsReceiveDeactivation = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveDeactivation"), this, StopAtClass);

	bNotifyBecomeRelevant = bImplementsReceiveActivation || bImplementsReceiveTick;
	bNotifyCeaseRelevant = bNotifyBecomeRelevant;
	bNotifyTick = bImplementsReceiveTick;
	bShowPropertyDetails = true;

	// all blueprint based nodes must create instances
	bCreateNodeInstance = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		BlueprintNodeHelpers::CollectPropertyData(this, StopAtClass, PropertyData);
	}
}

void UBTService_BlueprintBase::PostInitProperties()
{
	Super::PostInitProperties();
	NodeName = BlueprintNodeHelpers::GetNodeName(this);
}

void UBTService_BlueprintBase::OnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	// check flag, it could be used because user wants tick
	if (bImplementsReceiveActivation)
	{
		ReceiveActivation(OwnerComp->GetOwner());
	}
}

void UBTService_BlueprintBase::OnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	// force dropping all pending latent actions associated with this blueprint
	// we can't have those resuming activity when node is/was aborted
	BlueprintNodeHelpers::AbortLatentActions(OwnerComp->GetOwner(), this);

	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	if (bImplementsReceiveDeactivation)
	{
		ReceiveDeactivation(OwnerComp->GetOwner());
	}
}

void UBTService_BlueprintBase::TickNode(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// skip flag, will be handled by bNotifyTick

	ReceiveTick(OwnerComp->GetOwner(), DeltaSeconds);
}

bool UBTService_BlueprintBase::IsServiceActive() const
{
	UBehaviorTreeComponent* OwnerComp = Cast<UBehaviorTreeComponent>(GetOuter());
	const bool bIsActive = OwnerComp->IsAuxNodeActive(this);
	return bIsActive;
}

FString UBTService_BlueprintBase::GetStaticDescription() const
{
	FString ReturnDesc = Super::GetStaticDescription();

	UBTService_BlueprintBase* CDO = (UBTService_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (bShowPropertyDetails && CDO)
	{
		UClass* StopAtClass = UBTService_BlueprintBase::StaticClass();
		FString PropertyDesc = BlueprintNodeHelpers::CollectPropertyDescription(this, StopAtClass, CDO->PropertyData);
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
	UBTService_BlueprintBase* CDO = (UBTService_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (CDO && CDO->PropertyData.Num())
	{
		UClass* StopAtClass = UBTService_BlueprintBase::StaticClass();
		BlueprintNodeHelpers::DescribeRuntimeValues(this, CDO->PropertyData, Values);
	}
}
