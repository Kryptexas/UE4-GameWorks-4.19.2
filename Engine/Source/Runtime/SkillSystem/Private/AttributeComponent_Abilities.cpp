// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// ActorComponent.cpp: Actor component implementation.

#include "SkillSystemModulePrivatePCH.h"
#include "AttributeComponent.h"
#include "Abilities/GameplayAbility.h"

#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

#define LOCTEXT_NAMESPACE "AttributeComponent"

/** Enable to log out all render state create, destroy and updatetransform events */
#define LOG_RENDER_STATE 0

void UAttributeComponent::InitializeComponent()
{
	Super::InitializeComponent();
	
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

UGameplayAbility * UAttributeComponent::CreateNewInstanceOfAbility(UGameplayAbility *Ability)
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

bool UAttributeComponent::ActivateAbility(int32 AbilityIdx)
{
	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);

	if (ActivatableAbilities.IsValidIndex(AbilityIdx))
	{
		// Fixme
		ActivatableAbilities[AbilityIdx]->InputPressed(0, ActorInfo);
	}

	return false;
}

void UAttributeComponent::CancelAbilitiesWithTags(const FGameplayTagContainer Tags, const FGameplayAbilityActorInfo & ActorInfo, UGameplayAbility * Ignore)
{
	/**
	 *	Right now we are cancelling all activatable abilites that match Tags. This includes abilities that might not have been activated in the first place!
	 *	For instanced-per-actor abilities this is fine. They could check if they were activated/still activating.
	 *	For non-instanced abilities it is ambigous. We have no way to know 'how many' non-instanced abilities are in flight.
	 *	Likewise for instanced-per-execution abilities. Though they are present in Replicated/NonReplicatedInstancedAbilities list
	 */

	struct local
	{
		static void CancelAbilitiesWithTags(const FGameplayTagContainer Tags, TArray<UGameplayAbility*> Abilities, const FGameplayAbilityActorInfo ActorInfo, UGameplayAbility * Ignore)
		{
			for (int32 idx=0; idx < Abilities.Num(); ++idx)
			{
				UGameplayAbility *Ability = Abilities[idx];
				if (Ability && (Ability != Ignore) && Ability->AbilityTags.MatchesAny(Tags, false))
				{
					Ability->CancelAbility(ActorInfo);
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

	local::CancelAbilitiesWithTags(Tags, ActivatableAbilities, ActorInfo, Ignore);
}

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
int32 DenyClientActivation = 0;
static FAutoConsoleVariableRef CVarDenyClientActivation(
	TEXT("SkillSystem.DenyClientActivations"),
	DenyClientActivation,
	TEXT("Make server deny the next X ability activations from clients. For testing misprediction."),
	ECVF_Default
	);
#endif

void UAttributeComponent::ServerTryActivateAbility_Implementation(class UGameplayAbility * AbilityToActivate, int32 PredictionKey)
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

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);
	ActorInfo.SetPredictionKey(PredictionKey);	// Set the prediction key in case anyone else needs it. But we are still Authority and not Predicting.

	UGameplayAbility *InstancedAbility = NULL;

	if (AbilityToActivate->TryActivateAbility(ActorInfo, &InstancedAbility))
	{
		if (InstancedAbility && InstancedAbility->GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNone)
		{
			InstancedAbility->ClientActivateAbilitySucceed(OwnerActor);
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

bool UAttributeComponent::ServerTryActivateAbility_Validate(class UGameplayAbility * AbilityToActivate, int32 PredictionKey)
{
	return true;
}

void UAttributeComponent::ClientActivateAbilityFailed_Implementation(class UGameplayAbility * AbilityToActivate, int32 PredictionKey)
{
	if (PredictionKey > 0)
	{
		for (int32 idx = 0; idx < PredictionDelegates.Num(); ++idx)
		{
			int32 Key = PredictionDelegates[idx].Key;
			if (Key == PredictionKey)
			{
				SKILL_LOG(Warning, TEXT("Failed ActivateAbility, clearing prediction data %d"), PredictionKey);

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

void UAttributeComponent::ClientActivateAbilitySucceed_Implementation(class UGameplayAbility * AbilityToActivate, int32 PredictionKey)
{
	ensure(AbilityToActivate);

	AActor * OwnerActor = GetOwner();
	check(OwnerActor);

	FGameplayAbilityActorInfo	ActorInfo;
	ActorInfo.InitFromActor(OwnerActor);
	ActorInfo.SetPredictionKey(PredictionKey);
	ActorInfo.SetActivationConfirmed();

	AbilityToActivate->CallActivateAbility(ActorInfo);
}

void UAttributeComponent::MontageBranchPoint_AbilityDecisionStop()
{
	if (AnimatingAbility)
	{
		AActor * OwnerActor = GetOwner();
		check(OwnerActor);

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(OwnerActor);

		AnimatingAbility->MontageBranchPoint_AbilityDecisionStop(ActorInfo);
	}
}

void UAttributeComponent::MontageBranchPoint_AbilityDecisionStart()
{
	if (AnimatingAbility)
	{
		AActor * OwnerActor = GetOwner();
		check(OwnerActor);

		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(OwnerActor);

		AnimatingAbility->MontageBranchPoint_AbilityDecisionStart(ActorInfo);
	}
}

#undef LOCTEXT_NAMESPACE

