// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTDecorator_CompareBBEntries::UBTDecorator_CompareBBEntries(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
	, Operator(EArithmeticKeyOperation::LessOrEqual)
{
	NodeName = "Compare Blackboard entries";

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
}

void UBTDecorator_CompareBBEntries::PostInitProperties()
{
	Super::PostInitProperties();
	BBKeyObserver = FOnBlackboardChange::CreateUObject(this, &UBTDecorator_CompareBBEntries::OnBlackboardChange);
}

void UBTDecorator_CompareBBEntries::InitializeFromAsset(class UBehaviorTree* Asset)
{
	Super::InitializeFromAsset(Asset);

	UBlackboardData* BBAsset = GetBlackboardAsset();
	BlackboardKeyA.CacheSelectedKey(BBAsset);
	BlackboardKeyB.CacheSelectedKey(BBAsset);
}

// @note I know it's ugly to have "return" statements in many places inside a function, but the way 
// around was very awkward here 
bool UBTDecorator_CompareBBEntries::CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const 
{
	// first of all require same type
	// @todo this could be checked statically (i.e. in editor, asset creation time)!
	if (BlackboardKeyA.SelectedKeyType != BlackboardKeyB.SelectedKeyType)
	{
		return false;
	}
	
	const UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		//BlackboardComp->GetKeyType
		const UBlackboardKeyType::CompareResult Result = BlackboardComp->CompareKeyValues(BlackboardKeyA.SelectedKeyType
			, BlackboardKeyA.SelectedKeyID, BlackboardKeyB.SelectedKeyID);

		checkAtCompileTime(int32(UBlackboardKeyType::Equal) == int32(EBlackBoardEntryComparison::Equal)
			&& int32(UBlackboardKeyType::NotEqual) == int32(EBlackBoardEntryComparison::NotEqual)
			, "These values need to be equal");

		return int32(Operator) == int32(Result);
	}

	return false;
}

FString UBTDecorator_CompareBBEntries::GetStaticDescription() const 
{
	return FString::Printf(TEXT("%s:\n%s and %s\ncontain %s values")
		, *Super::GetStaticDescription()
		, *BlackboardKeyA.SelectedKeyName.ToString()
		, *BlackboardKeyB.SelectedKeyName.ToString()
		, Operator == EBlackBoardEntryComparison::Equal ? TEXT("EQUAL") : TEXT("NOT EQUAL"));
}

void UBTDecorator_CompareBBEntries::OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->RegisterObserver(BlackboardKeyA.SelectedKeyID, BBKeyObserver);
		BlackboardComp->RegisterObserver(BlackboardKeyB.SelectedKeyID, BBKeyObserver);
	}
}

void UBTDecorator_CompareBBEntries::OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	UBlackboardComponent* BlackboardComp = OwnerComp->GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->UnregisterObserver(BlackboardKeyA.SelectedKeyID, BBKeyObserver);
		BlackboardComp->UnregisterObserver(BlackboardKeyB.SelectedKeyID, BBKeyObserver);
	}
}

void UBTDecorator_CompareBBEntries::OnBlackboardChange(const UBlackboardComponent* Blackboard, uint8 ChangedKeyID)
{
	UBehaviorTreeComponent* BehaviorComp = Blackboard ? (UBehaviorTreeComponent*)Blackboard->GetBehaviorComponent() : NULL;
	if (BehaviorComp && (BlackboardKeyA.SelectedKeyID == ChangedKeyID || BlackboardKeyB.SelectedKeyID == ChangedKeyID))
	{
		BehaviorComp->RequestExecution(this);		
	}
}
