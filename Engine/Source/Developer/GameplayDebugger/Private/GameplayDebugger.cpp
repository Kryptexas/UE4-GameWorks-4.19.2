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
	void OnLevelActorAdded(AActor* InActor);
	void OnLevelActorDeleted(AActor* InActor);
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
		GEngine->OnLevelActorAdded().AddRaw(this, &FGameplayDebugger::OnLevelActorAdded);
		GEngine->OnLevelActorDeleted().AddRaw(this, &FGameplayDebugger::OnLevelActorDeleted);
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
		GEngine->OnLevelActorAdded().RemoveAll(this);
		GEngine->OnLevelActorDeleted().RemoveAll(this);
	}
#endif
}

void FGameplayDebugger::WorldAdded(UWorld* InWorld)
{

}

void FGameplayDebugger::WorldDestroyed(UWorld* InWorld)
{

}

void FGameplayDebugger::OnLevelActorAdded(AActor* InActor)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	APlayerController* PC = Cast<APlayerController>(InActor);
	if (PC && PC->GetNetMode() < ENetMode::NM_Client)
	{
		UWorld* World = PC->GetWorld();
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.bNoCollisionFail = true;
		SpawnInfo.Name = *FString::Printf(TEXT("GameplayDebuggingReplicator_%s"), *PC->GetName());
		AGameplayDebuggingReplicator *DestActor = World->SpawnActor<AGameplayDebuggingReplicator>(SpawnInfo);
		DestActor->SetLocalPlayerOwner(PC);
		DestActor->SetReplicates(true);
	}
#endif
}

void FGameplayDebugger::OnLevelActorDeleted(AActor* InActor)
{

}
