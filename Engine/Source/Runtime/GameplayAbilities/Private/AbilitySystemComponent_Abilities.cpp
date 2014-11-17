// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
// ActorComponent.cpp: Actor component implementation.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "Abilities/Tasks/AbilityTask.h"

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
	SCOPE_CYCLE_COUNTER(STAT_TickAbilityTasks);

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Because we have no control over what a task may do when it ticks, we must be careful.
	// Ticking a task may kill the task right here. It could also potentially kill another task
	// which was waiting on the original task to do something. Since when a tasks is killed, it removes
	// itself from the TickingTask list, we will make a copy of the tasks we want to service before ticking any

	int32 NumTickingTasks = TickingTasks.Num();
	switch(NumTickingTasks)
	{
		case 0:
			break;
		case 1:
			if (TickingTasks[0].IsValid())
			{
				TickingTasks[0]->TickTask(DeltaTime);
			}
			break;
		default:
			{
				TArray<TWeakObjectPtr<UAbilityTask> >	LocalTickingTasks = TickingTasks;
				for (TWeakObjectPtr<UAbilityTask>& TickingTask : LocalTickingTasks)
				{
					if (TickingTask.IsValid())
					{
						TickingTask->TickTask(DeltaTime);
					}
				}
			}
		break;
	};
}

void UAbilitySystemComponent::InitAbilityActorInfo(AActor* AvatarActor)
{
	check(AbilityActorInfo.IsValid());
	AbilityActorInfo->InitFromActor(AvatarActor, this);
	AbilityActor = AvatarActor;
}

void UAbilitySystemComponent::ClearActorInfo()
{
	check(AbilityActorInfo.IsValid());
	AbilityActorInfo->ClearActorInfo();
	AbilityActor = NULL;
}

void UAbilitySystemComponent::OnRep_OwningActor()
{
	check(AbilityActorInfo.IsValid());

	if (AbilityActor != AbilityActorInfo->Actor)
	{
		if (AbilityActor != NULL)
		{
			InitAbilityActorInfo(AbilityActor);
		}
		else
		{
			ClearActorInfo();
		}
	}
}

void UAbilitySystemComponent::RefreshAbilityActorInfo()
{
	check(AbilityActorInfo.IsValid());
	AbilityActorInfo->InitFromActor(AbilityActorInfo->Actor.Get(), this);
}

FGameplayAbilitySpecHandle UAbilitySystemComponent::GiveAbility(FGameplayAbilitySpec Spec)
{	
	check(Spec.Ability);
	check(IsOwnerActorAuthoritative());	// Should be called on authority
	
	FGameplayAbilitySpec& OwnedSpec = ActivatableAbilities[ActivatableAbilities.Add(Spec)];
	
	if (OwnedSpec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor)
	{
		OwnedSpec.Ability = CreateNewInstanceOfAbility(OwnedSpec, Spec.Ability);
	}
	
	OnGiveAbility(OwnedSpec);

	return OwnedSpec.Handle;
}

void UAbilitySystemComponent::ClearAllAbilities()
{
	check(IsOwnerActorAuthoritative());	// Should be called on authority

	// Note we aren't marking any old abilities pending kill. This shouldn't matter since they will be garbage collected.
	ActivatableAbilities.Empty(ActivatableAbilities.Num());
}

void UAbilitySystemComponent::OnGiveAbility(const FGameplayAbilitySpec Spec)
{
	for (const FAbilityTriggerData& TriggerData : Spec.Ability->AbilityTriggers)
	{
		FGameplayTag EventTag = TriggerData.TriggerTag;

		if (GameplayEventTriggeredAbilities.Contains(EventTag))
		{
			GameplayEventTriggeredAbilities[EventTag].AddUnique(Spec.Handle);	// Fixme: is this right? Do we want to trigger the ability directly of the spec?
		}
		else
		{
			TArray<FGameplayAbilitySpecHandle> Triggers;
			Triggers.Add(Spec.Handle);
			GameplayEventTriggeredAbilities.Add(EventTag, Triggers);
		}
	}
}

