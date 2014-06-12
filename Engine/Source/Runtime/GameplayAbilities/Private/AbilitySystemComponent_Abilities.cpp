// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// ActorComponent.cpp: Actor component implementation.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetActor.h"

#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

#define LOCTEXT_NAMESPACE "AbilitySystemComponent"

/** Enable to log out all render state create, destroy and updatetransform events */
#define LOG_RENDER_STATE 0

void UAbilitySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	InitAbilityActorInfo();
	
	// Look for DSO AttributeSets
	AActor *Owner = GetOwner();

	TArray<UObject*> ChildObjects;
	GetObjectsWithOuter(Owner, ChildObjects, false, RF_PendingKill);
	for (UObject * Obj : ChildObjects)
	{
		UAttributeSet * Set = Cast<UAttributeSet>(Obj);
		if (Set)  
		{
			UObject * AT = Set->GetArchetype();

			//ensure(SpawnedAttributes.Contains(Set) == false);
			//ensure(Set->IsDefaultSubobject());
			SpawnedAttributes.Add(Set);
		}
	}
}

void UAbilitySystemComponent::InitAbilityActorInfo()
{
	if (!AbilityActorInfo.IsValid())
	{
		// Alloc (and init) a new actor info
		AbilityActorInfo = TSharedPtr<FGameplayAbilityActorInfo>(UAbilitySystemGlobals::Get().AllocAbilityActorInfo(GetOwner()));
	}
	else
	{
		// We already have a valid actor info, just reinit it.
		AbilityActorInfo->InitFromActor(GetOwner());
	}
}

UGameplayAbility * UAbilitySystemComponent::CreateNewInstanceOfAbility(UGameplayAbility *Ability)
{
	check(Ability);
	check(Ability->HasAllFlags(RF_ClassDefaultObject));

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	UGameplayAbility * AbilityInstance = ConstructObject<UGameplayAbility>(Ability->GetClass(), OwnerActor);
	check(AbilityInstance);

	// Add it to one of our instance lists so that it doesn't GC.
	if (AbilityInstance->GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNone)
	{
		ReplicatedInstancedAbilities.Add(AbilityInstance);
	}
	else
	{
		NonReplicatedInstancedAbilities.Add(AbilityInstance);
	}
	
	return AbilityInstance;
}

bool UAbilitySystemComponent::ActivateAbility(TWeakObjectPtr<UGameplayAbility> Ability)
{
	check(AbilityActorInfo.IsValid());

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	if (Ability.IsValid())
	{
		Ability.Get()->InputPressed(0, AbilityActorInfo.Get());
	}

	return false;
}

