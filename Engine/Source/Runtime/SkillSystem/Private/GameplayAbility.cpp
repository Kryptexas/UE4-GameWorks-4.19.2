// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

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
	if (!ActorInfo.AttributeComponent->IsNetSimulating())
	{
		ApplyCooldown(ActorInfo);

		ApplyCost(ActorInfo);
	}
}

bool UGameplayAbility::TryActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	// This should only come from button presses/local instigation
	ENetRole NetMode = ActorInfo.Actor->Role;
	ensure(NetMode != ROLE_SimulatedProxy);

	// Always do a CanActivate check
	if (!CanActivateAbility(ActorInfo))
	{
		return false;
	}

	// If we are the server or this is a client authorative 
	if (ReplicationType == EGameplayAbilityReplication::Client || (NetMode == ROLE_Authority))
	{
		ActivateAbility(ActorInfo);
	}
	else if (ReplicationType == EGameplayAbilityReplication::Server)
	{
		ServerTryActivateAbility(ActorInfo);
	}
	else if (ReplicationType == EGameplayAbilityReplication::Predictive)
	{
		PredictiveActivateAbility(ActorInfo);
		ServerTryActivateAbility(ActorInfo);
	}

	return true;	
}

void UGameplayAbility::ActivateAbility(FGameplayAbilityActorInfo ActorInfo)
{
	// Child classes may want to do stuff here...

	if (CommitAbility(ActorInfo))		// Then commit to the ability...
	{
		//	Then do more stuff...
	}
}

void UGameplayAbility::PredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{

}

void UGameplayAbility::ServerTryActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	ActorInfo.AttributeComponent->ServerTryActivateAbility(this);
}

void UGameplayAbility::ClientActivateAbility(const FGameplayAbilityActorInfo ActorInfo)
{
	ActivateAbility(ActorInfo);
}

//----------------------------------------------------------------------

void FGameplayAbilityActorInfo::InitFromActor(AActor *InActor)
{
	Actor = InActor;

	USkeletalMeshComponent * SkelMeshComponent = InActor->FindComponentByClass<USkeletalMeshComponent>();
	if (SkelMeshComponent)
	{
		this->AnimInstance = SkelMeshComponent->GetAnimInstance();
	}

	AttributeComponent = InActor->FindComponentByClass<UAttributeComponent>();
/*
	AActor *TestActor = Actor;
	while(TestActor != NULL && this->InputComponent == NULL)
	{
		APawn *TestPawn = Cast<APawn>(TestActor);
		if (TestPawn && TestPawn->GetController())
		{
			TestActor = TestPawn->GetController();
		}

		this->InputComponent = TestActor->FindComponentByClass<UInputComponent>();
		TestActor = TestActor->GetOwner();
	}
*/
}

//----------------------------------------------------------------------

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
	check(ActorInfo.AttributeComponent.IsValid());
	if (CooldownGameplayEffect)
	{
		ActorInfo.AttributeComponent->ApplyGameplayEffectToSelf(CooldownGameplayEffect, 1.f, ActorInfo.Actor.Get());
	}
}

bool UGameplayAbility::CheckCost(const FGameplayAbilityActorInfo ActorInfo) const
{
	check(ActorInfo.AttributeComponent.IsValid());
	return ActorInfo.AttributeComponent->CanApplyAttributeModifiers(CostGameplayEffect, 1.f, ActorInfo.Actor.Get());
}

void UGameplayAbility::ApplyCost(const FGameplayAbilityActorInfo ActorInfo)
{
	check(ActorInfo.AttributeComponent.IsValid());
	if (CostGameplayEffect)
	{
		ActorInfo.AttributeComponent->ApplyGameplayEffectToSelf(CostGameplayEffect, 1.f, ActorInfo.Actor.Get());
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