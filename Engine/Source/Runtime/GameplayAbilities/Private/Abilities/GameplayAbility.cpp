// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayAbility
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

UGameplayAbility::UGameplayAbility(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	CostGameplayEffect = NULL;
	CooldownGameplayEffect = NULL;

}

bool UGameplayAbility::IsSupportedForNetworking() const
{
	/**
	 *	If this check fails, it means we are trying to serialize a reference to an invalid GameplayAbility. 
	 *	We can only replicate references to:
	 *		-CDOs and DataAssets (e.g., static, non-instanced gameplay abilities)
	 *		-Instanced abilities that are replicating (and will thus be created on clients).
	 *		
	 *	The network code should never be asking a gameplay ability if it IsSupportedForNetworking() for 
	 *	an ability that is not described above.
	 */

	bool Supported = GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNone || GetOuter()->IsA(UPackage::StaticClass());
	check(Supported);

	return Supported;
}

bool UGameplayAbility::CanActivateAbility(const FGameplayAbilityActorInfo* ActorInfo) const
{
	// Check cooldowns and resource consumption immediately
	if (!CheckCooldown(ActorInfo))
	{
		return false;
	}

	if (!CheckCost(ActorInfo))
	{
		return false;
	}

	return true;
}

bool UGameplayAbility::CommitAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!CommitCheck(ActorInfo, ActivationInfo))
	{
		return false;
	}

	CommitExecute(ActorInfo, ActivationInfo);
	return true;
}

bool UGameplayAbility::CommitCheck(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	return CanActivateAbility(ActorInfo);
}

void UGameplayAbility::CommitExecute(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!ActorInfo->AbilitySystemComponent->IsNetSimulating() || ActivationInfo.PredictionKey != 0)
	{
		ApplyCooldown(ActorInfo, ActivationInfo);

		ApplyCost(ActorInfo, ActivationInfo);
	}
}

bool UGameplayAbility::TryActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, int32 PredictionKey, UGameplayAbility ** OutInstancedAbility)
{	
	// This should only come from button presses/local instigation
	ENetRole NetMode = ActorInfo->Actor->Role;
	ensure(NetMode != ROLE_SimulatedProxy);
	
	FGameplayAbilityActivationInfo	ActivationInfo(ActorInfo->Actor.Get(), PredictionKey);

	// Always do a non instanced CanActivate check
	if (!CanActivateAbility(ActorInfo))
	{
		return false;
	}

	// If we are the server or this is a client authorative 
	if (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Client || (NetMode == ROLE_Authority))
	{
		// Create instance of this ability if necessary
		
		if (GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			UGameplayAbility * Ability = ActorInfo->AbilitySystemComponent->CreateNewInstanceOfAbility(this);
			Ability->CallActivateAbility(ActorInfo, ActivationInfo);
			if (OutInstancedAbility)
			{
				*OutInstancedAbility = Ability;
			}
		}
		else
		{
			CallActivateAbility(ActorInfo, ActivationInfo);
		}
	}
	else if (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Server)
	{
		ServerTryActivateAbility(ActorInfo, ActivationInfo);
	}
	else if (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Predictive)
	{
		// This execution is now officially EGameplayAbilityActivationMode:Predicting and has a PredictionKey
		ActivationInfo.GeneratePredictionKey(ActorInfo->AbilitySystemComponent.Get());

		if (GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			// For now, only NonReplicated + InstancedPerExecution abilities can be Predictive.
			// We lack the code to predict spawning an instance of the execution and then merge/combine
			// with the server spawned version when it arrives.
			
			if(GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNone)
			{
				UGameplayAbility * Ability = ActorInfo->AbilitySystemComponent->CreateNewInstanceOfAbility(this);
				Ability->CallPredictiveActivateAbility(ActorInfo, ActivationInfo);
				if (OutInstancedAbility)
				{
					*OutInstancedAbility = Ability;
				}
			}
			else
			{
				ensure(false);
			}
		}
		else
		{
			CallPredictiveActivateAbility(ActorInfo, ActivationInfo);
		}
		
		ServerTryActivateAbility(ActorInfo, ActivationInfo);
	}

	return true;	
}

void UGameplayAbility::EndAbility(const FGameplayAbilityActorInfo* ActorInfo)
{

}

void UGameplayAbility::ActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Child classes may want to do stuff here...

	if (CommitAbility(ActorInfo, ActivationInfo))		// ..then commit the ability...
	{
		//	Then do more stuff...
	}
}

void UGameplayAbility::PreActivate(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	ActorInfo->AbilitySystemComponent->CancelAbilitiesWithTags(CancelAbilitiesWithTag, ActorInfo, ActivationInfo, this);
}

void UGameplayAbility::CallActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	PreActivate(ActorInfo, ActivationInfo);
	ActivateAbility(ActorInfo, ActivationInfo);
}

void UGameplayAbility::CallPredictiveActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	PreActivate(ActorInfo, ActivationInfo);
	PredictiveActivateAbility(ActorInfo, ActivationInfo);
}

void UGameplayAbility::PredictiveActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	
}

void UGameplayAbility::ServerTryActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	ActorInfo->AbilitySystemComponent->ServerTryActivateAbility(this, ActivationInfo.PredictionKey);
}

void UGameplayAbility::ClientActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{ 
	CallActivateAbility(ActorInfo, ActivationInfo);
}

