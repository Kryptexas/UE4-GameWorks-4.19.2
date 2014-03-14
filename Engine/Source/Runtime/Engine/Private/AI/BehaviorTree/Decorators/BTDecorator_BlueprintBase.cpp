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

	// no point in waiting, since we don't care about meta data anymore
	DelayedInitialize();
}

void UBTDecorator_BlueprintBase::DelayedInitialize()
{
	UClass* StopAtClass = UBTDecorator_BlueprintBase::StaticClass();
	BlueprintNodeHelpers::CollectPropertyData(this, StopAtClass, PropertyData);

	PropertyMemorySize = BlueprintNodeHelpers::GetPropertiesMemorySize(PropertyData);

	ObservedKeyNames.Reset();
	if (GetFlowAbortMode() != EBTFlowAbortMode::None)
	{
		// find all blackboard key selectors and store their names
		BlueprintNodeHelpers::CollectBlackboardSelectors(this, StopAtClass, ObservedKeyNames);
	}

	if (ObservedKeyNames.Num())
	{
		UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
		MyNode->bNotifyBecomeRelevant = true;
		MyNode->bNotifyCeaseRelevant = true;
	}
}

void UBTDecorator_BlueprintBase::PostInitProperties()
{
	Super::PostInitProperties();
	NodeName = BlueprintNodeHelpers::GetNodeName(this);
	BBKeyObserver = FOnBlackboardChange::CreateUObject(this, &UBTDecorator_BlueprintBase::OnBlackboardChange);
}

void UBTDecorator_BlueprintBase::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	if (bImplementsReceiveObserverActivated)
	{
		// can't use const functions with blueprints
		UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
		CurrentCallOwner = OwnerComp;

		MyNode->CopyPropertiesFromMemory(NodeMemory);
		MyNode->ReceiveObserverActivated(CurrentCallOwner->GetOwner());
		CopyPropertiesToMemory(NodeMemory);

		CurrentCallOwner = NULL;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		for (int32 i = 0; i < ObservedKeyNames.Num(); i++)
		{
			const uint8 KeyID = BlackboardComp->GetKeyID(ObservedKeyNames[i]);
			if (KeyID < 0xFF)
			{
				BlackboardComp->RegisterObserver(KeyID, BBKeyObserver);
			}
		}
	}
}

void UBTDecorator_BlueprintBase::OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		for (int32 i = 0; i < ObservedKeyNames.Num(); i++)
		{
			const uint8 KeyID = BlackboardComp->GetKeyID(ObservedKeyNames[i]);
			if (KeyID < 0xFF)
			{
				BlackboardComp->UnregisterObserver(KeyID, BBKeyObserver);
			}
		}
	}
		
	if (bImplementsReceiveObserverDeactivated)
	{
		// can't use const functions with blueprints
		UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
		CurrentCallOwner = OwnerComp;

		MyNode->CopyPropertiesFromMemory(NodeMemory);
		MyNode->ReceiveObserverDeactivated(CurrentCallOwner->GetOwner());
		CopyPropertiesToMemory(NodeMemory);

		CurrentCallOwner = NULL;
	}
}

void UBTDecorator_BlueprintBase::OnNodeActivation(struct FBehaviorTreeSearchData& SearchData) const
{
	// skip flag, will be handled by bNotifyActivation

	// can't use const functions with blueprints
	UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
	CurrentCallOwner = SearchData.OwnerComp;

	MyNode->CopyPropertiesFromMemory(SearchData);
	MyNode->ReceiveExecutionStart(CurrentCallOwner->GetOwner());
	CopyPropertiesToMemory(SearchData);

	CurrentCallOwner = NULL;
}

