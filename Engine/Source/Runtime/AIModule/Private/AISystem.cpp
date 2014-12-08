// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EngineUtils.h"
#include "StringClassReference.h"
#include "BehaviorTree/BehaviorTreeManager.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "Perception/AIPerceptionSystem.h"
#include "GameFramework/PlayerController.h"
#include "HotSpots/AIHotSpotManager.h"
#include "AISystem.h"


UAISystem::UAISystem(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UWorld* WorldOuter = Cast<UWorld>(GetOuter());
		UObject* ManagersOuter = WorldOuter != NULL ? (UObject*)WorldOuter : (UObject*)this;
		BehaviorTreeManager = NewObject<UBehaviorTreeManager>(ManagersOuter);
		EnvironmentQueryManager = NewObject<UEnvQueryManager>(ManagersOuter);
	}
}

UAISystem::~UAISystem()
{
	CleanupWorld(true, true, NULL);
}

void UAISystem::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		UWorld* WorldOuter = Cast<UWorld>(GetOuter());
		UObject* ManagersOuter = WorldOuter != NULL ? (UObject*)WorldOuter : (UObject*)this;
		
		TSubclassOf<UAIHotSpotManager> HotSpotManagerClass = HotSpotManagerClassName.IsValid() ? LoadClass<UAIHotSpotManager>(NULL, *HotSpotManagerClassName.ToString(), NULL, LOAD_None, NULL) : nullptr;
		if (HotSpotManagerClass)
		{
			HotSpotManager = NewObject<UAIHotSpotManager>(ManagersOuter, HotSpotManagerClass);
		}

		TSubclassOf<UAIPerceptionSystem> PerceptionSystemClass = PerceptionSystemClassName.IsValid() ? LoadClass<UAIPerceptionSystem>(NULL, *PerceptionSystemClassName.ToString(), NULL, LOAD_None, NULL) : nullptr;
		if (PerceptionSystemClass)
		{
			PerceptionSystem = NewObject<UAIPerceptionSystem>(ManagersOuter, PerceptionSystemClass);
		}
	}
}

void UAISystem::InitializeActorsForPlay(bool bTimeGotReset)
{

}

void UAISystem::WorldOriginLocationChanged(FIntVector OldOriginLocation, FIntVector NewOriginLocation)
{

}

void UAISystem::CleanupWorld(bool bSessionEnded, bool bCleanupResources, UWorld* NewWorld)
{

}

void UAISystem::AIIgnorePlayers()
{
	AAIController::ToggleAIIgnorePlayers();
}

void UAISystem::AILoggingVerbose()
{
	UWorld* OuterWorld = GetOuterWorld();
	if (OuterWorld)
	{
		APlayerController* PC = OuterWorld->GetFirstPlayerController();
		if (PC)
		{
			PC->ConsoleCommand(TEXT("log lognavigation verbose | log logpathfollowing verbose | log LogCharacter verbose | log LogBehaviorTree verbose | log LogPawnAction verbose|"));
		}
	}
}

void UAISystem::RunEQS(const FString& QueryName, UObject* Target)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	UWorld* OuterWorld = GetOuterWorld();
	if (OuterWorld == NULL)
	{
		return;
	}

	APlayerController* MyPC = OuterWorld->GetFirstPlayerController();
	UEnvQueryManager* EQS = GetEnvironmentQueryManager();

	if (Target && MyPC && EQS)
	{
		const UEnvQuery* QueryTemplate = EQS->FindQueryTemplate(QueryName);

		if (QueryTemplate)
		{
			EQS->RunInstantQuery(FEnvQueryRequest(QueryTemplate, Target), EEnvQueryRunMode::AllMatching);
		}
		else
		{
			MyPC->ClientMessage(FString::Printf(TEXT("Unable to fing query template \'%s\'"), *QueryName));
		}
	}
	else
	{
		MyPC->ClientMessage(TEXT("No debugging target"));
	}
#endif
}