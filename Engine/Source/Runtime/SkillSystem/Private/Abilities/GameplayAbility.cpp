// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "AttributeComponent.h"

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

bool UGameplayAbility::CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const
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

bool UGameplayAbility::CommitAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	if (!CommitCheck(ActorInfo))
	{
		return false;
	}

	CommitExecute(ActorInfo);
	return true;
}

bool UGameplayAbility::CommitCheck(const FGameplayAbilityActorInfo ActorInfo)
{
	return CanActivateAbility(ActorInfo);
}

void UGameplayAbility::CommitExecute(const FGameplayAbilityActorInfo ActorInfo)
{
	if (!ActorInfo.AttributeComponent->IsNetSimulating() || ActorInfo.PredictionKey != 0)
	{
		ApplyCooldown(ActorInfo);

		ApplyCost(ActorInfo);
	}
}

bool UGameplayAbility::TryActivateAbility(const FGameplayAbilityActorInfo ActorInfo, UGameplayAbility ** OutInstancedAbility)
{	
	// This should only come from button presses/local instigation
	ENetRole NetMode = ActorInfo.Actor->Role;
	ensure(NetMode != ROLE_SimulatedProxy);

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
			UGameplayAbility * Ability = ActorInfo.AttributeComponent->CreateNewInstanceOfAbility(this);
			Ability->CallActivateAbility(ActorInfo);
			if (OutInstancedAbility)
			{
				*OutInstancedAbility = Ability;
			}
		}
		else
		{
			CallActivateAbility(ActorInfo);
		}
	}
	else if (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Server)
	{
		ServerTryActivateAbility(ActorInfo);
	}
	else if (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Predictive)
	{
		// This execution is now officially EGameplayAbilityActivationMode:Predicting and has a PredictionKey
		ActorInfo.GeneratePredictionKey();

		if (GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			// For now, only NonReplicated + InstancedPerExecution abilities can be Predictive.
			// We lack the code to predict spawning an instance of the execution and then merge/combine
			// with the server spawned version when it arrives.
			
			if(GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNone)
			{
				UGameplayAbility * Ability = ActorInfo.AttributeComponent->CreateNewInstanceOfAbility(this);
				Ability->CallPredictiveActivateAbility(ActorInfo);
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
			CallPredictiveActivateAbility(ActorInfo);
		}
		
		ServerTryActivateAbility(ActorInfo);
	}

	return true;	
}

void UGameplayAbility::EndAbility(const FGameplayAbilityActorInfo ActorInfo)
{

}

void UGameplayAbility::ActivateAbility(FGameplayAbilityActorInfo ActorInfo)
{
	// Child classes may want to do stuff here...

	if (CommitAbility(ActorInfo))		// ..then commit the ability...
	{
		//	Then do more stuff...
	}
}

void UGameplayAbility::PreActivate(const FGameplayAbilityActorInfo ActorInfo)
{
	ActorInfo.AttributeComponent->CancelAbilitiesWithTags(CancelAbilitiesWithTag, ActorInfo, this);
}

void UGameplayAbility::CallActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	PreActivate(ActorInfo);
	ActivateAbility(ActorInfo);
}

void UGameplayAbility::CallPredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	PreActivate(ActorInfo);
	PredictiveActivateAbility(ActorInfo);
}

void UGameplayAbility::PredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	
}

void UGameplayAbility::ServerTryActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	ActorInfo.AttributeComponent->ServerTryActivateAbility(this, ActorInfo.PredictionKey);
}

void UGameplayAbility::ClientActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	CallActivateAbility(ActorInfo);
}

void UGameplayAbility::InputPressed(int32 InputID, const FGameplayAbilityActorInfo ActorInfo)
{
	TryActivateAbility(ActorInfo);
}

void UGameplayAbility::InputReleased(int32 InputID, const FGameplayAbilityActorInfo ActorInfo)
{
	SKILL_LOG(Log, TEXT("InputReleased: %d"), InputID);
}

bool UGameplayAbility::CheckCooldown(const FGameplayAbilityActorInfo ActorInfo) const
{
	if (CooldownGameplayEffect)
	{
		check(ActorInfo.AttributeComponent.IsValid());
		if (ActorInfo.AttributeComponent->HasAnyTags(CooldownGameplayEffect->OwnedTagsContainer))
		{
			return false;
		}
	}
	return true;
}

void UGameplayAbility::ApplyCooldown(const FGameplayAbilityActorInfo ActorInfo)
{
	if (CooldownGameplayEffect)
	{
		check(ActorInfo.AttributeComponent.IsValid());
		ActorInfo.AttributeComponent->ApplyGameplayEffectToSelf(CooldownGameplayEffect, 1.f, ActorInfo.Actor.Get(), FModifierQualifier().PredictionKey(ActorInfo.PredictionKey));
	}
}

