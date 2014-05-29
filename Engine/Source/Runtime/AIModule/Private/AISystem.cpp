// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/BehaviorTreeManager.h"
#include "EnvironmentQuery/EnvQueryManager.h"
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
		AActor* Target = NULL;
		UGameplayDebuggingControllerComponent* DebugComp = MyPC->FindComponentByClass<UGameplayDebuggingControllerComponent>();
		if (DebugComp)
		{
			Target = DebugComp->GetCurrentDebugTarget();
		}

//#if WITH_EDITOR
//		if (Target == NULL && GEditor != NULL)
//		{
//			Target = GEditor->GetSelectedObjects()->GetTop<AActor>();
//
//			// this part should not be needed, but is due to gameplay debugging messed up design
//			if (Target == NULL)
//			{
//				for (FObjectIterator It(UGameplayDebuggingComponent::StaticClass()); It && (Target == NULL); ++It)
//				{
//					UGameplayDebuggingComponent* Comp = ((UGameplayDebuggingComponent*)*It);
//					if (Comp->IsSelected())
//					{
//						Target = Comp->GetOwner();
//					}
//				}
//			}
//		}
//#endif // WITH_EDITOR

		if (Target)
		{
			UEnvQuery* QueryTemplate = EQS->FindQueryTemplate(QueryName);

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