FGameplayAbilitySpec* UAbilitySystemComponent::FindAbilitySpecFromHandle(FGameplayAbilitySpecHandle Handle)
{
	SCOPE_CYCLE_COUNTER(STAT_FindAbilitySpecFromHandle);

	for (FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		if (Spec.Handle == Handle)
		{
			return &Spec;
		}
	}

	return nullptr;
}

FGameplayAbilitySpec* UAbilitySystemComponent::FindAbilitySpecFromInputID(int32 InputID)
{
	if (InputID != INDEX_NONE)
	{
		for (FGameplayAbilitySpec& Spec : ActivatableAbilities)
		{
			if (Spec.InputID == InputID)
			{
				return &Spec;
			}
		}
	}
	return nullptr;
}

UGameplayAbility* UAbilitySystemComponent::CreateNewInstanceOfAbility(FGameplayAbilitySpec& Spec, UGameplayAbility* Ability)
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
		Spec.ReplicatedInstances.Add(AbilityInstance);
		AllReplicatedInstancedAbilities.Add(AbilityInstance);
	}
	else
	{
		Spec.NonReplicatedInstances.Add(AbilityInstance);
	}
	
	return AbilityInstance;
}

void UAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	check(Spec);
	check(Ability);

	// If AnimatingAbility ended, clear the pointer
	if (AnimatingAbility == Ability)
	{
		AnimatingAbility = NULL;
	}

	Spec->IsActive = false;
	
	/** If this is instanced per execution, mark pending kill and remove it from our instanced lists if we are the authority */
	if (Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
	{
		check(Ability->HasAnyFlags(RF_ClassDefaultObject) == false);	// Should never be calling this on a CDO for an instanced ability!

		if (Ability->GetReplicationPolicy() != EGameplayAbilityReplicationPolicy::ReplicateNone)
		{
			if (GetOwnerRole() == ROLE_Authority)
			{
				Spec->ReplicatedInstances.Remove(Ability);
				AllReplicatedInstancedAbilities.Remove(Ability);
				Ability->MarkPendingKill();
			}
		}
		else
		{
			Spec->NonReplicatedInstances.Remove(Ability);
			Ability->MarkPendingKill();
		}
	}
}

void UAbilitySystemComponent::CancelAbilitiesWithTags(const FGameplayTagContainer Tags, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, UGameplayAbility* Ignore)
{
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		if (Spec.IsActive && Spec.Ability && Spec.Ability->AbilityTags.MatchesAny(Tags, false))
		{
			if (Spec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
			{
				// We need to cancel spawned instance, not the CDO
				TArray<UGameplayAbility*> AbilitiesToCancel = Spec.GetAbilityInstances();
				for (UGameplayAbility* InstanceAbility : AbilitiesToCancel)
				{
					if (InstanceAbility && Ignore != InstanceAbility)
					{
						InstanceAbility->CancelAbility(Spec.Handle, ActorInfo, ActivationInfo);
					}
				}
			}
			else
			{
				Spec.Ability->CancelAbility(Spec.Handle, ActorInfo, ActivationInfo);
				check(!Spec.IsActive);
			}
		}
	}
}

void UAbilitySystemComponent::BlockAbilitiesWithTags(const FGameplayTagContainer Tags)
{
	BlockedAbilityTags.UpdateTagMap(Tags, 1);
}

void UAbilitySystemComponent::UnBlockAbilitiesWithTags(const FGameplayTagContainer Tags)
{
	BlockedAbilityTags.UpdateTagMap(Tags, -1);
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
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		OnGiveAbility(Spec);
	}
}

/**
 * Attempts to activate the ability.
 *	-This function calls CanActivateAbility
 *	-This function handles instancing
 *	-This function handles networking and prediction
 *	-If all goes well, CallActivateAbility is called next.
 */