void UAbilitySystemComponent::CancelAbilitiesWithTags(const FGameplayTagContainer Tags, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, UGameplayAbility* Ignore)
{
	/**
	 *	FIXME
	 *
	 *	Right now we are canceling all activatable abilities that match Tags. This includes abilities that might not have been activated in the first place!
	 *	For instanced-per-actor abilities this is fine. They could check if they were activated/still activating.
	 *	For non-instanced abilities it is ambiguous. We have no way to know 'how many' non-instanced abilities are in flight.
	 *	Likewise for instanced-per-execution abilities. Though they are present in Replicated/NonReplicatedInstancedAbilities list
	 */

	struct local
	{
		static void CancelAbilitiesWithTags(const FGameplayTagContainer InTags, TArray<UGameplayAbility*> Abilities, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo, UGameplayAbility* InIgnore)
		{
			for (int32 idx=0; idx < Abilities.Num(); ++idx)
			{
				UGameplayAbility *Ability = Abilities[idx];
				if (Ability && (Ability != InIgnore) && Ability->AbilityTags.MatchesAny(InTags, false))
				{
					Ability->CancelAbility(InActorInfo, InActivationInfo);
					if (!Ability->HasAnyFlags(RF_ClassDefaultObject) && Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
					{
						Ability->MarkPendingKill();
						Abilities.RemoveAtSwap(idx);
						idx--;
					}
				}
			}
		}
	};

	//local::CancelAbilitiesWithTags(Tags, ReplicatedInstancedAbilities, ActorInfo, Ignore);
	//local::CancelAbilitiesWithTags(Tags, NonReplicatedInstancedAbilities, ActorInfo, Ignore);

	local::CancelAbilitiesWithTags(Tags, ActivatableAbilities, ActorInfo, ActivationInfo, Ignore);
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
int32 DenyClientActivation = 0;
static FAutoConsoleVariableRef CVarDenyClientActivation(
TEXT("AbilitySystem.DenyClientActivations"),
	DenyClientActivation,
	TEXT("Make server deny the next X ability activations from clients. For testing misprediction."),
	ECVF_Default
	);
#endif

void UAbilitySystemComponent::ServerTryActivateAbility_Implementation(UGameplayAbility * AbilityToActivate, int32 PredictionKey)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (DenyClientActivation > 0)
	{
		DenyClientActivation--;
		ClientActivateAbilityFailed(AbilityToActivate, PredictionKey);
		return;
	}
#endif

	ensure(AbilityToActivate);
	ensure(AbilityActorInfo.IsValid());

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	UGameplayAbility *InstancedAbility = NULL;

	if (AbilityToActivate->TryActivateAbility(AbilityActorInfo.Get(), PredictionKey, &InstancedAbility))
	{
		if (InstancedAbility && InstancedAbility->GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNone)
		{
			InstancedAbility->ClientActivateAbilitySucceed(PredictionKey);
		}
		else
		{
			ClientActivateAbilitySucceed(AbilityToActivate, PredictionKey);
		}

		// Update our ReplicatedPredictionKey. When the client gets value, he will know his state (actor+all components/subobjects) are up to do date and he can
		// remove any necessary predictive work.

		if (PredictionKey > 0)
		{
			ensure(PredictionKey > ReplicatedPredictionKey);
			ReplicatedPredictionKey = PredictionKey;
		}
	}
	else
	{
		ClientActivateAbilityFailed(AbilityToActivate, PredictionKey);
	}
}

bool UAbilitySystemComponent::ServerTryActivateAbility_Validate(UGameplayAbility * AbilityToActivate, int32 PredictionKey)
{
	return true;
}

void UAbilitySystemComponent::ClientActivateAbilityFailed_Implementation(UGameplayAbility * AbilityToActivate, int32 PredictionKey)
{
	if (PredictionKey > 0)
	{
		for (int32 idx = 0; idx < PredictionDelegates.Num(); ++idx)
		{
			int32 Key = PredictionDelegates[idx].Key;
			if (Key == PredictionKey)
			{
				ABILITY_LOG(Warning, TEXT("Failed ActivateAbility, clearing prediction data %d"), PredictionKey);

				PredictionDelegates[idx].Value.Broadcast();
				PredictionDelegates.RemoveAt(idx);
				break;
			}
			else if(Key >= PredictionKey)
			{
				break;
			}
		}
	}
}

void UAbilitySystemComponent::ClientActivateAbilitySucceed_Implementation(UGameplayAbility* AbilityToActivate, int32 PredictionKey)
{
	ensure(AbilityToActivate);
	ensure(AbilityActorInfo.IsValid());

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	// Fixme: We need a better way to link up/reconcile preditive replicated abilities.
	if (AbilityToActivate->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::InstancedPerExecution)
	{
		FGameplayAbilityActivationInfo	ActivationInfo(EGameplayAbilityActivationMode::Confirmed, PredictionKey);
		AbilityToActivate->CallActivateAbility(AbilityActorInfo.Get(), ActivationInfo);
	}
}

void UAbilitySystemComponent::MontageBranchPoint_AbilityDecisionStop()
{
	if (AnimatingAbility)
	{
		AActor * OwnerActor = GetOwner();
		check(OwnerActor);

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(OwnerActor);

		AnimatingAbility->MontageBranchPoint_AbilityDecisionStop(AbilityActorInfo.Get());
	}
}

void UAbilitySystemComponent::MontageBranchPoint_AbilityDecisionStart()
{
	if (AnimatingAbility)
	{
		AActor * OwnerActor = GetOwner();
		check(OwnerActor);

		AnimatingAbility->MontageBranchPoint_AbilityDecisionStart(AbilityActorInfo.Get());
	}
}

bool UAbilitySystemComponent::GetUserAbilityActivationInhibited() const
{
	return UserAbilityActivationInhibited;
}

void UAbilitySystemComponent::SetUserAbilityActivationInhibited(bool NewInhibit)
{
	if(AbilityActorInfo->IsLocallyControlled())
	{
		if (NewInhibit && UserAbilityActivationInhibited)
		{
			// This could cause problems if two sources try to inhibit ability activation, it is not clear when the ability should be uninhibited
			ABILITY_LOG(Warning, TEXT("Call to SetUserAbilityActivationInhibited(true) when UserAbilityActivationInhibited was already true"));
		}

		UserAbilityActivationInhibited = NewInhibit;
	}
}

// --------------------------------------------------------------------------

void UAbilitySystemComponent::BindToInputComponent(UInputComponent *InputComponent)
{
	static const FName ConfirmBindName(TEXT("AbilityConfirm"));
	static const FName CancelBindName(TEXT("AbilityCancel"));

	// Pressed event
	{
		FInputActionBinding AB(ConfirmBindName, IE_Pressed);
		AB.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &UAbilitySystemComponent::InputConfirm);
		InputComponent->AddActionBinding(AB);
	}

	// 
	{
		FInputActionBinding AB(CancelBindName, IE_Pressed);
		AB.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &UAbilitySystemComponent::InputCancel);
		InputComponent->AddActionBinding(AB);
	}
}

