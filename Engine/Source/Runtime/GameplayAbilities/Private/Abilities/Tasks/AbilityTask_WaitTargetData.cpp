
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

// ---------------------------------------------------------------------------------------

bool UAbilityTask_WaitTargetData::BeginSpawningActor(UObject* WorldContextObject, TSubclassOf<AGameplayAbilityTargetActor> TargetClass, AGameplayAbilityTargetActor*& SpawnedActor)
{
	SpawnedActor = nullptr;

	UGameplayAbility* MyAbility = Ability.Get();
	if (MyAbility)
	{
		const AGameplayAbilityTargetActor* CDO = CastChecked<AGameplayAbilityTargetActor>(TargetClass->GetDefaultObject());
		MyTargetActor = CDO;

		bool Replicates = CDO->GetReplicates();
		bool StaticFunc = CDO->StaticTargetFunction;

		if (Replicates && StaticFunc)
		{
			ABILITY_LOG(Fatal, TEXT("AbilityTargetActor class %s can't be Replicating and Static"), *TargetClass->GetName());
			Replicates = false;
		}

		// Spawn the actor if this is a locally controlled ability (always) or if this is a replicated targeting mode.
		if (Replicates || MyAbility->GetCurrentActorInfo()->IsLocallyControlled())
		{
			if (StaticFunc)
			{
				// This is just a static function that should instantly give us back target data
				FGameplayAbilityTargetDataHandle Data = CDO->StaticGetTargetData(MyAbility->GetWorld(), MyAbility->GetCurrentActorInfo(), MyAbility->GetCurrentActivationInfo());
				OnTargetDataReadyCallback(Data);
			}
			else
			{
				UClass* Class = *TargetClass;
				if (Class != NULL)
				{
					UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject);
					SpawnedActor = World->SpawnActorDeferred<AGameplayAbilityTargetActor>(Class, FVector::ZeroVector, FRotator::ZeroRotator, NULL, NULL, true);
				}

				SpawnedActor->TargetDataReadyDelegate.AddUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataReadyCallback);
				SpawnedActor->CanceledDelegate.AddUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataCancelledCallback);

				MyTargetActor = SpawnedActor;

				AGameplayAbilityTargetActor* TargetActor = CastChecked<AGameplayAbilityTargetActor>(SpawnedActor);
				if (TargetActor)
				{
					TargetActor->MasterPC = MyAbility->GetCurrentActorInfo()->PlayerController.Get();
				}
			}
		}
		else
		{
			// If not locally controlled (server for remote client), see if TargetData was already sent
			// else register callback for when it does get here

			if (AbilitySystemComponent->ReplicatedTargetData.IsValid())
			{
				ValidData.Broadcast(AbilitySystemComponent->ReplicatedTargetData);
				Cleanup();
			}
			else
			{
				AbilitySystemComponent->ReplicatedTargetDataDelegate.AddUObject(this, &UAbilityTask_WaitTargetData::OnTargetDataReplicatedCallback);
				AbilitySystemComponent->ReplicatedTargetDataCancelledDelegate.AddDynamic(this, &UAbilityTask_WaitTargetData::OnTargetDataReplicatedCancelledCallback);
			}
		}
	}
	return (SpawnedActor != nullptr);
}

void UAbilityTask_WaitTargetData::FinishSpawningActor(UObject* WorldContextObject, AGameplayAbilityTargetActor* SpawnedActor)
{
	if (SpawnedActor)
	{
		check(MyTargetActor == SpawnedActor);

		FTransform SpawnTransform = AbilitySystemComponent->GetOwner()->GetTransform();

		SpawnedActor->ExecuteConstruction(SpawnTransform, NULL);
		SpawnedActor->PostActorConstruction();

		// User ability activation is inhibited while this is active
		AbilitySystemComponent->SetUserAbilityActivationInhibited(true);
		AbilitySystemComponent->SpawnedTargetActors.Push(SpawnedActor);

		SpawnedActor->StartTargeting(Ability.Get());
	}
}

// ---------------------------------------------------------------------------------------

/** Valid TargetData was replicated to use (we are server, was sent from client) */
void UAbilityTask_WaitTargetData::OnTargetDataReplicatedCallback(FGameplayAbilityTargetDataHandle Data)
{
	check(AbilitySystemComponent.IsValid());

	/** 
	 *  Call into the TargetActor to sanitize/verify the data. If this returns false, we are rejecting
	 *	the replicated target data and will treat this as a cancel.
	 *	
	 *	This can also be used for bandwidth optimizations. OnReplicatedTargetDataReceived could do an actual
	 *	trace/check/whatever server side and use that data. So rather than having the client send that data
	 *	explicitly, the client is basically just sending a 'confirm' and the server is now going to do the work
	 *	in OnReplicatedTargetDataReceived.
	 */
	if (MyTargetActor.IsValid() && !MyTargetActor->OnReplicatedTargetDataReceived(Data))
	{
		Cancelled.Broadcast(Data);
	}
	else
	{
		ValidData.Broadcast(Data);
	}

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

	if (MyTargetActor.IsValid() && !MyTargetActor->HasAnyFlags(RF_ClassDefaultObject))
	{
		MyTargetActor->Destroy();
	}

	MarkPendingKill();
}


// --------------------------------------------------------------------------------------

