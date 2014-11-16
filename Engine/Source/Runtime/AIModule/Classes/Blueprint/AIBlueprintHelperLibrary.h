// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * This kismet library is used for helper functions primarily used in the kismet compiler for AI related nodes
 * NOTE: Do not change the signatures for any of these functions as it can break the kismet compiler and/or the nodes referencing them
 */

#pragma once
#include "AIBlueprintHelperLibrary.generated.h"

UCLASS()
class AIMODULE_API UAIBlueprintHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta=(WorldContext="WorldContextObject", BlueprintInternalUseOnly = "TRUE"))
	static class UAIAsyncTaskBlueprintProxy* CreateMoveToProxyObject(class UObject* WorldContextObject, APawn* Pawn, FVector Destination, AActor* TargetActor = NULL, float AcceptanceRadius = 0, bool bStopOnOverlap = false);

	UFUNCTION(BlueprintCallable, Category="AI", meta=(DefaultToSelf="MessageSource"))
	static void SendAIMessage(APawn* Target, FName Message, UObject* MessageSource, bool bSuccess = true);

	UFUNCTION(BlueprintCallable, Category="AI", meta=(WorldContext="WorldContextObject", UnsafeDuringActorConstruction="true"))
	static class APawn* SpawnAIFromClass(class UObject* WorldContextObject, TSubclassOf<APawn> PawnClass, class UBehaviorTree* BehaviorTree, FVector Location, FRotator Rotation=FRotator::ZeroRotator, bool bNoCollisionFail=false);

	UFUNCTION(BlueprintPure, Category="AI", meta=(DefaultToSelf="Target"))
	static class UBlackboardComponent* GetBlackboard(AActor* Target);

	/** locks indicated AI resources of animated pawn */
	UFUNCTION(BlueprintCallable, Category = "Animation", BlueprintAuthorityOnly, meta = (DefaultToSelf = "AnimInstance"))
	static void LockAIResourcesWithAnimation(class UAnimInstance* AnimInstance, bool bLockMovement, bool LockAILogic);

	/** unlocks indicated AI resources of animated pawn. Will unlock only animation-locked resources */
	UFUNCTION(BlueprintCallable, Category = "Animation", BlueprintAuthorityOnly, meta = (DefaultToSelf = "AnimInstance"))
	static void UnlockAIResourcesWithAnimation(class UAnimInstance* AnimInstance, bool bUnlockMovement, bool UnlockAILogic);

	UFUNCTION(BlueprintPure, Category = "AI")
	static bool IsValidAILocation(FVector Location);

	UFUNCTION(BlueprintPure, Category = "AI")
	static bool IsValidAIDirection(FVector DirectionVector);

	UFUNCTION(BlueprintPure, Category = "AI")
	static bool IsValidAIRotation(FRotator Rotation);
};
