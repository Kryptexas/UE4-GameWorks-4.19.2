// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_BlackboardBase::UBTDecorator_BlackboardBase(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "BlackboardBase";

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;

	// empty KeySelector = allow everything
}

void UBTDecorator_BlackboardBase::PostInitProperties()
{
	Super::PostInitProperties();
	BBKeyObserver = FOnBlackboardChange::CreateUObject(this, &UBTDecorator_BlackboardBase::OnBlackboardChange);
}

void UBTDecorator_BlackboardBase::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);
	BlackboardKey.CacheSelectedKey(GetBlackboardAsset());
}

void UBTDecorator_BlackboardBase::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->RegisterObserver(BlackboardKey.SelectedKeyID, BBKeyObserver);
	}
}

void UBTDecorator_BlackboardBase::OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->UnregisterObserver(BlackboardKey.SelectedKeyID, BBKeyObserver);
	}
}

void UBTDecorator_BlackboardBase::OnBlackboardChange(const UBlackboardComponent* Blackboard, uint8 ChangedKeyID)
{
	UBehaviorTreeComponent* BehaviorComp = Blackboard ? (UBehaviorTreeComponent*)Blackboard->GetBehaviorComponent() : NULL;
	if (BlackboardKey.SelectedKeyID == ChangedKeyID && BehaviorComp)
	{
		BehaviorComp->RequestExecution(this);		
	}
}
