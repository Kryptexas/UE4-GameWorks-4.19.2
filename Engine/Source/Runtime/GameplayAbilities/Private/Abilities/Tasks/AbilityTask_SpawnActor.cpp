
#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetActor.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_SpawnActor.h"


UAbilityTask_SpawnActor::UAbilityTask_SpawnActor(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
}

UAbilityTask_SpawnActor* UAbilityTask_SpawnActor::SpawnActor(UObject* WorldContextObject, TSubclassOf<AActor> InClass)
{
	return NewTask<UAbilityTask_SpawnActor>(WorldContextObject);
}

// ---------------------------------------------------------------------------------------

bool UAbilityTask_SpawnActor::BeginSpawningActor(UObject* WorldContextObject, TSubclassOf<AActor> InClass, AActor*& SpawnedActor)
{
	if (Ability.IsValid() && Ability.Get()->GetCurrentActorInfo()->IsNetAuthority())
	{
		UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
		SpawnedActor = World->SpawnActorDeferred<AActor>(InClass, FVector::ZeroVector, FRotator::ZeroRotator, NULL, NULL, true);
	}
	
	return (SpawnedActor != nullptr);
}

void UAbilityTask_SpawnActor::FinishSpawningActor(UObject* WorldContextObject, AActor* SpawnedActor)
{
	if (SpawnedActor)
	{
		const FTransform SpawnTransform = AbilitySystemComponent->GetOwner()->GetTransform();

		SpawnedActor->FinishSpawning(SpawnTransform);

		Success.Broadcast(SpawnedActor);
	}

	EndTask();
}

// ---------------------------------------------------------------------------------------

