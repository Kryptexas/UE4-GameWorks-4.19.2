
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
	UAbilityTask_SpawnActor * MyObj = NewObject<UAbilityTask_SpawnActor>();
	UGameplayAbility * ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	MyObj->InitTask(ThisAbility);
	return MyObj;	
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
		FTransform SpawnTransform = AbilitySystemComponent->GetOwner()->GetTransform();

		SpawnedActor->ExecuteConstruction(SpawnTransform, NULL);
		SpawnedActor->PostActorConstruction();

		Success.Broadcast(SpawnedActor);
	}

	MarkPendingKill();
}

// ---------------------------------------------------------------------------------------