void UBTDecorator_BlueprintBase::OnNodeDeactivation(struct FBehaviorTreeSearchData& SearchData, EBTNodeResult::Type NodeResult) const
{
	// skip flag, will be handled by bNotifyDeactivation

	// can't use const functions with blueprints
	UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
	CurrentCallOwner = SearchData.OwnerComp;

	MyNode->CopyPropertiesFromMemory(SearchData);
	MyNode->ReceiveExecutionFinish(CurrentCallOwner->GetOwner(), NodeResult);
	CopyPropertiesToMemory(SearchData);

	CurrentCallOwner = NULL;
}

void UBTDecorator_BlueprintBase::TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const
{
	// skip flag, will be handled by bNotifyTick

	// can't use const functions with blueprints
	UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
	CurrentCallOwner = OwnerComp;

	MyNode->CopyPropertiesFromMemory(NodeMemory);
	MyNode->ReceiveTick(CurrentCallOwner->GetOwner(), DeltaSeconds);
	CopyPropertiesToMemory(NodeMemory);

	CurrentCallOwner = NULL;
}

bool UBTDecorator_BlueprintBase::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	CurrentCallResult = false;
	if (bImplementsReceiveConditionCheck)
	{
		// can't use const functions with blueprints
		UBTDecorator_BlueprintBase* MyNode = (UBTDecorator_BlueprintBase*)this;
		CurrentCallOwner = OwnerComp;

		MyNode->CopyPropertiesFromMemory(NodeMemory);
		MyNode->ReceiveConditionCheck(CurrentCallOwner->GetOwner());
		CopyPropertiesToMemory(NodeMemory);

		CurrentCallOwner = NULL;
	}

	return CurrentCallResult;
}

void UBTDecorator_BlueprintBase::FinishConditionCheck(UBTDecorator_BlueprintBase* NodeOwner, bool bAllowExecution)
{
	if (NodeOwner)
	{
		NodeOwner->CurrentCallResult = bAllowExecution;
	}
}

uint16 UBTDecorator_BlueprintBase::GetInstanceMemorySize() const
{
	return Super::GetInstanceMemorySize() + PropertyMemorySize;
}

void UBTDecorator_BlueprintBase::CopyPropertiesToMemory(struct FBehaviorTreeSearchData& SearchData) const
{
	CopyPropertiesToMemory(GetNodeMemory<uint8>(SearchData));
}

void UBTDecorator_BlueprintBase::CopyPropertiesToMemory(uint8* NodeMemory) const
{
	if (PropertyMemorySize > 0)
	{
		BlueprintNodeHelpers::CopyPropertiesToContext(PropertyData, (uint8*)this, NodeMemory + Super::GetInstanceMemorySize());
	}
}

void UBTDecorator_BlueprintBase::CopyPropertiesFromMemory(const struct FBehaviorTreeSearchData& SearchData)
{
	CopyPropertiesFromMemory(GetNodeMemory<uint8>(SearchData));
}

void UBTDecorator_BlueprintBase::CopyPropertiesFromMemory(const uint8* NodeMemory)
{
	if (PropertyMemorySize > 0)
	{
		BlueprintNodeHelpers::CopyPropertiesFromContext(PropertyData, (uint8*)this, (uint8*)NodeMemory + Super::GetInstanceMemorySize());
	}
}

void UBTDecorator_BlueprintBase::StartUsingExternalEvent(AActor* OwningActor)
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

void UBTDecorator_BlueprintBase::StopUsingExternalEvent()
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

FString UBTDecorator_BlueprintBase::GetStaticDescription() const
{
	FString ReturnDesc = Super::GetStaticDescription();
	if (bShowPropertyDetails)
	{
		UClass* StopAtClass = UBTDecorator_BlueprintBase::StaticClass();
		FString PropertyDesc = BlueprintNodeHelpers::CollectPropertyDescription(this, StopAtClass, PropertyData);
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
	if (PropertyMemorySize > 0)
	{
		UClass* StopAtClass = UBTDecorator_BlueprintBase::StaticClass();
		BlueprintNodeHelpers::DescribeRuntimeValues(this, StopAtClass, PropertyData, NodeMemory + Super::GetInstanceMemorySize(), Verbosity, Values);
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