bool UAbilitySystemComponent::TryActivateAbility(FGameplayAbilitySpecHandle Handle, FPredictionKey InPredictionKey, UGameplayAbility** OutInstancedAbility)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		ABILITY_LOG(Warning, TEXT("TryActivateAbility called with invalid Handle"));
		return false;
	}

	const FGameplayAbilityActorInfo* ActorInfo = AbilityActorInfo.Get();

	// make sure the ActorInfo and then Actor on that FGameplayAbilityActorInfo are valid, if not bail out.
	if (ActorInfo == NULL || ActorInfo->Actor == NULL)
	{
		return false;
	}

	// This should only come from button presses/local instigation (AI, etc)
	ENetRole NetMode = ActorInfo->Actor->Role;
	ensure(NetMode != ROLE_SimulatedProxy);

	UGameplayAbility* Ability = Spec->Ability;

	// Check if any of this ability's tags are currently blocked
	if (BlockedAbilityTags.HasAnyMatchingGameplayTags(Ability->AbilityTags, EGameplayTagMatchType::IncludeParentTags, false))
	{
		return false;
	}

	// Always do a non instanced CanActivate check
	if (!Ability->CanActivateAbility(Handle, ActorInfo))
	{
		return false;
	}

	if (Spec->IsActive && Ability->GetInstancingPolicy() != EGameplayAbilityInstancingPolicy::NonInstanced)
	{
		// Don't have a great solution for ending non instanced abilities on the server. So only warn if this was instanced.
		ABILITY_LOG(Warning, TEXT("TryActivateAbility called when ability was already active. NetMode: %d. Ability: %s"), (int32)NetMode, *Ability->GetName());
	}
	
	Spec->IsActive = true;

	// Setup a fresh ActivationInfo for this AbilitySpec.
	Spec->ActivationInfo = FGameplayAbilityActivationInfo(ActorInfo->Actor.Get(), InPredictionKey);
	FGameplayAbilityActivationInfo &ActivationInfo = Spec->ActivationInfo;

	UGameplayAbility* InstancedAbility = Ability;

	// If we are the server or this is a client authorative 
	if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::Client || (NetMode == ROLE_Authority))
	{
		// Create instance of this ability if necessary
		if (Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			InstancedAbility = CreateNewInstanceOfAbility(*Spec, Ability);
			InstancedAbility->CallActivateAbility(Handle, ActorInfo, ActivationInfo);
			if (OutInstancedAbility)
			{
				*OutInstancedAbility = InstancedAbility;
			}
		}
		else
		{
			Ability->CallActivateAbility(Handle, ActorInfo, ActivationInfo);
		}
	}
	else if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::Server)
	{
		ServerTryActivateAbility(Handle, FPredictionKey());
	}
	else if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::Predictive)
	{
		// This execution is now officially EGameplayAbilityActivationMode:Predicting and has a PredictionKey
		ActivationInfo.GenerateNewPredictionKey();

		FPredictionKey ThisPredictionKey = ActivationInfo.GetPredictionKeyForNewAction();
		
		// This must be called immediately after GeneratePredictionKey to prevent problems with recursively activating abilities
		ServerTryActivateAbility(Handle, ThisPredictionKey);

		// If this PredictionKey is rejected, we will call OnClientActivateAbilityFailed.
		ThisPredictionKey.NewRejectedDelegate().BindUObject(this, &UAbilitySystemComponent::OnClientActivateAbilityFailed, Handle, ThisPredictionKey.Current);

		if (Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			// For now, only NonReplicated + InstancedPerExecution abilities can be Predictive.
			// We lack the code to predict spawning an instance of the execution and then merge/combine
			// with the server spawned version when it arrives.

			if (Ability->GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNone)
			{
				InstancedAbility = CreateNewInstanceOfAbility(*Spec, Ability);
				InstancedAbility->CallActivateAbility(Handle, ActorInfo, ActivationInfo);
				if (OutInstancedAbility)
				{
					*OutInstancedAbility = InstancedAbility;
				}
			}
			else
			{
				ABILITY_LOG(Error, TEXT("TryActivateAbility called on ability %s that is InstancePerExecution and Replicated. This is an invalid configuration."), *Ability->GetName() );
			}
		}
		else
		{
			Ability->CallActivateAbility(Handle, ActorInfo, ActivationInfo);
		}
	}

	// Anything after this can no longer be predicted with the prediction key we just generated.
	// The key we generated and sent in ServerTryActivateAbility will be confirmed/denied after the server run CallActivateAbility.
	// If game code tries to do something predictively after this (latent call, response to input, etc) it will not be able to use this key.
	ActivationInfo.SetPredictionStale();
	InstancedAbility->SetCurrentActivationInfo(ActivationInfo);	// Need to push this to the ability if it was instanced.

	return true;
}

