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

	/** Allocate an AbilityActorInfo. Note: this goes through a global function and is a SharedPtr so projects can make their own AbilityActorInfo */
	AbilityActorInfo = TSharedPtr<FGameplayAbilityActorInfo>(UAbilitySystemGlobals::Get().AllocAbilityActorInfo());
	
	// Look for DSO AttributeSets (note we are currently requiring all attribute sets to be subobjects of the same owner. This doesn't *have* to be the case forever.
	AActor *Owner = GetOwner();
	InitAbilityActorInfo(Owner);	// Default init to our outer owner

	TArray<UObject*> ChildObjects;
	GetObjectsWithOuter(Owner, ChildObjects, false, RF_PendingKill);
	for (UObject * Obj : ChildObjects)
	{
		UAttributeSet* Set = Cast<UAttributeSet>(Obj);
		if (Set)  
		{
			UObject * AT = Set->GetArchetype();	
			SpawnedAttributes.Add(Set);
		}
	}
}

void UAbilitySystemComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	for (TWeakObjectPtr<UAbilityTask> TickingTask : TickingTasks)
	{
		if (TickingTask.IsValid())
		{
			TickingTask->TickTask(DeltaTime);
		}
	}
}

void UAbilitySystemComponent::InitAbilityActorInfo(AActor* AvatarActor)
{
	check(AbilityActorInfo.IsValid());
	AbilityActorInfo->InitFromActor(AvatarActor, this);
}

void UAbilitySystemComponent::RefreshAbilityActorInfo()
{
	check(AbilityActorInfo.IsValid());
	AbilityActorInfo->InitFromActor(AbilityActorInfo->Actor.Get(), this);
}

UGameplayAbility* UAbilitySystemComponent::GiveAbility(UGameplayAbility* Ability, int32 InputID)
{
	check(Ability);
	check(IsOwnerActorAuthoritative());	// Should be called on authority
	
	if (Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor)
	{
		Ability = CreateNewInstanceOfAbility(Ability);
	}

	ActivatableAbilities.Add(FGameplayAbilityInputIDPair(Ability, InputID));

	OnGiveAbility(Ability, InputID);

	return Ability;
}

void UAbilitySystemComponent::OnGiveAbility(UGameplayAbility* Ability, int32 InputID)
{
	for (FAbilityTriggerData TriggerData : Ability->AbilityTriggers)
	{
		FGameplayTag EventTag = TriggerData.TriggerTag;

		if (GameplayEventTriggeredAbilities.Contains(EventTag))
		{
			GameplayEventTriggeredAbilities[EventTag].AddUnique(Ability);
		}
		else
		{
			TArray<TWeakObjectPtr<UGameplayAbility> > Triggers;
			Triggers.Add(Ability);
			GameplayEventTriggeredAbilities.Add(EventTag, Triggers);
		}
	}
}

UGameplayAbility* UAbilitySystemComponent::CreateNewInstanceOfAbility(UGameplayAbility *Ability)
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

