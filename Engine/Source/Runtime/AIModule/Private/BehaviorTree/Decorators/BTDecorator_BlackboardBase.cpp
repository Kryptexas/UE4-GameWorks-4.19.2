// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Decorators/BTDecorator_BlackboardBase.h"

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

void UBTDecorator_BlackboardBase::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->RegisterObserver(BlackboardKey.GetSelectedKeyID(), BBKeyObserver);
	}
}

void UBTDecorator_BlackboardBase::OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->UnregisterObserver(BlackboardKey.GetSelectedKeyID(), BBKeyObserver);
	}
}

void UBTDecorator_BlackboardBase::OnBlackboardChange(const UBlackboardComponent* Blackboard, FBlackboard::FKey ChangedKeyID)
{
	UBehaviorTreeComponent* BehaviorComp = Blackboard ? (UBehaviorTreeComponent*)Blackboard->GetBrainComponent() : NULL;
	if (BlackboardKey.GetSelectedKeyID() == ChangedKeyID && BehaviorComp)
	{
		BehaviorComp->RequestExecution(this);		
	}
}

#if WITH_EDITOR

FName UBTDecorator_BlackboardBase::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Decorator.Blackboard.Icon");
}

#endif	// WITH_EDITOR