bool UGameplayAbility::CheckCost(const FGameplayAbilityActorInfo ActorInfo) const
{
	if (CostGameplayEffect)
	{
		check(ActorInfo.AttributeComponent.IsValid());
		return ActorInfo.AttributeComponent->CanApplyAttributeModifiers(CostGameplayEffect, 1.f, ActorInfo.Actor.Get());
	}
	return true;
}

void UGameplayAbility::ApplyCost(const FGameplayAbilityActorInfo ActorInfo)
{
	check(ActorInfo.AttributeComponent.IsValid());
	if (CostGameplayEffect)
	{
		ActorInfo.AttributeComponent->ApplyGameplayEffectToSelf(CostGameplayEffect, 1.f, ActorInfo.Actor.Get(), FModifierQualifier().PredictionKey(ActorInfo.PredictionKey));
	}
}

float UGameplayAbility::GetCooldownTimeRemaining(const FGameplayAbilityActorInfo ActorInfo) const
{
	check(ActorInfo.AttributeComponent.IsValid());
	if (CooldownGameplayEffect)
	{
		TArray< float > Durations = ActorInfo.AttributeComponent->GetActiveEffectsTimeRemaining(FActiveGameplayEffectQuery(&CooldownGameplayEffect->OwnedTagsContainer));
		if (Durations.Num() > 0)
		{
			Durations.Sort();
			return Durations[0];
		}
	}

	return 0.f;
}

// --------------------------------------------------------------------

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStop(const FGameplayAbilityActorInfo ActorInfo) const
{

}

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStart(const FGameplayAbilityActorInfo ActorInfo) const
{

}

void UGameplayAbility::ClientActivateAbilitySucceed_Implementation(AActor *Actor)
{
	if (Actor)
	{
		FGameplayAbilityActorInfo	ActorInfo;
		ActorInfo.InitFromActor(Actor);

		CallActivateAbility(ActorInfo);
	}
	else
	{
		SKILL_LOG(Warning, TEXT("ClientActivateAbilitySucceed called on %s with no valid Actor"), *GetName());
	}
}

//----------------------------------------------------------------------

void FGameplayAbilityActorInfo::InitFromActor(AActor *InActor)
{
	Actor = InActor;

	// On Init, we are either Authority or NonAuthority. We haven't been given a PredictionKey and we haven't been confirmed.
	// NonAuthority essentially means 'I'm not sure what how I'm going to do this yet'.
	// 
	ActivationMode = (Actor->Role == ROLE_Authority ? EGameplayAbilityActivationMode::Authority : EGameplayAbilityActivationMode::NonAuthority);
	PredictionKey = 0;

	// Grab Components that we care about
	USkeletalMeshComponent * SkelMeshComponent = InActor->FindComponentByClass<USkeletalMeshComponent>();
	if (SkelMeshComponent)
	{
		this->AnimInstance = SkelMeshComponent->GetAnimInstance();
	}

	AttributeComponent = InActor->FindComponentByClass<UAttributeComponent>();

	MovementComponent = InActor->FindComponentByClass<UMovementComponent>();
}

void FGameplayAbilityActorInfo::GeneratePredictionKey() const
{
	check(AttributeComponent.IsValid());
	check(PredictionKey == 0);
	check(ActivationMode == EGameplayAbilityActivationMode::NonAuthority);

	PredictionKey = AttributeComponent->GetNextPredictionKey();
	ActivationMode = EGameplayAbilityActivationMode::Predicting;
}

void FGameplayAbilityActorInfo::SetPredictionKey(int32 InPredictionKey)
{
	PredictionKey = InPredictionKey;
}

void FGameplayAbilityActorInfo::SetActivationConfirmed()
{
	check(ActivationMode == EGameplayAbilityActivationMode::NonAuthority);
	ActivationMode = EGameplayAbilityActivationMode::Confirmed;
}

//----------------------------------------------------------------------

void FGameplayAbilityTargetData::ApplyGameplayEffect(UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo)
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
		
		UAttributeComponent * TargetComponent = USkillSystemBlueprintLibrary::GetAttributeComponent(TargetActor);
		if (TargetComponent)
		{
			InstigatorInfo.AttributeComponent->ApplyGameplayEffectSpecToTarget(SpecToApply, TargetComponent);
		}
	}
}


FString FGameplayAbilityTargetData::ToString() const
{
	return TEXT("BASE CLASS");
}
