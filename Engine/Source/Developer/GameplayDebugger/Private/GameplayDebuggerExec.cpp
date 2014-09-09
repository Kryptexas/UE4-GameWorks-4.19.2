// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
#include "GameplayDebuggingControllerComponent.h"
#include "Misc/CoreMisc.h"
#include "AISystem.h"

#if WITH_EDITOR
#include "LevelEditorViewport.h"
#endif //WITH_EDITOR

struct GAMEPLAYDEBUGGER_API FGameplayDebuggerExec : public FSelfRegisteringExec
{
	FGameplayDebuggerExec()
		: FSelfRegisteringExec()
	{
		CachedDebuggingReplicator = NULL;
	}

	TWeakObjectPtr<AGameplayDebuggingReplicator> GetDebuggingReplicator(UWorld* InWorld);

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	// End FExec Interface

	TWeakObjectPtr<AGameplayDebuggingReplicator> CachedDebuggingReplicator;
};
FGameplayDebuggerExec GameplayDebuggerExecInstance;


TWeakObjectPtr<AGameplayDebuggingReplicator> FGameplayDebuggerExec::GetDebuggingReplicator(UWorld* InWorld)
{
	if (CachedDebuggingReplicator.IsValid() && CachedDebuggingReplicator->GetWorld() == InWorld)
	{
		return CachedDebuggingReplicator;
	}

	for (FActorIterator It(InWorld); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor != NULL && !Actor->IsPendingKill() && Actor->IsA(AGameplayDebuggingReplicator::StaticClass()))
		{
			CachedDebuggingReplicator = Cast<AGameplayDebuggingReplicator>(Actor);
			return CachedDebuggingReplicator;
		}
	}

	if (InWorld->GetNetMode() < ENetMode::NM_Client)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.bNoCollisionFail = true;
		SpawnInfo.Name = TEXT("GameplayDebuggingReplicator");
		CachedDebuggingReplicator = InWorld->SpawnActor<AGameplayDebuggingReplicator>(SpawnInfo);
	}

	return CachedDebuggingReplicator;
}

bool FGameplayDebuggerExec::Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	bool bHandled = false;

	APlayerController* PC = Inworld ? Inworld->GetFirstPlayerController() : NULL;
	if (!Inworld || !PC)
	{
		return bHandled;
	}

	if (FParse::Command(&Cmd, TEXT("EnableGDT")) && Inworld->GetNetMode() < NM_Client)
	{
		GetDebuggingReplicator(Inworld); // get or eventually create AGameplayDebuggerReplicator on server
	}
	
	if (FParse::Command(&Cmd, TEXT("cheat EnableGDT")) && Inworld->GetNetMode() != NM_DedicatedServer)
	{
		if (Inworld->GetNetMode() != NM_DedicatedServer)
		{
			AGameplayDebuggingReplicator* Replicator = NULL;
			for (TActorIterator<AGameplayDebuggingReplicator> It(Inworld); It; ++It)
			{
				Replicator = *It;
 				if (Replicator && !Replicator->IsPendingKill())
				{
					APlayerController* LocalPC = Replicator->GetLocalPlayerOwner();
					if (LocalPC == PC)
					{
						break;
					}
				}
				Replicator = NULL;
			}

			if (Replicator && !Replicator->IsToolCreated())
			{
				Replicator->CreateTool();
				Replicator->EnableTool();
				bHandled = true;
			}
		}
	}
	else if (FParse::Command(&Cmd, TEXT("RunEQS")))
	{
		bHandled = true;
		APlayerController* MyPC = Inworld->GetFirstPlayerController();
		UAISystem* AISys = UAISystem::GetCurrent(Inworld);

		UEnvQueryManager* EQS = AISys ? AISys->GetEnvironmentQueryManager() : NULL;
		if (MyPC && EQS)
		{
			AGameplayDebuggingReplicator* DebuggingReplicator = NULL;
			for (FActorIterator It(Inworld); It; ++It)
			{
				AActor* A = *It;
				if (A && A->IsA(AGameplayDebuggingReplicator::StaticClass()) && !A->IsPendingKill())
				{
					DebuggingReplicator = Cast<AGameplayDebuggingReplicator>(A);
					if (DebuggingReplicator && !DebuggingReplicator->IsGlobalInWorld() && DebuggingReplicator->GetLocalPlayerOwner() == MyPC)
					{
						break;
					}
				}
			}

			UObject* Target = DebuggingReplicator != NULL ? DebuggingReplicator->GetSelectedActorToDebug() : NULL;
			FString QueryName = FParse::Token(Cmd, 0);
			if (Target)
			{
				AISys->RunEQS(QueryName, Target);
			}
			else
			{
				MyPC->ClientMessage(TEXT("No debugging target to run EQS"));
			}
		}
	}

	return bHandled;
}

#endif // !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
