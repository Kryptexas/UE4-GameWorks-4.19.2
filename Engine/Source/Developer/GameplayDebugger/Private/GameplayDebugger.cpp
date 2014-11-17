// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "GameplayDebuggerPrivate.h"
#include "GameplayDebuggingComponent.h"
#include "GameplayDebuggingHUDComponent.h"
#include "GameplayDebuggingReplicator.h"

uint32 FGameplayDebuggerSettings::ShowFlagIndex = 0;

FGameplayDebuggerSettings GameplayDebuggerSettings(class AGameplayDebuggingReplicator* Replicator)
{
	static uint32 Settings = (1 << EAIDebugDrawDataView::Basic) | (1 << EAIDebugDrawDataView::OverHead);
	return FGameplayDebuggerSettings(Replicator == NULL ? Settings : Replicator->DebuggerShowFlags);
}

class FGameplayDebugger : public IGameplayDebugger
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

private:
	virtual bool CreateGameplayDebuggerForPlayerController(APlayerController* PlayerController) const override;

	bool DoesGameplayDebuggingReplicatorExistForPlayerController(APlayerController* PlayerController) const;
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

void FGameplayDebugger::WorldAdded(UWorld* InWorld)
{
}

bool FGameplayDebugger::DoesGameplayDebuggingReplicatorExistForPlayerController(APlayerController* PlayerController) const
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

	for (TActorIterator<AGameplayDebuggingReplicator> It(World); It; ++It)
	{
		AGameplayDebuggingReplicator* Replicator = *It;
		if (Replicator != NULL)
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

bool FGameplayDebugger::CreateGameplayDebuggerForPlayerController(APlayerController* PlayerController) const
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

		return true;
	}
#endif
	
	return false;
}

void FGameplayDebugger::WorldDestroyed(UWorld* InWorld)
{

}
#if WITH_EDITOR
void FGameplayDebugger::OnLevelActorAdded(AActor* InActor)
{
	// This function doesn't help much, because it's only called in EDITOR!
	// We need a function that is called in the game!  So instead of creating it automatically, I'm leaving it
	// to be created explicitly by any player controller that needs to create it.
}

void FGameplayDebugger::OnLevelActorDeleted(AActor* InActor)
{

}
#endif
