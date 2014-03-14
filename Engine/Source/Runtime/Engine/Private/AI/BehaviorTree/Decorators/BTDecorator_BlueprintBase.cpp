// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "../BlueprintNodeHelpers.h"

UBTDecorator_BlueprintBase::UBTDecorator_BlueprintBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	UClass* StopAtClass = UBTDecorator_BlueprintBase::StaticClass();
	bImplementsReceiveTick = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveTick"), this, StopAtClass);
	bImplementsReceiveExecutionStart = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveExecutionStart"), this, StopAtClass);
	bImplementsReceiveExecutionFinish = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveExecutionFinish"), this, StopAtClass);
	bImplementsReceiveObserverActivated = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveObserverActivated"), this, StopAtClass);
	bImplementsReceiveObserverDeactivated = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveObserverDeactivated"), this, StopAtClass);
	bImplementsReceiveConditionCheck = BlueprintNodeHelpers::HasBlueprintFunction(TEXT("ReceiveConditionCheck"), this, StopAtClass);

	bNotifyBecomeRelevant = bImplementsReceiveObserverActivated;
	bNotifyCeaseRelevant = bImplementsReceiveObserverDeactivated;
	bNotifyTick = bImplementsReceiveTick;
	bNotifyActivation = bImplementsReceiveExecutionStart;
	bNotifyDeactivation = bImplementsReceiveExecutionFinish;
	bShowPropertyDetails = true;

	// all blueprint based nodes must create instances
	bCreateNodeInstance = true;

	InitializeProperties();
}

void UBTDecorator_BlueprintBase::InitializeProperties()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		UClass* StopAtClass = UBTDecorator_BlueprintBase::StaticClass();
		BlueprintNodeHelpers::CollectPropertyData(this, StopAtClass, PropertyData);

		if (GetFlowAbortMode() != EBTFlowAbortMode::None)
		{
			// find all blackboard key selectors and store their names
			BlueprintNodeHelpers::CollectBlackboardSelectors(this, StopAtClass, ObservedKeyNames);
		}
	}
	else
	{
		UBTDecorator_BlueprintBase* CDO = (UBTDecorator_BlueprintBase*)(GetClass()->GetDefaultObject());
		if (CDO && CDO->ObservedKeyNames.Num())
		{
			bNotifyBecomeRelevant = true;
			bNotifyCeaseRelevant = true;
		}
	}
}

void UBTDecorator_BlueprintBase::PostInitProperties()
{
	Super::PostInitProperties();
	NodeName = BlueprintNodeHelpers::GetNodeName(this);
	BBKeyObserver = FOnBlackboardChange::CreateUObject(this, &UBTDecorator_BlueprintBase::OnBlackboardChange);
}

void UBTDecorator_BlueprintBase::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	if (bImplementsReceiveObserverActivated)
	{
		ReceiveObserverActivated(OwnerComp->GetOwner());
	}

	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	UBTDecorator_BlueprintBase* CDO = (UBTDecorator_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (BlackboardComp && CDO)
	{
		for (int32 i = 0; i < CDO->ObservedKeyNames.Num(); i++)
		{
			const uint8 KeyID = BlackboardComp->GetKeyID(CDO->ObservedKeyNames[i]);
			if (KeyID != UBlackboardData::InvalidKeyID)
			{
				BlackboardComp->RegisterObserver(KeyID, BBKeyObserver);
			}
		}
	}
}

void UBTDecorator_BlueprintBase::OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	UBTDecorator_BlueprintBase* CDO = (UBTDecorator_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (BlackboardComp && CDO)
	{
		for (int32 i = 0; i < CDO->ObservedKeyNames.Num(); i++)
		{
			const uint8 KeyID = BlackboardComp->GetKeyID(CDO->ObservedKeyNames[i]);
			if (KeyID != UBlackboardData::InvalidKeyID)
			{
				BlackboardComp->UnregisterObserver(KeyID, BBKeyObserver);
			}
		}
	}
		
	if (bImplementsReceiveObserverDeactivated)
	{
		ReceiveObserverDeactivated(OwnerComp->GetOwner());
	}
}

void UBTDecorator_BlueprintBase::OnNodeActivation(struct FBehaviorTreeSearchData& SearchData)
{
	// skip flag, will be handled by bNotifyActivation

	ReceiveExecutionStart(SearchData.OwnerComp->GetOwner());
}

void UBTDecorator_BlueprintBase::OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult)
{
	// skip flag, will be handled by bNotifyDeactivation

	ReceiveExecutionFinish(SearchData.OwnerComp->GetOwner(), NodeResult);
}

void UBTDecorator_BlueprintBase::TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// skip flag, will be handled by bNotifyTick

	ReceiveTick(OwnerComp->GetOwner(), DeltaSeconds);
}

bool UBTDecorator_BlueprintBase::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	CurrentCallResult = false;
	if (bImplementsReceiveConditionCheck)
	{
		// can't use const functions with blueprints
		UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
		MyNode->ReceiveConditionCheck(OwnerComp->GetOwner());
	}

	return CurrentCallResult;
}

void UBTDecorator_BlueprintBase::FinishConditionCheck(bool bAllowExecution)
{
	CurrentCallResult = bAllowExecution;
}

FString UBTDecorator_BlueprintBase::GetStaticDescription() const
{
	FString ReturnDesc = Super::GetStaticDescription();

	UBTDecorator_BlueprintBase* CDO = (UBTDecorator_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (bShowPropertyDetails && CDO)
	{
		UClass* StopAtClass = UBTDecorator_BlueprintBase::StaticClass();
		FString PropertyDesc = BlueprintNodeHelpers::CollectPropertyDescription(this, StopAtClass, CDO->PropertyData);
		if (PropertyDesc.Len())
		{
			ReturnDesc += TEXT(":\n\n");
			ReturnDesc += PropertyDesc;
		}
	}

	return ReturnDesc;
}

void UBTDecorator_BlueprintBase::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	UBTDecorator_BlueprintBase* CDO = (UBTDecorator_BlueprintBase*)(GetClass()->GetDefaultObject());
	if (CDO && CDO->PropertyData.Num())
	{
		BlueprintNodeHelpers::DescribeRuntimeValues(this, CDO->PropertyData, Values);
	}
}

void UBTDecorator_BlueprintBase::OnBlackboardChange(const UBlackboardComponent* Blackboard, uint8 ChangedKeyID)
{
	UBehaviorTreeComponent* BehaviorComp = Blackboard ? (UBehaviorTreeComponent*)Blackboard->GetBehaviorComponent() : NULL;
	if (BehaviorComp)
	{
		BehaviorComp->RequestExecution(this);		
	}
}
