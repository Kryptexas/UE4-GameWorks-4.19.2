// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EngineUtils.h"
#include "BehaviorTree/BehaviorTreeManager.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "GameFramework/PlayerController.h"
#include "GameplayDebuggingReplicator.h"
#include "GameplayDebuggingComponent.h"
#include "AISystem.h"

UAISystem::UAISystem(const FPostConstructInitializeProperties& PCIP) : Super(PCIP)
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

void UAISystem::InitializeActorsForPlay(bool bTimeGotReset)
{

}

void UAISystem::WorldOriginChanged(FIntPoint OldOrigin, FIntPoint NewOrigin)
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

void UAISystem::RunEQS(const FString& QueryName)
{
	UWorld* OuterWorld = GetOuterWorld();
	if (OuterWorld == NULL)
	{
		return;
	}

	APlayerController* MyPC = OuterWorld->GetFirstPlayerController();
	UEnvQueryManager* EQS = GetEnvironmentQueryManager();
	if (MyPC && EQS)
	{
		AGameplayDebuggingReplicator* DebuggingReplicator = NULL;
		for (FActorIterator It(OuterWorld); It; ++It)
		{
			AActor* A = *It;
			if (A && A->IsA(AGameplayDebuggingReplicator::StaticClass()) && !A->IsPendingKill())
			{
				DebuggingReplicator = Cast<AGameplayDebuggingReplicator>(A);
				break;
			}
		}

		UGameplayDebuggingComponent* DebuggingComponent = DebuggingReplicator != NULL ? DebuggingReplicator->GetDebugComponent() : NULL;
		UObject* Target = DebuggingComponent != NULL ? DebuggingComponent->GetSelectedActor() : NULL;

		if (Target)
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
	}
}