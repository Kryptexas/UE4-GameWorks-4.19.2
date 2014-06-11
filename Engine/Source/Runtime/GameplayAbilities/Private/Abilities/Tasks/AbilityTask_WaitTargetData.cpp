
#include "AbilitySystemPrivatePCH.h"
#include "GameplayAbilityTargetActor.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"

UAbilityTask_WaitTargetData::UAbilityTask_WaitTargetData(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ASC = NULL;
}

UAbilityTask_WaitTargetData* UAbilityTask_WaitTargetData::WaitTargetData(UObject* WorldContextObject, TSubclassOf<AGameplayAbilityTargetActor> InTargetClass)
{
	UAbilityTask_WaitTargetData * MyObj = NewObject<UAbilityTask_WaitTargetData>();
	UGameplayAbility * ThisAbility = CastChecked<UGameplayAbility>(WorldContextObject);
	MyObj->Ability = ThisAbility;
	MyObj->ASC = ThisAbility->GetCurrentActorInfo()->AbilitySystemComponent.Get();
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

			if (ASC->ReplicatedTargetData.IsValid())
			{
				ValidData.Broadcast(ASC->ReplicatedTargetData);
				ASC->ConsumeAbilityTargetData();
				return;
			}
			else
			{
				ASC->ReplicatedTargetDataDelegate.AddUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataReplicatedCallback);
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

				SpawnedActor->StartTargeting(Ability.Get());
			}
		}
	}
}

void UAbilityTask_WaitTargetData::OnTargetDataReplicatedCallback(FGameplayAbilityTargetDataHandle Data)
{
	check(ASC);
	ValidData.Broadcast(Data);
	ASC->ConsumeAbilityTargetData();
}

void UAbilityTask_WaitTargetData::OnTargetDataReadyCallback(FGameplayAbilityTargetDataHandle Data)
{
	check(ASC);
	ASC->ServerSetReplicatedTargetData(Data);
	ValidData.Broadcast(Data);
	ASC->ConsumeAbilityTargetData();
}