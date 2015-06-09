// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayTask.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "AITask_MoveTo.generated.h"

class APawn;
class AAIController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMoveTaskCompletedSignature, const TEnumAsByte<EPathFollowingResult::Type>&, Result);

UCLASS()
class AIMODULE_API UAITask_MoveTo : public UGameplayTask
{
	GENERATED_BODY()
protected:
	UPROPERTY(BlueprintAssignable)
	FGenericGameplayTaskDelegate OnRequestFailed;

	UPROPERTY(BlueprintAssignable)
	FMoveTaskCompletedSignature OnMoveFinished;

	UPROPERTY()
	AAIController* OwnerController;
	
	UPROPERTY()
	FAIMoveRequest MoveRequest;

	FAIRequestID MoveRequestID;

	virtual void HandleMoveFinished(FAIRequestID RequestID, EPathFollowingResult::Type Result);
	virtual void Activate() override;

	virtual void Pause() override;
	virtual void Resume() override;

public:
	UAITask_MoveTo(const FObjectInitializer& ObjectInitializer);

	/** Spawn new Actor on the network authority (server) */
	UFUNCTION(BlueprintCallable, Category = "GameplayTasks", meta = (AdvancedDisplay = "TaskOwner,AcceptanceRadius,bStopOnOverlap,bAcceptPartialPath", DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "TRUE", DisplayName="Move To Location or Actor"))
	static UAITask_MoveTo* PawnMoveTo(APawn* Pawn, FVector Destination, AActor* TargetActor = NULL, float AcceptanceRadius = 5.f, bool bStopOnOverlap = false, bool bAcceptPartialPath = false);

	UFUNCTION(BlueprintCallable, Category = "AI|Tasks", meta = (AdvancedDisplay = "TaskOwner,AcceptanceRadius,bStopOnOverlap,bAcceptPartialPath", DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "TRUE", DisplayName = "Move To Location or Actor"))
	static UAITask_MoveTo* AIMoveTo(AAIController* Controller, FVector TargetLocation, AActor* TargetActor = NULL, float AcceptanceRadius = 5.f, bool bStopOnOverlap = false, bool bAcceptPartialPath = false);
};