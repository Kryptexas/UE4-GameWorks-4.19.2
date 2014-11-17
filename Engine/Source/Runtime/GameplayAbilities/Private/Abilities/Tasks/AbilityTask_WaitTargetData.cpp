
#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetActor.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"

UAbilityTask_WaitTargetData::UAbilityTask_WaitTargetData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	
}

UAbilityTask_WaitTargetData* UAbilityTask_WaitTargetData::WaitTargetData(UObject* WorldContextObject, TSubclassOf<AGameplayAbilityTargetActor> InTargetClass)
{
	UAbilityTask_WaitTargetData * MyObj = NewObject<UAbilityTask_WaitTargetData>();
	UGameplayAbility * ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	MyObj->InitTask(ThisAbility);
	MyObj->TargetClass = InTargetClass;
	return MyObj;	
}

void UAbilityTask_WaitTargetData::Activate()
{
	if (Ability.IsValid())
	{
		if (!Ability.Get()->GetCurrentActorInfo()->IsLocallyControlled())
		{
			// If not locally controlled (server for remote client), see if TargetData was already sent
			// else register callback for when it does get here

			if (AbilitySystemComponent->ReplicatedTargetData.IsValid())
			{
				ValidData.Broadcast(AbilitySystemComponent->ReplicatedTargetData);
				Cleanup();
				return;
			}
			else
			{
				AbilitySystemComponent->ReplicatedTargetDataDelegate.AddUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataReplicatedCallback);
				AbilitySystemComponent->ReplicatedTargetDataCancelledDelegate.AddDynamic(this, &UAbilityTask_WaitTargetData::OnTargetDataReplicatedCancelledCallback);
				return;
			}
		}
		else
		{
			// Locally controlled - spawn the targeting actor.
			AGameplayAbilityTargetActor* CDO = CastChecked<AGameplayAbilityTargetActor>(TargetClass->GetDefaultObject());
			if (CDO->StaticTargetFunction)
			{
				// This is just a static function that should instantly give us back target data
				FGameplayAbilityTargetDataHandle Data = CDO->StaticGetTargetData(Ability->GetWorld(), Ability->GetCurrentActorInfo(), Ability->GetCurrentActivationInfo());
				OnTargetDataReadyCallback(Data);
			}
			else
			{
				// This is a latent thing, probably with visuals and other gameplay related stuff. Spawn an actor locally and it will tell us when it has TargetData.
				AGameplayAbilityTargetActor* SpawnedActor = CastChecked<AGameplayAbilityTargetActor>(Ability->GetWorld()->SpawnActor(TargetClass));
				SpawnedActor->TargetDataReadyDelegate.AddUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataReadyCallback);
				SpawnedActor->CanceledDelegate.AddUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataCancelledCallback);
								
				// User ability activation is inhibited while this is active
				AbilitySystemComponent->SetUserAbilityActivationInhibited(true);
				AbilitySystemComponent->SpawnedTargetActors.Push(SpawnedActor);

				SpawnedActor->StartTargeting(Ability.Get());

				MySpawnedTargetActor = SpawnedActor;
			}
		}
	}
}

/** Valid TargetData was replicated to use (we are server, was sent from client) */
void UAbilityTask_WaitTargetData::OnTargetDataReplicatedCallback(FGameplayAbilityTargetDataHandle Data)
{
	check(AbilitySystemComponent.IsValid());

	ValidData.Broadcast(Data);

	Cleanup();
}

/** Client cancelled this Targeting Task (we are the server */
void UAbilityTask_WaitTargetData::OnTargetDataReplicatedCancelledCallback()
{
	check(AbilitySystemComponent.IsValid());

	Cancelled.Broadcast(FGameplayAbilityTargetDataHandle());
	
	Cleanup();
}

/** The TargetActor we spawned locally has called back with valid target data */
void UAbilityTask_WaitTargetData::OnTargetDataReadyCallback(FGameplayAbilityTargetDataHandle Data)
{
	check(AbilitySystemComponent.IsValid());

	AbilitySystemComponent->ServerSetReplicatedTargetData(Data);
	ValidData.Broadcast(Data);
	
	Cleanup();
}

/** The TargetActor we spawned locally has called back with a cancel event (they still include the 'last/best' targetdata but the consumer of this may want to discard it) */
void UAbilityTask_WaitTargetData::OnTargetDataCancelledCallback(FGameplayAbilityTargetDataHandle Data)
{
	check(AbilitySystemComponent.IsValid());

	AbilitySystemComponent->ServerSetReplicatedTargetDataCancelled();
	Cancelled.Broadcast(Data);

	Cleanup();
}

void UAbilityTask_WaitTargetData::Cleanup()
{
	AbilitySystemComponent->ConsumeAbilityTargetData();
	AbilitySystemComponent->SetUserAbilityActivationInhibited(false);

	AbilitySystemComponent->ReplicatedTargetDataDelegate.RemoveUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataReplicatedCallback);
	AbilitySystemComponent->ReplicatedTargetDataCancelledDelegate.RemoveDynamic(this, &UAbilityTask_WaitTargetData::OnTargetDataReplicatedCancelledCallback);

	if (MySpawnedTargetActor.IsValid())
	{
		MySpawnedTargetActor->Destroy();
	}

	MarkPendingKill();
}