void UAbilitySystemComponent::ServerTryActivateAbility_Implementation(FGameplayAbilitySpecHandle Handle, FPredictionKey PredictionKey)
{
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	if (DenyClientActivation > 0)
	{
		DenyClientActivation--;
		ClientActivateAbilityFailed(Handle, PredictionKey.Current);
		return;
	}
#endif

	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		// Can potentially happen in race conditions where client tries to activate ability that is removed server side before it is received.
		return;
	}

	UGameplayAbility* AbilityToActivate = Spec->Ability;

	ensure(AbilityToActivate);
	ensure(AbilityActorInfo.IsValid());

	// if this was triggered by a predicted ability the server triggered copy should be the canonical one
	if (PredictionKey.Base != 0)
	{
		// first check if the server has already started this ability
		for (int32 ExecutedIdx = 0; ExecutedIdx < ExecutingServerAbilities.Num(); ++ExecutedIdx)
		{
			FExecutingAbilityInfo& ExecutingAbilityInfo = ExecutingServerAbilities[ExecutedIdx];
			if (ExecutingAbilityInfo.PredictionKey.Current == PredictionKey.Base && ExecutingAbilityInfo.Handle == Handle)
			{
				switch(ExecutingAbilityInfo.State)
				{
					case EAbilityExecutionState::Failed:
						ClientActivateAbilityFailed(Handle, PredictionKey.Current);
						break;
					case EAbilityExecutionState::Executing:
					case EAbilityExecutionState::Succeeded:
						ClientActivateAbilitySucceed(Handle, PredictionKey.Current);
						break;
				}

				ExecutingServerAbilities.RemoveAtSwap(ExecutedIdx);
				return;
			}
		}

		FPendingAbilityInfo AbilityInfo;
		AbilityInfo.PredictionKey = PredictionKey;
		AbilityInfo.Handle = Handle;

		PendingClientAbilities.Add(AbilityInfo);
		return;
	}

	UGameplayAbility* InstancedAbility = NULL;
	Spec->InputPressed = true; // Pretend input was pressed. Allows UAbilityTask_WaitInputRelease to work.

	// Attempt to activate the ability (server side) and tell the client if it succeeded or failed.
	if (TryActivateAbility(Handle, PredictionKey, &InstancedAbility))
	{
		ClientActivateAbilitySucceed(Handle, PredictionKey.Current);

		// Update our ReplicatedPredictionKey. When the client gets value, he will know his state (actor+all components/subobjects) are up to do date and he can
		// remove any necessary predictive work.

		if (PredictionKey.IsValidKey())
		{
			ensure(PredictionKey.Current > ReplicatedPredictionKey.Current);
			ReplicatedPredictionKey = PredictionKey;
		}
	}
	else
	{
		Spec->InputPressed = false;
		ClientActivateAbilityFailed(Handle, PredictionKey.Current);
	}
}

bool UAbilitySystemComponent::ServerTryActivateAbility_Validate(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey PredictionKey)
{
	return true;
}

static_assert(sizeof(int16) == sizeof(FPredictionKey::KeyType), "Sizeof PredictionKey::KeyType does not match RPC parameters in AbilitySystemComponent ClientActivateAbilityFailed_Implementation");

void UAbilitySystemComponent::ClientActivateAbilityFailed_Implementation(FGameplayAbilitySpecHandle Handle, int16 PredictionKey)
{
	// If this was predicted, we must use the PredictionKey system to end the ability
	if (PredictionKey > 0)
	{
		FPredictionKeyDelegates::BroadcastRejectedDelegate(PredictionKey);
	}

	// If this was not predicted... there is not much to do right now. We may need to keep track on the AbilitySpec if we are waiting on a confirm in these cases.
}

