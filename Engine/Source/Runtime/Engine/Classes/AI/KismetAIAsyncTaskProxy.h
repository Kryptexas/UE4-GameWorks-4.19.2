// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "KismetAIAsyncTaskProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOAISimpleDelegate, EPathFollowingResult::Type, MovementResult);

UCLASS(MinimalAPI)
class UKismetAIAsyncTaskProxy : public UObject
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FOAISimpleDelegate	OnSuccess;

	UPROPERTY(BlueprintAssignable)
	FOAISimpleDelegate	OnFail;

public:
	UFUNCTION()
	void OnMoveCompleted(int32 RequestID, EPathFollowingResult::Type MovementResult);

	void OnNoPath();

	// Begin UObject interface
	virtual void BeginDestroy() OVERRIDE;
	// End UObject interface

	TWeakObjectPtr<class AAIController> AIController;
	int32 MoveRequestId;
};
