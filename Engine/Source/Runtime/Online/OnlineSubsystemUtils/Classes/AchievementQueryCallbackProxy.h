// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Runtime/Online/OnlineSubsystem/Public/Interfaces/OnlineAchievementsInterface.h"
#include "AchievementQueryCallbackProxy.generated.h"

UCLASS(MinimalAPI)
class UAchievementQueryCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when there is a successful query
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	// Called when there is an unsuccessful query
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

	// Fetches and caches achievement progress from the default online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true"), Category = "Online|Achievements")
	static UAchievementQueryCallbackProxy* CacheAchievements(class APlayerController* PlayerController);

	// Fetches and caches achievement descriptions from the default online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true"), Category = "Online|Achievements")
	static UAchievementQueryCallbackProxy* CacheAchievementDescriptions(class APlayerController* PlayerController);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() override;
	// End of UOnlineBlueprintCallProxyBase interface

private:
	// Internal callback when the achievement query completes, calls out to the public success/failure callbacks
	void OnQueryCompleted(const FUniqueNetId& UserID, bool bSuccess);

private:
	// The player controller triggering things
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	// Are we querying achievement progress or achievement descriptions?
	bool bFetchDescriptions;
};