void UGameplayAbility::InputPressed(int32 InputID, const FGameplayAbilityActorInfo* ActorInfo)
{
	TryActivateAbility(ActorInfo);
}

void UGameplayAbility::InputReleased(int32 InputID, const FGameplayAbilityActorInfo* ActorInfo)
{
	ABILITY_LOG(Log, TEXT("InputReleased: %d"), InputID);
}

bool UGameplayAbility::CheckCooldown(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownGameplayEffect)
	{
		check(ActorInfo->AbilitySystemComponent.IsValid());
		if (ActorInfo->AbilitySystemComponent->HasAnyTags(CooldownGameplayEffect->OwnedTagsContainer))
		{
			return false;
		}
	}
	return true;
}

void UGameplayAbility::ApplyCooldown(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (CooldownGameplayEffect)
	{
		check(ActorInfo->AbilitySystemComponent.IsValid());
		ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(CooldownGameplayEffect, 1.f, ActorInfo->Actor.Get(), FModifierQualifier().PredictionKey(ActivationInfo.PredictionKey));
	}
}

bool UGameplayAbility::CheckCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CostGameplayEffect)
	{
		check(ActorInfo->AbilitySystemComponent.IsValid());
		return ActorInfo->AbilitySystemComponent->CanApplyAttributeModifiers(CostGameplayEffect, 1.f, ActorInfo->Actor.Get());
	}
	return true;
}

void UGameplayAbility::ApplyCost(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	check(ActorInfo->AbilitySystemComponent.IsValid());
	if (CostGameplayEffect)
	{
		ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(CostGameplayEffect, 1.f, ActorInfo->Actor.Get(), FModifierQualifier().PredictionKey(ActivationInfo.PredictionKey));
	}
}

float UGameplayAbility::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayAbilityGetCooldownTimeRemaining);

	check(ActorInfo->AbilitySystemComponent.IsValid());
	if (CooldownGameplayEffect)
	{
		TArray< float > Durations = ActorInfo->AbilitySystemComponent->GetActiveEffectsTimeRemaining(FActiveGameplayEffectQuery(&CooldownGameplayEffect->OwnedTagsContainer));
		if (Durations.Num() > 0)
		{
			Durations.Sort();
			return Durations[0];
		}
	}

	return 0.f;
}

// --------------------------------------------------------------------

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStop(const FGameplayAbilityActorInfo* ActorInfo) const
{

}

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStart(const FGameplayAbilityActorInfo* ActorInfo) const
{

}

void UGameplayAbility::ClientActivateAbilitySucceed_Implementation(int32 PredictionKey)
{
	// Child classes can't override ClientActivateAbilitySucceed_Implementation, so just call ClientActivateAbilitySucceed_Internal
	ClientActivateAbilitySucceed_Internal(PredictionKey);
}

void UGameplayAbility::ClientActivateAbilitySucceed_Internal(int32 PredictionKey)
{
	ABILITY_LOG(Fatal, TEXT("ClientActivateAbilitySucceed_Internal called on base class ability %s"), *GetName());
}

//----------------------------------------------------------------------

void FGameplayAbilityActorInfo::InitFromActor(AActor *InActor)
{
	Actor = InActor;

	// Grab Components that we care about
	USkeletalMeshComponent * SkelMeshComponent = InActor->FindComponentByClass<USkeletalMeshComponent>();
	if (SkelMeshComponent)
	{
		this->AnimInstance = SkelMeshComponent->GetAnimInstance();
	}

	AbilitySystemComponent = InActor->FindComponentByClass<UAbilitySystemComponent>();

	MovementComponent = InActor->FindComponentByClass<UMovementComponent>();
}

void FGameplayAbilityActivationInfo::GeneratePredictionKey(UAbilitySystemComponent * Component) const
{
	check(Component);
	check(PredictionKey == 0);
	check(ActivationMode == EGameplayAbilityActivationMode::NonAuthority);

	PredictionKey = Component->GetNextPredictionKey();
	ActivationMode = EGameplayAbilityActivationMode::Predicting;
}

void FGameplayAbilityActivationInfo::SetPredictionKey(int32 InPredictionKey)
{
	PredictionKey = InPredictionKey;
}

void FGameplayAbilityActivationInfo::SetActivationConfirmed()
{
	//check(ActivationMode == EGameplayAbilityActivationMode::NonAuthority);
	ActivationMode = EGameplayAbilityActivationMode::Confirmed;
}

//----------------------------------------------------------------------

void FGameplayAbilityTargetData::ApplyGameplayEffect(UGameplayEffect* GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo)
{
	// Improve relationship between InstigatorContext and 
	

	FGameplayEffectSpec	SpecToApply(GameplayEffect,					// The UGameplayEffect data asset
									InstigatorInfo.Actor.Get(),		// The actor who instigated this
									1.f,							// FIXME: Leveling
									NULL							// FIXME: CurveData override... should we just remove this?
									);
	if (HasHitResult())
	{
		SpecToApply.InstigatorContext.AddHitResult(*GetHitResult());
	}

	TArray<AActor*> Actors = GetActors();
	for (AActor * TargetActor : Actors)
	{
		check(TargetActor);
		
		UAbilitySystemComponent * TargetComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
		if (TargetComponent)
		{
			InstigatorInfo.AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(SpecToApply, TargetComponent);
		}
	}
}


FString FGameplayAbilityTargetData::ToString() const
{
	return TEXT("BASE CLASS");
}