void UAbilitySystemComponent::OnClientActivateAbilityFailed(FGameplayAbilitySpecHandle Handle, FPredictionKey::KeyType PredictionKey)
{
	// Find the actual UGameplayAbility		
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);

	ABILITY_LOG(Warning, TEXT("ClientActivateAbilityFailed_Implementation. PredictionKey :%d"), PredictionKey);

	TArray<UGameplayAbility*> Instances = Spec->GetAbilityInstances();
	for (UGameplayAbility* Ability : Instances)
	{
		if (Ability->CurrentActivationInfo.GetPredictionKey().Current == PredictionKey)
		{
			ABILITY_LOG(Warning, TEXT("Ending Ability %s"), *Ability->GetName());
			Ability->K2_EndAbility();
		}
	}
}


static_assert(sizeof(int16) == sizeof(FPredictionKey::KeyType), "Sizeof PredictionKey::KeyType does not match RPC parameters in AbilitySystemComponent ClientActivateAbilitySucceed_Implementation");

void UAbilitySystemComponent::ClientActivateAbilitySucceed_Implementation(FGameplayAbilitySpecHandle Handle, int16 PredictionKey)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (!Spec)
	{
		// Can potentially happen in race conditions where client tries to activate ability that is removed server side before it is received.
		return;
	}

	UGameplayAbility* AbilityToActivate = Spec->Ability;

	ensure(AbilityToActivate);
	ensure(AbilityActorInfo.IsValid());

	Spec->ActivationInfo.SetActivationConfirmed();

	// Fixme: We need a better way to link up/reconcile predictive replicated abilities. It would be ideal if we could predictively spawn an
	// ability and then replace/link it with the server spawned one once the server has confirmed it.

	if (AbilityToActivate->NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Predictive)
	{
		if (AbilityToActivate->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::NonInstanced)
		{
			AbilityToActivate->ConfirmActivateSucceed();
		}
		else
		{
			// Find the one we predictively spawned, tell them we are confirmed
			bool found = false;
			for (UGameplayAbility* LocalAbility : Spec->NonReplicatedInstances)				// Fixme: this has to be updated once predictive abilities can replicate
			{
				if (LocalAbility->GetCurrentActivationInfo().GetPredictionKey().Current == PredictionKey)
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
	}
	else
	{
		// We haven't already executed this ability at all, so kick it off.
		
		if (AbilityToActivate->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			// Need to instantiate this in order to execute
			UGameplayAbility* InstancedAbility = CreateNewInstanceOfAbility(*Spec, AbilityToActivate);
			InstancedAbility->CallActivateAbility(Handle, AbilityActorInfo.Get(), Spec->ActivationInfo);
		}
		else
		{
			AbilityToActivate->CallActivateAbility(Handle, AbilityActorInfo.Get(), Spec->ActivationInfo);
		}
	}
}

void UAbilitySystemComponent::TriggerAbilityFromGameplayEvent(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActorInfo* ActorInfo, FGameplayTag EventTag, FGameplayEventData* Payload, UAbilitySystemComponent& Component)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (!ensure(Spec))
	{
		return;
	}

	UGameplayAbility* Ability = Spec->Ability;
	if (!ensure(Ability))
	{
		return;
	}

	if (Ability->ShouldAbilityRespondToEvent(EventTag, Payload))
	{
		int32 ExecutingAbilityIndex = -1;

		// if we're the server and this is coming from a predicted event we should check if the client has already predicted it
		if (Payload->PredictionKey.Current > 0
			&& Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::Predictive
			&& ActorInfo->Actor->Role == ROLE_Authority)
		{
			bool bPendingClientAbilityFound = false;
			for (auto PendingAbilityInfo : Component.PendingClientAbilities)
			{
				if (Payload->PredictionKey.Current == PendingAbilityInfo.PredictionKey.Base && Handle == PendingAbilityInfo.Handle) // found a match
				{
					Payload->PredictionKey = PendingAbilityInfo.PredictionKey;

					Component.PendingClientAbilities.RemoveSingleSwap(PendingAbilityInfo);
					bPendingClientAbilityFound = true;
					break;
				}
			}

			// we haven't received the client's copy of the triggered ability
			// keep track of this so we can associate the prediction keys when it comes in
			if (bPendingClientAbilityFound == false)
			{
				UAbilitySystemComponent::FExecutingAbilityInfo Info;
				Info.PredictionKey = Payload->PredictionKey;
				Info.Handle = Handle;

				ExecutingAbilityIndex = Component.ExecutingServerAbilities.Add(Info);
			}
		}

		if (TryActivateAbility(Handle, Payload->PredictionKey))
		{
			if (ExecutingAbilityIndex >= 0)
			{
				Component.ExecutingServerAbilities[ExecutingAbilityIndex].State = UAbilitySystemComponent::EAbilityExecutionState::Succeeded;
			}
		}
		else if (ExecutingAbilityIndex >= 0)
		{
			Component.ExecutingServerAbilities[ExecutingAbilityIndex].State = UAbilitySystemComponent::EAbilityExecutionState::Failed;
		}
	}
}

