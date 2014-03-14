// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "BTService_BlackboardBase.generated.h"

UCLASS(Abstract)
class ENGINE_API UBTService_BlackboardBase : public UBTService
{
	GENERATED_UCLASS_BODY()

	/** initialize any asset related data */
	virtual void InitializeFromAsset(class UBehaviorTree* Asset) OVERRIDE;

	/** get name of selected blackboard key */
	FName GetSelectedBlackboardKey() const;

protected:

	/** blackboard key selector */
	UPROPERTY(EditAnywhere, Category=Blackboard)
	struct FBlackboardKeySelector BlackboardKey;
};

//////////////////////////////////////////////////////////////////////////
// Inlines

FORCEINLINE FName UBTService_BlackboardBase::GetSelectedBlackboardKey() const
{
	return BlackboardKey.SelectedKeyName;
}
