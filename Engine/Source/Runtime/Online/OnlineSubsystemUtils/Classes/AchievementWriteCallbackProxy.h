// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Runtime/Online/OnlineSubsystem/Public/Interfaces/OnlineAchievementsInterface.h"
#include "AchievementWriteCallbackProxy.generated.h"

UCLASS(MinimalAPI)
class UAchievementWriteCallbackProxy : public UOnlineBlueprintCallProxyBase
{
	GENERATED_UCLASS_BODY()

	// Called when there is a successful achievement write
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnSuccess;

	// Called when there is an unsuccessful achievement write
	UPROPERTY(BlueprintAssignable)
	FEmptyOnlineDelegate OnFailure;

	// Writes progress about an achievement to the default online subsystem
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly = "true"), Category="Online|Achievements")
	static UAchievementWriteCallbackProxy* WriteAchievementProgress(class APlayerController* PlayerController, FName AchievementName, float Progress = 100.0f);

	// UOnlineBlueprintCallProxyBase interface
	virtual void Activate() OVERRIDE;
	// End of UOnlineBlueprintCallProxyBase interface

	// UObject interface
	virtual void BeginDestroy() OVERRIDE;
	// End of UObject interface

private:
	// Internal callback when the achievement is written, calls out to the public success/failure callbacks
	void OnAchievementWritten(const FUniqueNetId& UserID, bool bSuccess);

private:
	/** The player controller triggering things */
	TWeakObjectPtr<APlayerController> PlayerControllerWeakPtr;

	/** The achievements write object */
	FOnlineAchievementsWritePtr WriteObject;
};
