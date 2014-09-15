// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "Misc/CoreMisc.h"
#include "EngineDefines.h"
#include "Engine/World.h"
#include "AI/AISystemBase.h"
#include "AISystem.generated.h"

class UBehaviorTreeManager;
class UEnvQueryManager;
class UAIPerceptionSystem;
class UAIAsyncTaskBlueprintProxy;

UCLASS(config=Engine)
class AIMODULE_API UAISystem : public UAISystemBase
{
	GENERATED_UCLASS_BODY()

protected:
	/** Behavior tree manager used by game */
	UPROPERTY(Transient)
	UBehaviorTreeManager* BehaviorTreeManager;

	/** Environment query manager used by game */
	UPROPERTY(Transient)
	UEnvQueryManager* EnvironmentQueryManager;

	UPROPERTY(Transient)
	UAIPerceptionSystem* PerceptionSystem;

	UPROPERTY(Transient)
	TArray<class UAIAsyncTaskBlueprintProxy*> AllProxyObjects;

public:
	virtual ~UAISystem();

	// IAISystemInterface begin		
	virtual void InitializeActorsForPlay(bool bTimeGotReset) override;
	virtual void WorldOriginLocationChanged(FIntVector OldOriginLocation, FIntVector NewOriginLocation) override;
	virtual void CleanupWorld(bool bSessionEnded = true, bool bCleanupResources = true, UWorld* NewWorld = NULL) override;
	// IAISystemInterface end
	
	/** Behavior tree manager getter */
	FORCEINLINE UBehaviorTreeManager* GetBehaviorTreeManager() { return BehaviorTreeManager; }
	/** Behavior tree manager const getter */
	FORCEINLINE const UBehaviorTreeManager* GetBehaviorTreeManager() const { return BehaviorTreeManager; }

	/** Behavior tree manager getter */
	FORCEINLINE UEnvQueryManager* GetEnvironmentQueryManager() { return EnvironmentQueryManager; }
	/** Behavior tree manager const getter */
	FORCEINLINE const UEnvQueryManager* GetEnvironmentQueryManager() const { return EnvironmentQueryManager; }

	FORCEINLINE UAIPerceptionSystem* GetPerceptionSystem() { return PerceptionSystem; }
	FORCEINLINE const UAIPerceptionSystem* GetPerceptionSystem() const { return PerceptionSystem; }

	FORCEINLINE static UAISystem* GetCurrent(UWorld* World, bool bChecked = true) 
	{ 
		if (!bChecked && World == NULL)
		{
			return NULL;
		}

		check(World);
		return Cast<UAISystem>(World->GetAISystem());
	}

	FORCEINLINE UWorld* GetOuterWorld() const { return Cast<UWorld>(GetOuter()); }

	virtual UWorld* GetWorld() const override { return GetOuterWorld(); }
	
	FORCEINLINE void AddReferenceFromProxyObject(UAIAsyncTaskBlueprintProxy* BlueprintProxy) { AllProxyObjects.AddUnique(BlueprintProxy); }

	FORCEINLINE void RemoveReferenceToProxyObject(UAIAsyncTaskBlueprintProxy* BlueprintProxy) { AllProxyObjects.RemoveSwap(BlueprintProxy); }

	//----------------------------------------------------------------------//
	// cheats
	//----------------------------------------------------------------------//
	UFUNCTION(exec)
	virtual void AIIgnorePlayers();

	UFUNCTION(exec)
	virtual void AILoggingVerbose();

	/** insta-runs EQS query for given Target */
	void RunEQS(const FString& QueryName, UObject* Target);
};