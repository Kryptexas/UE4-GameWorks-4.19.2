// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayTagsModule.h"
#include "GameplayEffectTypes.h"

//----------------------------------------------------------------------

void FGameplayAbilityActorInfo::InitFromActor(AActor *InActor, UAbilitySystemComponent* InAbilitySystemComponent)
{
	check(InActor);
	check(InAbilitySystemComponent);

	Actor = InActor;
	AbilitySystemComponent = InAbilitySystemComponent;

	// Look for a player controller or pawn in the owner chain.
	AActor *TestActor = InActor;
	while (TestActor)
	{
		if (APlayerController * CastPC = Cast<APlayerController>(TestActor))
		{
			PlayerController = CastPC;
			break;
		}

		if (APawn * Pawn = Cast<APawn>(TestActor))
		{
			PlayerController = Cast<APlayerController>(Pawn->GetController());
			break;
		}

		TestActor = TestActor->GetOwner();
	}

	// Grab Components that we care about
	USkeletalMeshComponent * SkelMeshComponent = InActor->FindComponentByClass<USkeletalMeshComponent>();
	if (SkelMeshComponent)
	{
		this->AnimInstance = SkelMeshComponent->GetAnimInstance();
	}

	MovementComponent = InActor->FindComponentByClass<UMovementComponent>();
}

void FGameplayAbilityActorInfo::ClearActorInfo()
{
	Actor = NULL;
	PlayerController = NULL;
	AnimInstance = NULL;
	MovementComponent = NULL;
}

bool FGameplayAbilityActorInfo::IsLocallyControlled() const
{
	if (PlayerController.IsValid())
	{
		return PlayerController->IsLocalController();
	}

	return false;
}

bool FGameplayAbilityActorInfo::IsNetAuthority() const
{
	if (Actor.IsValid())
	{
		return (Actor->Role == ROLE_Authority);
	}

	// If we encounter issues with this being called before or after the owning actor is destroyed,
	// we may need to cache off the authority (or look for it on some global/world state).

	ensure(false);
	return false;
}

void FGameplayAbilityActivationInfo::GeneratePredictionKey(UAbilitySystemComponent * Component) const
{
	check(Component);
	check(ActivationMode == EGameplayAbilityActivationMode::NonAuthority);

	PrevPredictionKey = CurrPredictionKey;
	CurrPredictionKey = Component->GetNextPredictionKey();
	ActivationMode = EGameplayAbilityActivationMode::Predicting;
}

void FGameplayAbilityActivationInfo::SetActivationConfirmed()
{
	ActivationMode = EGameplayAbilityActivationMode::Confirmed;
}


// -----------------------------------------------------------------

bool FGameplayTagCountContainer::HasMatchingGameplayTag(FGameplayTag TagToCheck, EGameplayTagMatchType::Type TagMatchType) const
{
	if (TagMatchType == EGameplayTagMatchType::Explicit)
	{
		// Search for TagToCheck
		const int32* Count = GameplayTagCountMap.Find(TagToCheck);
		if (Count && *Count > 0)
		{
			return true;
		}
	}
	else if (TagMatchType == EGameplayTagMatchType::IncludeParentTags)
	{
		// Search for TagToCheck or any of its parent tags
		FGameplayTagContainer TagAndParentsContainer = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTagParents(TagToCheck);
		for (auto TagIt = TagAndParentsContainer.CreateConstIterator(); TagIt; ++TagIt)
		{
			const int32* Count = GameplayTagCountMap.Find(*TagIt);
			if (Count && *Count > 0)
			{
				return true;
			}
		}
	}

	return false;
}

bool FGameplayTagCountContainer::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer, EGameplayTagMatchType::Type TagMatchType, bool bCountEmptyAsMatch) const
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsHasAnyTag);
	if (TagContainer.Num() == 0)
		return bCountEmptyAsMatch;

	for (auto It = TagContainer.CreateConstIterator(); It; ++It)
	{
		if (HasMatchingGameplayTag(*It, TagMatchType))
		{
			return true;
		}
	}

	return false;
}

void FGameplayTagCountContainer::UpdateTagMap(const FGameplayTag& Tag, int32 CountDelta)
{
	// Update count of Tag
	int32& Count = GameplayTagCountMap.FindOrAdd(Tag);

	bool WasZero = Count == 0;
	Count = FMath::Max(Count + CountDelta, 0);
		
	// If we went from 0->1 or 1->0
	if (WasZero || Count == 0)
	{
		FOnGameplayEffectTagCountChanged *Delegate = GameplayTagEventMap.Find(Tag);
		if (Delegate)
		{
			Delegate->Broadcast(Tag, Count);
		}
	}
}

void FGameplayTagCountContainer::UpdateTagMap(const FGameplayTagContainer& Container, int32 CountDelta)
{
	for (auto TagIt = Container.CreateConstIterator(); TagIt; ++TagIt)
	{
		const FGameplayTag& BaseTag = *TagIt;
		if (TagContainerType == EGameplayTagMatchType::Explicit)
		{
			// Update count of BaseTag
			UpdateTagMap(BaseTag, CountDelta);
		}
		else if (TagContainerType == EGameplayTagMatchType::IncludeParentTags)
		{
			// Update count of BaseTag and all of its parent tags
			FGameplayTagContainer TagAndParentsContainer = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTagParents(BaseTag);
			for (auto ParentTagIt = TagAndParentsContainer.CreateConstIterator(); ParentTagIt; ++ParentTagIt)
			{
				UpdateTagMap(*ParentTagIt, CountDelta);
			}
		}
	}
}




