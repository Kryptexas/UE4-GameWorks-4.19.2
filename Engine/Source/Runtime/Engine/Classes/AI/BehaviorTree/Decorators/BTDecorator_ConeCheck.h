// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTDecorator_ConeCheck.generated.h"

struct FBTConeCheckDecoratorMemory
{
	bool bLastRawResult;
};

UCLASS()
class ENGINE_API UBTDecorator_ConeCheck : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	typedef FBTConeCheckDecoratorMemory TNodeInstanceMemory;

	/** max allowed time for execution of underlying node */
	UPROPERTY(Category=Decorator, EditAnywhere)
	float ConeHalfAngle;
	
	/** blackboard key selector */
	UPROPERTY(EditAnywhere, Category=Blackboard)
	struct FBlackboardKeySelector ConeOrigin;

	UPROPERTY(EditAnywhere, Category=Blackboard)
	struct FBlackboardKeySelector ConeDirection;

	/** blackboard key selector */
	UPROPERTY(EditAnywhere, Category=Blackboard)
	struct FBlackboardKeySelector Observed;
	
	float ConeHalfAngleDot;

	virtual void InitializeFromAsset(class UBehaviorTree* Asset) OVERRIDE;
	virtual uint16 GetInstanceMemorySize() const OVERRIDE;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;

protected:

	virtual bool CalculateRawConditionValue(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	void OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID);

	virtual void TickNode(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, float DeltaSeconds) const OVERRIDE;
	
	bool CalculateDirection(const UBlackboardComponent* BlackboardComp, const FBlackboardKeySelector& Origin, const FBlackboardKeySelector& End, FVector& Direction) const;

private:
	bool CalcConditionImpl(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const;
};
