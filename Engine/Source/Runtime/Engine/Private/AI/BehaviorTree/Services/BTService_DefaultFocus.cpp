// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTService_DefaultFocus::UBTService_DefaultFocus(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
{
	NodeName = "Set default focus";

	bNotifyTick = false;
	bTickIntervals = false;
	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;

	// accept only actors and vectors
	BlackboardKey.AddObjectFilter(this, AActor::StaticClass());
	BlackboardKey.AddVectorFilter(this);
}

void UBTService_DefaultFocus::OnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	FBTDefaultFocusMemory* MyMemory = (FBTDefaultFocusMemory*)NodeMemory;
	check(MyMemory);
	MyMemory->Reset();

	AAIController* OwnerController = OwnerComp ? Cast<AAIController>(OwnerComp->GetOwner()) : NULL;	
	const UBlackboardComponent* MyBlackboard = OwnerComp->GetBlackboardComponent();
	
	if (OwnerController != NULL && MyBlackboard != NULL)
	{
		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* KeyValue = MyBlackboard->GetValueAsObject(BlackboardKey.SelectedKeyID);
			AActor* TargetActor = Cast<AActor>(KeyValue);
			if (TargetActor)
			{
				OwnerController->SetFocus(TargetActor, EAIFocusPriority::Default);
				MyMemory->FocusActorSet = TargetActor;
				MyMemory->bActorSet = true;
			}
		}
		else
		{
			const FVector FocusLocation = MyBlackboard->GetValueAsVector(BlackboardKey.SelectedKeyID);
			OwnerController->SetFocalPoint(FocusLocation, /*bOffsetFromBase=*/false, EAIFocusPriority::Default);
			MyMemory->FocusLocationSet = FocusLocation;
		}
	}
}

void UBTService_DefaultFocus::OnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	FBTDefaultFocusMemory* MyMemory = (FBTDefaultFocusMemory*)NodeMemory;
	check(MyMemory);
	AAIController* OwnerController = OwnerComp ? Cast<AAIController>(OwnerComp->GetOwner()) : NULL;	
	if (OwnerController != NULL)
	{
		bool bClearFocus = false;
		if (MyMemory->bActorSet)
		{
			bClearFocus = (MyMemory->FocusActorSet == OwnerController->GetFocusActor(EAIFocusPriority::Default));
		}
		else
		{
			bClearFocus = (MyMemory->FocusLocationSet == OwnerController->GetFocalPoint(EAIFocusPriority::Default));
		}

		if (bClearFocus)
		{
			OwnerController->ClearFocus();
		}
	}
}

FString UBTService_DefaultFocus::GetStaticDescription() const
{	
	FString KeyDesc("invalid");
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDesc = BlackboardKey.SelectedKeyName.ToString();
	}

	return FString::Printf(TEXT("Set default focus to %s"), *KeyDesc);
}

void UBTService_DefaultFocus::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);
}

uint16 UBTService_DefaultFocus::GetInstanceMemorySize() const
{
	return sizeof(FBTDefaultFocusMemory);
}
