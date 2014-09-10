// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#include "Misc/CoreMisc.h"
#include "GameplayDebuggingComponent.h"
#include "GameplayDebuggingHUDComponent.h"
#include "GameplayDebuggingReplicator.h"
#include "GameplayDebuggingControllerComponent.h"
#include "AISystem.h"

uint32 FGameplayDebuggerSettings::ShowFlagIndex = 0;

FGameplayDebuggerSettings GameplayDebuggerSettings(class AGameplayDebuggingReplicator* Replicator)
{
	static uint32 Settings = (1 << EAIDebugDrawDataView::Basic) | (1 << EAIDebugDrawDataView::OverHead);
	return FGameplayDebuggerSettings(Replicator == NULL ? Settings : Replicator->DebuggerShowFlags);
}

class FGameplayDebugger : public FSelfRegisteringExec, public IGameplayDebugger
{
public:
	// Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// End IModuleInterface

	void WorldAdded(UWorld* InWorld);
	void WorldDestroyed(UWorld* InWorld);
#if WITH_EDITOR
	void OnLevelActorAdded(AActor* InActor);
	void OnLevelActorDeleted(AActor* InActor);
#endif

	TArray<TWeakObjectPtr<AGameplayDebuggingReplicator> >& GetAllReplicators(UWorld* InWorld);
	void AddReplicator(UWorld* InWorld, AGameplayDebuggingReplicator* InReplicator);
	void RemoveReplicator(UWorld* InWorld, AGameplayDebuggingReplicator* InReplicator);

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	// End FExec Interface

private:
	virtual bool CreateGameplayDebuggerForPlayerController(APlayerController* PlayerController) override;

	bool DoesGameplayDebuggingReplicatorExistForPlayerController(APlayerController* PlayerController);

	TMap<TWeakObjectPtr<UWorld>, TArray<TWeakObjectPtr<AGameplayDebuggingReplicator> > > AllReplilcatorsPerWorlds;
};

IMPLEMENT_MODULE(FGameplayDebugger, GameplayDebugger)

// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)
void FGameplayDebugger::StartupModule()
{ 
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (GEngine)
	{
		GEngine->OnWorldAdded().AddRaw(this, &FGameplayDebugger::WorldAdded);
		GEngine->OnWorldDestroyed().AddRaw(this, &FGameplayDebugger::WorldDestroyed);
#if WITH_EDITOR
		GEngine->OnLevelActorAdded().AddRaw(this, &FGameplayDebugger::OnLevelActorAdded);
		GEngine->OnLevelActorDeleted().AddRaw(this, &FGameplayDebugger::OnLevelActorDeleted);
#endif
	}
#endif
}

// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
// we call this function before unloading the module.
void FGameplayDebugger::ShutdownModule()
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (GEngine)
	{
		GEngine->OnWorldAdded().RemoveAll(this);
		GEngine->OnWorldDestroyed().RemoveAll(this);
#if WITH_EDITOR
		GEngine->OnLevelActorAdded().RemoveAll(this);
		GEngine->OnLevelActorDeleted().RemoveAll(this);
#endif
	}
#endif
}

bool FGameplayDebugger::DoesGameplayDebuggingReplicatorExistForPlayerController(APlayerController* PlayerController)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (PlayerController == NULL)
	{
		return false;
	}

	UWorld* World = PlayerController->GetWorld();
	if (World == NULL)
	{
		return false;
	}

	for (auto It = GetAllReplicators(World).CreateConstIterator(); It; ++It)
	{
		TWeakObjectPtr<AGameplayDebuggingReplicator> Replicator = *It;
		if (Replicator.IsValid())
		{
			if (Replicator->GetLocalPlayerOwner() == PlayerController)
			{
				return true;
			}
		}
	}
#endif

	return false;
}

bool FGameplayDebugger::CreateGameplayDebuggerForPlayerController(APlayerController* PlayerController)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (PlayerController == NULL)
	{
		return false;
	}

	bool bIsServer = PlayerController->GetNetMode() < ENetMode::NM_Client; // (Only create on some sort of server)
	if (!bIsServer)
	{
		return false;
	}

	UWorld* World = PlayerController->GetWorld();
	if (World == NULL)
	{
		return false;
	}

	if (DoesGameplayDebuggingReplicatorExistForPlayerController(PlayerController))
	{
		// No need to create one if we already have one.
		return false;
	}

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.bNoCollisionFail = true;
	SpawnInfo.Name = *FString::Printf(TEXT("GameplayDebuggingReplicator_%s"), *PlayerController->GetName());
	AGameplayDebuggingReplicator* DestActor = World->SpawnActor<AGameplayDebuggingReplicator>(SpawnInfo);
	if (DestActor != NULL)
	{
		DestActor->SetLocalPlayerOwner(PlayerController);
		DestActor->SetReplicates(true);
		DestActor->SetAsGlobalInWorld(false);
		AddReplicator(World, DestActor);
		return true;
	}
