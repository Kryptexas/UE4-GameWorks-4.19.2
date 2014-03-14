// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BTDecorator_CompareBBEntries.generated.h"

UENUM()
namespace EBlackBoardEntryComparison
{
	enum Type
	{
		Equal			UMETA(DisplayName="Is Equal To"),
		NotEqual		UMETA(DisplayName="Is Not Equal To")
	};
}

UCLASS(HideCategories=(Condition))
class UBTDecorator_CompareBBEntries : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

protected:

	/** operation type */
	UPROPERTY(EditAnywhere, Category=Blackboard)
	TEnumAsByte<EBlackBoardEntryComparison::Type> Operator;

	/** blackboard key selector */
	UPROPERTY(EditAnywhere, Category=Blackboard)
	struct FBlackboardKeySelector BlackboardKeyA;

	/** blackboard key selector */
	UPROPERTY(EditAnywhere, Category=Blackboard)
	struct FBlackboardKeySelector BlackboardKeyB;

	FOnBlackboardChange BBKeyObserver;

public:

	virtual void PostInitProperties() OVERRIDE;

	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;

	virtual void InitializeFromAsset(class UBehaviorTree* Asset) OVERRIDE;

	virtual void OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID);	
	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
};