void UAbilitySystemComponent::InputConfirm()
{
	if (GetOwnerRole() != ROLE_Authority)
	{
		// Tell the server we confirmed input
		// FIXME TODO: we don't need to do this everytime! just when there is an activae ability waiting for this!
		// 
		ServerSetReplicatedConfirm(true);
	}
	
	ConfirmCallbacks.Broadcast();
}

void UAbilitySystemComponent::InputCancel()
{
	CancelCallbacks.Broadcast();
}

void UAbilitySystemComponent::TargetConfirm()
{
	for (AGameplayAbilityTargetActor* TargetActor : SpawnedTargetActors)
	{
		if (TargetActor)
		{
			TargetActor->ConfirmTargeting();
		}
	}

	SpawnedTargetActors.Empty();
}

void UAbilitySystemComponent::TargetCancel()
{
	for (AGameplayAbilityTargetActor* TargetActor : SpawnedTargetActors)
	{
		if (TargetActor)
		{
			TargetActor->CancelTargeting();
		}
	}

	SpawnedTargetActors.Empty();
}

// --------------------------------------------------------------------------

void UAbilitySystemComponent::ServerSetReplicatedConfirm_Implementation(bool Confirmed)
{
	ReplicatedConfirmAbility = Confirmed;
	ConfirmCallbacks.Broadcast();
}

bool UAbilitySystemComponent::ServerSetReplicatedConfirm_Validate(bool Confirmed)
{
	return true;
}

// -------

void UAbilitySystemComponent::ServerSetReplicatedTargetData_Implementation(FGameplayAbilityTargetDataHandle Confirmed)
{
	ReplicatedTargetData = Confirmed;
	ReplicatedTargetDataDelegate.Broadcast(ReplicatedTargetData);
}

bool UAbilitySystemComponent::ServerSetReplicatedTargetData_Validate(FGameplayAbilityTargetDataHandle Confirmed)
{
	return true;
}

// -------

void UAbilitySystemComponent::ServerSetReplicatedTargetDataCancelled_Implementation()
{
	ReplicatedTargetDataCancelledDelegate.Broadcast();
}

bool UAbilitySystemComponent::ServerSetReplicatedTargetDataCancelled_Validate()
{
	return true;
}

// -------

void UAbilitySystemComponent::SetTargetAbility(UGameplayAbility* NewTargetingAbility)
{
	TargetingAbility = NewTargetingAbility;

	//ReplicatedConfirmAbility = false;
	//ReplicatedTargetData.Clear();
}

void UAbilitySystemComponent::ConsumeAbilityConfirm()
{
	ReplicatedConfirmAbility = false;
}

void UAbilitySystemComponent::ConsumeAbilityTargetData()
{
	ReplicatedTargetData.Clear();
}


#undef LOCTEXT_NAMESPACE

