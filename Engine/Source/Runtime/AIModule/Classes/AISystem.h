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

	FORCEINLINE static UAISystem* GetCurrent(UWorld* World) 
	{ 
		check(World != NULL)
		check(World->GetAISystem() == NULL || Cast<UAISystem>(World->GetAISystem()) != NULL);
		return (UAISystem*)(World->GetAISystem());
	}

	FORCEINLINE static UBehaviorTreeManager* GetCurrentBTManager(UWorld* World)
	{
		UAISystem* AISys = GetCurrent(World);
		// if AI system is NULL you're probably running your AI code on a client
		// or did something horribly wrong
		check(AISys != NULL);
		return AISys->GetBehaviorTreeManager();
	}

	FORCEINLINE static UEnvQueryManager* GetCurrentEQSManager(UWorld* World)
	{
		UAISystem* AISys = GetCurrent(World);
		// if AI system is NULL you're probably running your AI code on a client
		check(AISys != NULL);
		return AISys->GetEnvironmentQueryManager();
	}

	FORCEINLINE UWorld* GetOuterWorld() const { return Cast<UWorld>(GetOuter()); }

	virtual UWorld* GetWorld() const override { return GetOuterWorld(); }
	
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