// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTService_DefaultFocus.generated.h"

struct FBTDefaultFocusMemory
{
	AActor* FocusActorSet;
	FVector FocusLocationSet;
	bool bActorSet;

	void Reset()
	{
		FMemory::MemZero(*this);
	}
};

UCLASS(hidecategories=(Service))
class UBTService_DefaultFocus : public UBTService_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	virtual uint16 GetInstanceMemorySize() const override;
	virtual void OnBecomeRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) override;

	virtual FString GetStaticDescription() const override;
	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
};