// ----------------------------------------------------------------------------------------------------------------------------------------------------
//								Input 
// ----------------------------------------------------------------------------------------------------------------------------------------------------

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

void UAbilitySystemComponent::NotifyAbilityActivated(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	AbilityActivatedCallbacks.Broadcast(Ability);
}

void UAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, FGameplayEventData* Payload)
{
	if (GameplayEventTriggeredAbilities.Contains(EventTag))
	{		
		TArray<FGameplayAbilitySpecHandle> TriggeredAbilityHandles = GameplayEventTriggeredAbilities[EventTag];

		for (auto AbilityHandle : TriggeredAbilityHandles)
		{
			TriggerAbilityFromGameplayEvent(AbilityHandle, AbilityActorInfo.Get(), EventTag, Payload, *this);
						
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
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		if (Spec.InputID == InputID)
		{
			if (Spec.Ability)
			{
				Spec.InputPressed = true;
				if (Spec.IsActive)
				{
					// The ability is active, so just pipe the input event to it
					if (Spec.Ability->GetInstancingPolicy() ==  EGameplayAbilityInstancingPolicy::NonInstanced)
					{
						Spec.Ability->InputPressed(Spec.Handle, AbilityActorInfo.Get(), Spec.ActivationInfo);
					}
					else
					{
						TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
						for (UGameplayAbility* Instance : Instances)
						{
							Instance->InputPressed(Spec.Handle, AbilityActorInfo.Get(), Spec.ActivationInfo);
						}						
					}
				}
				else
				{
					// Ability is not active, so try to activate it
					TryActivateAbility(Spec.Handle);
				}
			}
		}
	}
}

void UAbilitySystemComponent::AbilityInputReleased(int32 InputID)
{
	// FIXME: Maps or multicast delegate to actually do this
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities)
	{
		if (Spec.InputID == InputID)
		{
			if (Spec.Ability)
			{
				AbilitySpectInputReleased(Spec);
			}
		}
	}
}

void UAbilitySystemComponent::AbilitySpectInputReleased(FGameplayAbilitySpec& Spec)
{
	Spec.InputPressed = false;
	if (Spec.IsActive)
	{
		// The ability is active, so just pipe the input event to it
		if (Spec.Ability->GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::NonInstanced)
		{
			Spec.Ability->InputReleased(Spec.Handle, AbilityActorInfo.Get(), Spec.ActivationInfo);
		}
		else
		{
			TArray<UGameplayAbility*> Instances = Spec.GetAbilityInstances();
			for (UGameplayAbility* Instance : Instances)
			{
				Instance->InputReleased(Spec.Handle, AbilityActorInfo.Get(), Spec.ActivationInfo);
			}
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

bool UAbilitySystemComponent::ServerInputRelease_Validate(FGameplayAbilitySpecHandle Handle)
{
	return true;
}

void UAbilitySystemComponent::ServerInputRelease_Implementation(FGameplayAbilitySpecHandle Handle)
{
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
	if (Spec)
	{
		AbilitySpectInputReleased(*Spec);
	}
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

