// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * This kismet library is used for helper functions primarily used in the kismet compiler for AI related nodes
 * NOTE: Do not change the signatures for any of these functions as it can break the kismet compiler and/or the nodes referencing them
 */

#pragma once
#include "KismetAIHelperLibrary.generated.h"

UCLASS(HeaderGroup=KismetLibrary,MinimalAPI)
class UKismetAIHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	UFUNCTION(BlueprintCallable, meta=(HidePin="WorldContextObject", DefaultToSelf="WorldContextObject", BlueprintInternalUseOnly = "TRUE"))
	static class UKismetAIAsyncTaskProxy* CreateMoveToProxyObject(class UObject* WorldContextObject, APawn* Pawn, FVector Destination, AActor* TargetActor=NULL, float AcceptanceRadius=0, bool bStopOnOverlap=false);

	UFUNCTION(BlueprintCallable, Category="AI", meta=(DefaultToSelf="MessageSource"))
	static void SendAIMessage(APawn* Target, FName Message, UObject* MessageSource, bool bSuccess = true);

	UFUNCTION(BlueprintCallable, Category="AI", meta=(HidePin="WorldContextObject", DefaultToSelf="WorldContextObject"))
	static class APawn* SpawnAI(class UObject* WorldContextObject, UBlueprint* Pawn, class UBehaviorTree* BehaviorTree, FVector Location, FRotator Rotation=FRotator::ZeroRotator, bool bNoCollisionFail=false);

	UFUNCTION(BlueprintPure, Category="AI", meta=(DefaultToSelf="Target"))
	static class UBlackboardComponent* GetBlackboard(AActor* Target);
};
