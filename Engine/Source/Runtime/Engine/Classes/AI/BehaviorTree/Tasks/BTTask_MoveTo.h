// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "BTTask_MoveTo.generated.h"

struct FBTMoveToTaskMemory
{
	/** Move request ID */
	uint32 MoveRequestID;
};

UCLASS(config=Game)
class ENGINE_API UBTTask_MoveTo : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(config, Category=Node, EditAnywhere, meta=(ClampMin = "0.0"))
	float AcceptableRadius;

	/** "None" will result in default filter being used */
	UPROPERTY(Category=Node, EditAnywhere)
	TSubclassOf<class UNavigationQueryFilter> FilterClass;

	UPROPERTY(Category=Node, EditAnywhere)
	uint32 bAllowStrafe : 1;
	
	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual uint16 GetInstanceMemorySize() const OVERRIDE;

	virtual void OnMessage(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess) const OVERRIDE;

	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;
};
