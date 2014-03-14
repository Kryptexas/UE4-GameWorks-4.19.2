// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BTDecorator_BlackboardBase.generated.h"

UCLASS(Abstract)
class ENGINE_API UBTDecorator_BlackboardBase : public UBTDecorator
{
	GENERATED_UCLASS_BODY()

	/** initialize any asset related data */
	virtual void InitializeFromAsset(class UBehaviorTree* Asset) OVERRIDE;

	/** notify about change in blackboard keys */
	virtual void OnBlackboardChange(const class UBlackboardComponent* Blackboard, uint8 ChangedKeyID);

	/** get name of selected blackboard key */
	FName GetSelectedBlackboardKey() const;

protected:

	/** blackboard key selector */
	UPROPERTY(EditAnywhere, Category=Blackboard)
	struct FBlackboardKeySelector BlackboardKey;

	FOnBlackboardChange BBKeyObserver;

	virtual void PostInitProperties() OVERRIDE;

	/** called when execution flow controller becomes active */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;

	/** called when execution flow controller becomes inactive */
	virtual void OnCeaseRelevant(UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE FName UBTDecorator_BlackboardBase::GetSelectedBlackboardKey() const
{
	return BlackboardKey.SelectedKeyName;
}