#endif
	
	return false;
}

TArray<TWeakObjectPtr<AGameplayDebuggingReplicator> >& FGameplayDebugger::GetAllReplicators(UWorld* InWorld)
{
	return AllReplilcatorsPerWorlds.FindOrAdd(InWorld);
}

void FGameplayDebugger::AddReplicator(UWorld* InWorld, AGameplayDebuggingReplicator* InReplicator)
{
	GetAllReplicators(InWorld).Add(InReplicator);
}

void FGameplayDebugger::RemoveReplicator(UWorld* InWorld, AGameplayDebuggingReplicator* InReplicator)
{
	GetAllReplicators(InWorld).RemoveSwap(InReplicator);
}

void FGameplayDebugger::WorldAdded(UWorld* InWorld)
{
	bool bIsServer = InWorld && InWorld->GetNetMode() < ENetMode::NM_Client; // (Only server code)
	if (!bIsServer)
	{
		return;
	}

	for (auto It = GetAllReplicators(InWorld).CreateConstIterator(); It; ++It)
	{
		TWeakObjectPtr<AGameplayDebuggingReplicator> Replicator = *It;
		if (Replicator.IsValid() && Replicator->IsGlobalInWorld())
		{
			// Ok, we have global replicator on level
			return;
		}
	}

	// create global replicator on level
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.bNoCollisionFail = true;
	SpawnInfo.Name = *FString::Printf(TEXT("GameplayDebuggingReplicator_Global"));
	AGameplayDebuggingReplicator* DestActor = InWorld->SpawnActor<AGameplayDebuggingReplicator>(SpawnInfo);
	if (DestActor != NULL)
	{
		DestActor->SetLocalPlayerOwner(NULL);
		DestActor->SetReplicates(false);
		DestActor->SetActorTickEnabled(true);
		DestActor->SetAsGlobalInWorld(true);
		AddReplicator(InWorld, DestActor);
	}
}

void FGameplayDebugger::WorldDestroyed(UWorld* InWorld)
{
	bool bIsServer = InWorld && InWorld->GetNetMode() < ENetMode::NM_Client; // (Only work on  server)
	if (!bIsServer)
	{
		return;
	}

	// remove global replicator from level
	AllReplilcatorsPerWorlds.Remove(InWorld);
}
#if WITH_EDITOR
void FGameplayDebugger::OnLevelActorAdded(AActor* InActor)
{
	// This function doesn't help much, because it's only called in EDITOR!
	// We need a function that is called in the game!  So instead of creating it automatically, I'm leaving it
	// to be created explicitly by any player controller that needs to create it.
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* PC = Cast<APlayerController>(InActor);
	if (PC && PC->GetNetMode() < ENetMode::NM_Client)
	{
		CreateGameplayDebuggerForPlayerController(PC);
	}
#endif
}

void FGameplayDebugger::OnLevelActorDeleted(AActor* InActor)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* PC = Cast<APlayerController>(InActor);
	if (!PC)
	{
		return;
	}

	UWorld* World = PC->GetWorld();
	if (!World)
	{
		return;
	}

	for (auto It = GetAllReplicators(World).CreateConstIterator(); It; ++It)
	{
		TWeakObjectPtr<AGameplayDebuggingReplicator> Replicator = *It;
		if (Replicator != NULL)
		{
			if (Replicator->GetLocalPlayerOwner() == PC)
			{
				RemoveReplicator(World, Replicator.Get());
				World->DestroyActor(Replicator.Get());
			}
		}
	}
#endif
}
#endif //WITH_EDITOR

bool FGameplayDebugger::Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	bool bHandled = false;

	APlayerController* PC = Inworld ? Inworld->GetFirstPlayerController() : NULL;
	if (!Inworld || !PC)
	{
		return bHandled;
	}

	if (FParse::Command(&Cmd, TEXT("EnableGDT")))
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

		if (Inworld->GetNetMode() == NM_Client)
		{
			if (!Replicator)
			{
				PC->ConsoleCommand("cheat EnableGDT");
			}
			else if (!Replicator->IsToolCreated())
			{
				Replicator->CreateTool();
				Replicator->EnableTool();
				bHandled = true;
			}
		}
		else if (Inworld->GetNetMode() < NM_Client)
		{
			if (!Replicator)
			{
				CreateGameplayDebuggerForPlayerController(PC);
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
			}

			if (Inworld->GetNetMode() != NM_DedicatedServer)
			{
				if (Replicator && !Replicator->IsToolCreated())
				{
					Replicator->CreateTool();
					Replicator->EnableTool();
					bHandled = true;
				}
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