void UAbilitySystemComponent::NotifyAbilityEnded(UGameplayAbility* Ability)
{
	check(Ability);

	// If AnimatingAbility ended, clear the pointer
	if (AnimatingAbility == Ability)
	{
		AnimatingAbility = NULL;
	}
	
	/** If this is instanced per execution, mark pending kill and remove it from our instanced lists if we are the authority */
	if (Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
	{
		if (Ability->GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNone)
		{
			if (GetOwnerRole() == ROLE_Authority)
			{
				ReplicatedInstancedAbilities.Remove(Ability);
				Ability->MarkPendingKill();
			}
		}
		else
		{
			NonReplicatedInstancedAbilities.Remove(Ability);
			Ability->MarkPendingKill();
		}
	}
}

bool UAbilitySystemComponent::ActivateAbility(TWeakObjectPtr<UGameplayAbility> Ability)
{
	check(AbilityActorInfo.IsValid());

	if (Ability.IsValid())
	{
		Ability.Get()->InputPressed(AbilityActorInfo.Get());
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
		static void CancelAbilitiesWithTags(const FGameplayTagContainer InTags, TArray<FGameplayAbilityInputIDPair> Abilities, const FGameplayAbilityActorInfo* InActorInfo, const FGameplayAbilityActivationInfo InActivationInfo, UGameplayAbility* InIgnore)
		{
			for (int32 idx=0; idx < Abilities.Num(); ++idx)
			{
				UGameplayAbility *Ability = Abilities[idx].Ability;
				if (Ability && (Ability != InIgnore) && Ability->AbilityTags.MatchesAny(InTags, false))
				{
					Ability->CancelAbility(InActorInfo, InActivationInfo);

					/*
					if (!Ability->HasAnyFlags(RF_ClassDefaultObject) && Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
					{
						Ability->EndAbility();
						Abilities.RemoveAtSwap(idx);
						idx--;
					}
					*/
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

void UAbilitySystemComponent::OnRep_ActivateAbilities()
{
	for (FGameplayAbilityInputIDPair& Pair : ActivatableAbilities)
	{
		UGameplayAbility* Ability = Pair.Ability;
		if (Ability)
		{
			OnGiveAbility(Ability, Pair.InputID);
		}
	}
}

void UAbilitySystemComponent::ServerTryActivateAbility_Implementation(UGameplayAbility * AbilityToActivate, uint32 PrevPredictionKey, uint32 CurrPredictionKey)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (DenyClientActivation > 0)
	{
		DenyClientActivation--;
		ClientActivateAbilityFailed(AbilityToActivate, CurrPredictionKey);
		return;
	}
#endif

	ensure(AbilityToActivate);
	ensure(AbilityActorInfo.IsValid());

	// if this was triggered by a predicted ability the server triggered copy should be the canonical one
	if (PrevPredictionKey != 0)
	{
		// first check if the server has already started this ability
		for (auto ExecutingAbilityInfo : ExecutingServerAbilities)
		{
			if (ExecutingAbilityInfo.CurrPredictionKey == PrevPredictionKey && ExecutingAbilityInfo.Ability == AbilityToActivate)
			{
				switch(ExecutingAbilityInfo.State)
				{
					case EAbilityExecutionState::Executing:
						ExecutingAbilityInfo.Ability->CurrentActivationInfo.PrevPredictionKey = PrevPredictionKey;
						ExecutingAbilityInfo.Ability->CurrentActivationInfo.CurrPredictionKey = CurrPredictionKey;
						break;
					case EAbilityExecutionState::Failed:
						ClientActivateAbilityFailed(AbilityToActivate, CurrPredictionKey);
						break;
					case EAbilityExecutionState::Succeeded:
						ClientActivateAbilitySucceed(AbilityToActivate, CurrPredictionKey);
						break;
				}

				ExecutingServerAbilities.RemoveSingleSwap(ExecutingAbilityInfo);
				return;
			}
		}

		FPendingAbilityInfo AbilityInfo;
		AbilityInfo.PrevPredictionKey = PrevPredictionKey;
		AbilityInfo.CurrPredictionKey = CurrPredictionKey;
		AbilityInfo.Ability = AbilityToActivate;

		PendingClientAbilities.Add(AbilityInfo);
		return;
	}

	UGameplayAbility *InstancedAbility = NULL;

	if (AbilityToActivate->TryActivateAbility(AbilityActorInfo.Get(), PrevPredictionKey, CurrPredictionKey, &InstancedAbility))
	{
		if (InstancedAbility && InstancedAbility->GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNone)
		{
			InstancedAbility->ClientActivateAbilitySucceed(CurrPredictionKey);
		}
		else
		{
			ClientActivateAbilitySucceed(AbilityToActivate, CurrPredictionKey);
		}

		// Update our ReplicatedPredictionKey. When the client gets value, he will know his state (actor+all components/subobjects) are up to do date and he can
		// remove any necessary predictive work.

		if (CurrPredictionKey > 0)
		{
			ensure(CurrPredictionKey > ReplicatedPredictionKey);
			ReplicatedPredictionKey = CurrPredictionKey;
		}
	}
	else
	{
		ClientActivateAbilityFailed(AbilityToActivate, CurrPredictionKey);
	}
}

bool UAbilitySystemComponent::ServerTryActivateAbility_Validate(UGameplayAbility * AbilityToActivate, uint32 PrevPredictionKey, uint32 CurrPredictionKey)
{
	return true;
}

void UAbilitySystemComponent::ClientActivateAbilityFailed_Implementation(UGameplayAbility * AbilityToActivate, uint32 PredictionKey)
{
	if (PredictionKey > 0)
	{
		for (int32 idx = 0; idx < PredictionDelegates.Num(); ++idx)
		{
			uint32 Key = PredictionDelegates[idx].Key;
			if (Key == PredictionKey)
			{
				ABILITY_LOG(Warning, TEXT("Failed ActivateAbility, clearing prediction data %d"), PredictionKey);

				PredictionDelegates[idx].Value.PredictionKeyClearDelegate.Broadcast();
				for (uint32 DependentKey : PredictionDelegates[idx].Value.DependentPredictionKeys)
				{
					ClientActivateAbilityFailed(AbilityToActivate, DependentKey);
				}
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

void UAbilitySystemComponent::ClientActivateAbilitySucceed_Implementation(UGameplayAbility* AbilityToActivate, uint32 PredictionKey)
{
	ensure(AbilityToActivate);
	ensure(AbilityActorInfo.IsValid());

	// Fixme: We need a better way to link up/reconcile predictive replicated abilities. It would be ideal if we could predictively spawn an
	// ability and then replace/link it with the server spawned one once the server has confirmed it.

	if (AbilityToActivate->NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Predictive)
	{
		// Find the one we predictively spawned, tell them we are confirmed
		bool found = false;
		for (UGameplayAbility* LocalAbility : NonReplicatedInstancedAbilities)				// Fixme: this has to be updated once predictive abilities can replicate
		{
			if (LocalAbility->GetCurrentActivationInfo().CurrPredictionKey == PredictionKey)
			{
				LocalAbility->ConfirmActivateSucceed();
				found = true;
				break;
			}
		}

		if (!found)
		{
			ABILITY_LOG(Warning, TEXT("Ability %s was confirmed by server but no longer exists on client (replication key: %d"), *AbilityToActivate->GetName(), PredictionKey);
		}
	}
	else
	{
		FGameplayAbilityActivationInfo	ActivationInfo(EGameplayAbilityActivationMode::Confirmed, 0, PredictionKey);
		// We haven't already executed this ability at all, so kick it off.
		if (AbilityToActivate->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			// Need to instantiate this in order to execute
			UGameplayAbility* InstancedAbility = CreateNewInstanceOfAbility(AbilityToActivate);
			InstancedAbility->CallActivateAbility(AbilityActorInfo.Get(), ActivationInfo);
		}
		else
		{
			AbilityToActivate->CallActivateAbility(AbilityActorInfo.Get(), ActivationInfo);
		}
	}
}

void UAbilitySystemComponent::MontageBranchPoint_AbilityDecisionStop()
{
	if (AnimatingAbility)
	{
		AnimatingAbility->MontageBranchPoint_AbilityDecisionStop(AbilityActorInfo.Get());
	}
}

void UAbilitySystemComponent::MontageBranchPoint_AbilityDecisionStart()
{
	if (AnimatingAbility)
	{
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

void UAbilitySystemComponent::NotifyAbilityCommit(UGameplayAbility* Ability)
{
	AbilityCommitedCallbacks.Broadcast(Ability);
}

void UAbilitySystemComponent::NotifyAbilityActivated(UGameplayAbility* Ability)
{
	AbilityActivatedCallbacks.Broadcast(Ability);
}

void UAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, FGameplayEventData* Payload)
{
	if (GameplayEventTriggeredAbilities.Contains(EventTag))
	{
		TArray<TWeakObjectPtr<UGameplayAbility> > TriggeredAbilities = GameplayEventTriggeredAbilities[EventTag];

		for (auto Ability : TriggeredAbilities)
		{
			if (Ability.IsValid())
			{
				Ability.Get()->TriggerAbilityFromGameplayEvent(AbilityActorInfo.Get(), EventTag, Payload, *this);
			}
		}
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------
//								Input 
// ----------------------------------------------------------------------------------------------------------------------------------------------------


void UAbilitySystemComponent::BindToInputComponent(UInputComponent* InputComponent)
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

void UAbilitySystemComponent::BindAbilityActivationToInputComponent(UInputComponent* InputComponent, FGameplayAbiliyInputBinds BindInfo)
{
	UEnum* EnumBinds = BindInfo.GetBindEnum();

	for(int32 idx=0; idx < EnumBinds->NumEnums(); ++idx)
	{
		FString FullStr = EnumBinds->GetEnum(idx).ToString();
		FString BindStr;

		FullStr.Split(TEXT("::"), nullptr, &BindStr);

		// Pressed event
		{
			FInputActionBinding AB(FName(*BindStr), IE_Pressed);
			AB.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &UAbilitySystemComponent::AbilityInputPressed, idx);
			InputComponent->AddActionBinding(AB);
		}

		// Released event
		{
			FInputActionBinding AB(FName(*BindStr), IE_Released);
			AB.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &UAbilitySystemComponent::AbilityInputReleased, idx);
			InputComponent->AddActionBinding(AB);
		}
	}

	// Bind Confirm/Cancel. Note: these have to come last!
	{
		FInputActionBinding AB(FName(*BindInfo.ConfirmTargetCommand), IE_Pressed);
		AB.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &UAbilitySystemComponent::InputConfirm);
		InputComponent->AddActionBinding(AB);
	}
	
	{
		FInputActionBinding AB(FName(*BindInfo.CancelTargetCommand), IE_Pressed);
		AB.ActionDelegate.GetDelegateForManualSet().BindUObject(this, &UAbilitySystemComponent::InputCancel);
		InputComponent->AddActionBinding(AB);
	}

}

void UAbilitySystemComponent::AbilityInputPressed(int32 InputID)
{
	// FIXME: Maps or multicast delegate to actually do this
	for (FGameplayAbilityInputIDPair& Pair: ActivatableAbilities)
	{
		if (Pair.InputID == InputID)
		{
			Pair.Ability->InputPressed(AbilityActorInfo.Get());
		}
	}
}

void UAbilitySystemComponent::AbilityInputReleased(int32 InputID)
{
	// FIXME: Maps or multicast delegate to actually do this
	for (FGameplayAbilityInputIDPair& Pair : ActivatableAbilities)
	{
		if (Pair.InputID == InputID)
		{
			Pair.Ability->InputReleased(AbilityActorInfo.Get());
		}
	}
}

void UAbilitySystemComponent::InputConfirm()
{
	if (GetOwnerRole() != ROLE_Authority && ConfirmCallbacks.IsBound())
	{
		// Tell the server we confirmed input.
		ServerSetReplicatedConfirm(true);
	}
	
	ConfirmCallbacks.Broadcast();
}

void UAbilitySystemComponent::InputCancel()
{
	if (GetOwnerRole() != ROLE_Authority && CancelCallbacks.IsBound())
	{
		// Tell the server we confirmed input.
		ServerSetReplicatedConfirm(false);
	}

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
	if (Confirmed)
	{
		ReplicatedConfirmAbility = true;
		ConfirmCallbacks.Broadcast();
	}
	else
	{
		ReplicatedCancelAbility = true;
		CancelCallbacks.Broadcast();
	}
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
}

void UAbilitySystemComponent::ConsumeAbilityConfirmCancel()
{
	ReplicatedConfirmAbility = false;
	ReplicatedCancelAbility = false;
	ConfirmCallbacks.Clear();
	CancelCallbacks.Clear();
}

void UAbilitySystemComponent::ConsumeAbilityTargetData()
{
	ReplicatedTargetData.Clear();
}


#undef LOCTEXT_NAMESPACE

