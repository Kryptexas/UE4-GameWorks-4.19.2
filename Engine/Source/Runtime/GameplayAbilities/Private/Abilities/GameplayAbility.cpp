// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagsModule.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilityTask.h"
#pragma optimize("",off)
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

	{
		static FName FuncName = FName(TEXT("K2_ShouldAbilityRespondToEvent"));
		UFunction* ShouldRespondFunction = GetClass()->FindFunctionByName(FuncName);
		HasBlueprintShouldAbilityRespondToEvent = ShouldRespondFunction && ShouldRespondFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	}
	{
		static FName FuncName = FName(TEXT("K2_CanActivateAbility"));
		UFunction* CanActivateFunction = GetClass()->FindFunctionByName(FuncName);
		HasBlueprintCanUse = CanActivateFunction && CanActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	}
	{
		static FName FuncName = FName(TEXT("K2_ActivateAbility"));
		UFunction* ActivateFunction = GetClass()->FindFunctionByName(FuncName);
		HasBlueprintActivate = ActivateFunction && ActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	}
	
#if WITH_EDITOR
	/** Autoregister abilities with the blueprint debugger in the editor.*/
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		UBlueprint* BP = Cast<UBlueprint>(GetClass()->ClassGeneratedBy);
		if (BP && (BP->GetWorldBeingDebugged() == nullptr || BP->GetWorldBeingDebugged() == GetWorld()))
		{
			BP->SetObjectBeingDebugged(this);
		}
	}
#endif
}

void UGameplayAbility::TriggerAbilityFromGameplayEvent(FGameplayAbilityActorInfo* ActorInfo, FGameplayTag EventTag, FGameplayEventData* Payload, UAbilitySystemComponent& Component)
{
	if (ShouldAbilityRespondToEvent(EventTag, Payload))
	{
		int32 ExecutingAbilityIndex = -1;

		// if we're the server and this is coming from a predicted event we should check if the client has already predicted it
		if (Payload->CurrPredictionKey 
			&& NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Predictive
			&& ActorInfo->Actor->Role == ROLE_Authority)
		{
			bool bPendingClientAbilityFound = false;
			for (auto PendingAbilityInfo : Component.PendingClientAbilities)
			{
				if (Payload->CurrPredictionKey == PendingAbilityInfo.PrevPredictionKey && this == PendingAbilityInfo.Ability) // found a match
				{
					Payload->PrevPredictionKey = PendingAbilityInfo.PrevPredictionKey;
					Payload->CurrPredictionKey = PendingAbilityInfo.CurrPredictionKey;

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
				Info.CurrPredictionKey = Payload->CurrPredictionKey;
				Info.Ability = this;

				ExecutingAbilityIndex = Component.ExecutingServerAbilities.Add(Info);
			}
		}

		if (TryActivateAbility(ActorInfo, Payload->PrevPredictionKey, Payload->CurrPredictionKey))
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

// TODO: polymorphic payload
void UGameplayAbility::SendGameplayEvent(FGameplayTag EventTag, FGameplayEventData Payload)
{
	if (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Predictive)
	{
		check(CurrentActivationInfo.CurrPredictionKey != 0);
		Payload.PrevPredictionKey = CurrentActivationInfo.PrevPredictionKey;
		Payload.CurrPredictionKey = CurrentActivationInfo.CurrPredictionKey;
	}

	AActor *OwnerActor = Cast<AActor>(GetOuter());
	if (OwnerActor)
	{
		UAbilitySystemComponent* AbilitySystemComponent = OwnerActor->FindComponentByClass<UAbilitySystemComponent>();
		if (ensure(AbilitySystemComponent))
		{
			AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);
		}
	}
}

void UGameplayAbility::PostNetInit()
{
	/** We were dynamically spawned from replication - we need to init a currentactorinfo by looking at outer.
	 *  This may need to be updated further if we start having abilities live on different outers than player AbilitySystemComponents.
	 */
	
	if (CurrentActorInfo == NULL)
	{
		AActor *OwnerActor = Cast<AActor>(GetOuter());
		if (ensure(OwnerActor))
		{
			UAbilitySystemComponent* AbilitySystemComponent = OwnerActor->FindComponentByClass<UAbilitySystemComponent>();
			if (ensure(AbilitySystemComponent))
			{
				CurrentActorInfo = AbilitySystemComponent->AbilityActorInfo.Get();
			}
		}
	}
}

bool UGameplayAbility::IsActive() const
{
	// Only Instanced-Per-Actor abilities persist between activations
	if (GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerActor)
	{
		return bIsActive;
	}

	// Non-instanced and Instanced-Per-Execution abilities are by definition active unless they are pending kill
	return !IsPendingKill();
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
	SetCurrentActorInfo(ActorInfo);

	if (ActorInfo->AbilitySystemComponent->GetUserAbilityActivationInhibited())
	{
		/**
		 *	Input is inhibited (UI is pulled up, another ability may be blocking all other input, etc).
		 *	When we get into triggered abilities, we may need to better differentiate between CanActviate and CanUserActivate or something.
		 *	E.g., we would want LMB/RMB to be inhibited while the user is in the menu UI, but we wouldn't want to prevent a 'buff when I am low health'
		 *	ability to not trigger.
		 *	
		 *	Basically: CanActivateAbility is only used by user activated abilities now. If triggered abilities need to check costs/cooldowns, then we may
		 *	want to split this function up and change the calling API to distinguish between 'can I initiate an ability activation' and 'can this ability be activated'.
		 */ 
		return false;
	}
	
	if (!CheckCooldown(ActorInfo))
	{
		return false;
	}

	if (!CheckCost(ActorInfo))
	{
		return false;
	}

	if (HasBlueprintCanUse)
	{
		if (K2_CanActivateAbility(*ActorInfo) == false)
		{
			ABILITY_LOG(Log, TEXT("CanActivateAbility %s failed, blueprint refused"), *GetName());
			return false;
		}
	}

	return true;
}

bool UGameplayAbility::ShouldAbilityRespondToEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) const
{
	if (HasBlueprintShouldAbilityRespondToEvent)
	{
		if (K2_ShouldAbilityRespondToEvent(*Payload) == false)
		{
			ABILITY_LOG(Log, TEXT("ShouldAbilityRespondToEvent %s failed, blueprint refused"), *GetName());
			return false;
		}
	}

	return true;
}

bool UGameplayAbility::CommitAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Last chance to fail (maybe we no longer have resources to commit since we after we started this ability activation)
	if (!CommitCheck(ActorInfo, ActivationInfo))
	{
		return false;
	}

	CommitExecute(ActorInfo, ActivationInfo);

	// Fixme: Should we always call this or only if it is implemented? A noop may not hurt but could be bad for perf (storing a HasBlueprintCommit per instance isn't good either)
	K2_CommitExecute();

	// Broadcast this commitment
	ActorInfo->AbilitySystemComponent->NotifyAbilityCommit(this);

	return true;
}

bool UGameplayAbility::CommitCheck(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	/**
	 *	Checks if we can (still) commit this ability. There are some subtleties here.
	 *		-An ability can start activating, play an animation, wait for a user confirmation/target data, and then actually commit
	 *		-Commit = spend resources/cooldowns. Its possible the source has changed state since he started activation, so a commit may fail.
	 *		-We don't want to just call CanActivateAbility() since right now that also checks things like input inhibition.
	 *			-E.g., its possible the act of starting your ability makes it no longer activatable (CanaCtivateAbility() may be false if called here).
	 */

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

void UGameplayAbility::CommitExecute(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!ActorInfo->AbilitySystemComponent->IsNetSimulating() || ActivationInfo.CurrPredictionKey != 0)
	{
		ApplyCooldown(ActorInfo, ActivationInfo);

		ApplyCost(ActorInfo, ActivationInfo);
	}
}

bool UGameplayAbility::TryActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, uint32 PrevPredictionKey, uint32 CurrPredictionKey, UGameplayAbility ** OutInstancedAbility)
{	
	// make sure the ActorInfo and then Actor on that FGameplayAbilityActorInfo are valid, if not bail out.
	if (ActorInfo == NULL || ActorInfo->Actor == NULL)
	{
		return false;
	}

	// This should only come from button presses/local instigation
	ENetRole NetMode = ActorInfo->Actor->Role;
	ensure(NetMode != ROLE_SimulatedProxy);
	
	FGameplayAbilityActivationInfo	ActivationInfo(ActorInfo->Actor.Get(), PrevPredictionKey, CurrPredictionKey);

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
		
		// This must be called immediately after GeneratePredictionKey to prevent problems with recursively activating abilities
		ServerTryActivateAbility(ActorInfo, ActivationInfo);

		if (GetInstancingPolicy() == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
		{
			// For now, only NonReplicated + InstancedPerExecution abilities can be Predictive.
			// We lack the code to predict spawning an instance of the execution and then merge/combine
			// with the server spawned version when it arrives.
			
			if(GetReplicationPolicy() == EGameplayAbilityReplicationPolicy::ReplicateNone)
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
				ensure(false);
			}
		}
		else
		{
			CallActivateAbility(ActorInfo, ActivationInfo);
		}
	}

	return true;	
}

void UGameplayAbility::CancelAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(ActorInfo);
}

void UGameplayAbility::EndAbility(const FGameplayAbilityActorInfo* ActorInfo)
{
	bIsActive = false;

	// Tell all our tasks that we are finished and they should cleanup
	for (TWeakObjectPtr<UAbilityTask> Task : ActiveTasks)
	{
		if (Task.IsValid())
		{
			Task.Get()->AbilityEnded();
		}
	}
	ActiveTasks.Reset();	// Empty the array but dont resize memory, since this object is probably going to be destroyed very soon anyways.

	// Tell owning AbilitySystemComponent that we ended so it can do stuff (including MarkPendingKill us)
	ActorInfo->AbilitySystemComponent->NotifyAbilityEnded(this);
}

void UGameplayAbility::ActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	SetCurrentInfo(ActorInfo, ActivationInfo);

	if (HasBlueprintActivate)
	{
		// A Blueprinted ActivateAbility function must call CommitAbility somewhere in its execution chain.
		K2_ActivateAbility();
	}
	else
	{
		// Native child classes may want to override ActivateAbility and do something like this:

		// Do stuff...

		if (CommitAbility(ActorInfo, ActivationInfo))		// ..then commit the ability...
		{			
			//	Then do more stuff...
		}
	}
}

void UGameplayAbility::PreActivate(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UAbilitySystemComponent* Comp = ActorInfo->AbilitySystemComponent.Get();

	Comp->CancelAbilitiesWithTags(CancelAbilitiesWithTag, ActorInfo, ActivationInfo, this);

	bIsActive = true;
	Comp->NotifyAbilityActivated(this);

	// Become the 'Targeting Ability'
	// These may need to be more robust - does every ability that activates become the targeting ability, or only certain ones?
	Comp->SetTargetAbility(this);
}

void UGameplayAbility::CallActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	PreActivate(ActorInfo, ActivationInfo);
	ActivateAbility(ActorInfo, ActivationInfo);
}

void UGameplayAbility::ConfirmActivateSucceed()
{
	CurrentActivationInfo.SetActivationConfirmed();

	OnConfirmDelegate.Broadcast(this);
	OnConfirmDelegate.Clear();
}

void UGameplayAbility::ServerTryActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	ActorInfo->AbilitySystemComponent->ServerTryActivateAbility(this, ActivationInfo.PrevPredictionKey, ActivationInfo.CurrPredictionKey);
}

void UGameplayAbility::ClientActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{ 
	CallActivateAbility(ActorInfo, ActivationInfo);
}

void UGameplayAbility::InputPressed(const FGameplayAbilityActorInfo* ActorInfo)
{
	TryActivateAbility(ActorInfo);
}

void UGameplayAbility::InputReleased(const FGameplayAbilityActorInfo* ActorInfo)
{
	
}

bool UGameplayAbility::CheckCooldown(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (CooldownGameplayEffect)
	{
		check(ActorInfo->AbilitySystemComponent.IsValid());
		if (ActorInfo->AbilitySystemComponent->HasAnyMatchingGameplayTags(CooldownGameplayEffect->OwnedTagsContainer))
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
		ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(CooldownGameplayEffect, 1.f, ActorInfo->Actor.Get(), FModifierQualifier().PredictionKeys(ActivationInfo.PrevPredictionKey, ActivationInfo.CurrPredictionKey));
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
		ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(CostGameplayEffect, 1.f, ActorInfo->Actor.Get(), FModifierQualifier().PredictionKeys(ActivationInfo.PrevPredictionKey, ActivationInfo.CurrPredictionKey));
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
			return Durations[Durations.Num()-1];
		}
	}

	return 0.f;
}

FGameplayAbilityActorInfo UGameplayAbility::GetActorInfo() const
{
	check(CurrentActorInfo);
	return *CurrentActorInfo;
}

/** Convenience method for abilities to get skeletal mesh component - useful for aiming abilities */
USkeletalMeshComponent* UGameplayAbility::GetOwningComponentFromActorInfo() const
{
	check(CurrentActorInfo);
	if (CurrentActorInfo->AnimInstance.IsValid())
	{
		return CurrentActorInfo->AnimInstance.Get()->GetOwningComponent();
	}
	return NULL;
}

/** Convenience method for abilities to get outgoing gameplay effect specs (for example, to pass on to projectiles to apply to whoever they hit) */
FGameplayEffectSpecHandle UGameplayAbility::GetOutgoingSpec(UGameplayEffect* GameplayEffect) const
{
	check(CurrentActorInfo && CurrentActorInfo->AbilitySystemComponent.IsValid());
	return CurrentActorInfo->AbilitySystemComponent->GetOutgoingSpec(GameplayEffect);
}

/** Fixme: Naming is confusing here */

bool UGameplayAbility::K2_CommitAbility()
{
	check(CurrentActorInfo);
	return CommitAbility(CurrentActorInfo, CurrentActivationInfo);
}

void UGameplayAbility::K2_EndAbility()
{
	check(CurrentActorInfo);
	EndAbility(CurrentActorInfo);
}


int32 UGameplayAbility::GetFunctionCallspace(UFunction* Function, void* Parameters, FFrame* Stack)
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return FunctionCallspace::Local;
	}
	check(GetOuter() != NULL);
	return GetOuter()->GetFunctionCallspace(Function, Parameters, Stack);
}

bool UGameplayAbility::CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack)
{
	check(!HasAnyFlags(RF_ClassDefaultObject));
	check(GetOuter() != NULL);

	AActor* Owner = CastChecked<AActor>(GetOuter());
	UNetDriver* NetDriver = Owner->GetNetDriver();
	if (NetDriver)
	{
		NetDriver->ProcessRemoteFunction(Owner, Function, Parameters, OutParms, Stack, this);
		return true;
	}

	return false;
}

// --------------------------------------------------------------------

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStop(const FGameplayAbilityActorInfo* ActorInfo) const
{

}

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStart(const FGameplayAbilityActorInfo* ActorInfo) const
{

}

FGameplayAbilityTargetingLocationInfo UGameplayAbility::MakeTargetLocationInfoFromOwnerActor() const
{
	FGameplayAbilityTargetingLocationInfo ReturnLocation;
	ReturnLocation.LocationType = EGameplayAbilityTargetingLocationType::ActorTransform;
	ReturnLocation.SourceActor = GetActorInfo().Actor.Get();
	return ReturnLocation;
}

FGameplayAbilityTargetingLocationInfo UGameplayAbility::MakeTargetLocationInfoFromOwnerSkeletalMeshComponent(FName SocketName) const
{
	FGameplayAbilityTargetingLocationInfo ReturnLocation;
	ReturnLocation.LocationType = EGameplayAbilityTargetingLocationType::SocketTransform;
	ReturnLocation.SourceComponent = GetActorInfo().AnimInstance.IsValid() ? GetActorInfo().AnimInstance.Get()->GetOwningComponent() : NULL;
	ReturnLocation.SourceSocketName = SocketName;
	return ReturnLocation;
}

void UGameplayAbility::ClientActivateAbilitySucceed_Implementation(int32 PredictionKey)
{
	PostNetInit();

	// Child classes can't override ClientActivateAbilitySucceed_Implementation, so just call ClientActivateAbilitySucceed_Internal
	ClientActivateAbilitySucceed_Internal(PredictionKey);
}

void UGameplayAbility::ClientActivateAbilitySucceed_Internal(int32 PredictionKey)
{
	check(CurrentActorInfo);
	CurrentActivationInfo.SetActivationConfirmed();

	CallActivateAbility(CurrentActorInfo, CurrentActivationInfo);
}

//----------------------------------------------------------------------

void UGameplayAbility::TaskStarted(UAbilityTask* NewTask)
{
	ActiveTasks.Add(NewTask);
}

void UGameplayAbility::CancelTaskByInstanceName(FName InstanceName)
{
	TArray<TWeakObjectPtr<UAbilityTask>> NamedTasks = ActiveTasks.FilterByPredicate<FAbilityInstanceNamePredicate>(FAbilityInstanceNamePredicate(InstanceName));
	for (int32 i = NamedTasks.Num() - 1; i >= 0; --i)
	{
		if (NamedTasks[i].IsValid())
		{
			NamedTasks[i].Get()->EndTask();
		}
	}
}

void UGameplayAbility::TaskEnded(UAbilityTask* Task)
{
	ActiveTasks.Remove(Task);
}

//----------------------------------------------------------------------

void FGameplayAbilityActorInfo::InitFromActor(AActor *InActor, UAbilitySystemComponent* InAbilitySystemComponent)
{
	check(InActor);
	check(InAbilitySystemComponent);

	Actor = InActor;
	AbilitySystemComponent = InAbilitySystemComponent;

	// Look for a player controller or pawn in the owner chain.
	AActor *TestActor = InActor;
	while(TestActor)
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
	//check(ActivationMode == EGameplayAbilityActivationMode::NonAuthority);
	ActivationMode = EGameplayAbilityActivationMode::Confirmed;
}

//----------------------------------------------------------------------

void FGameplayAbilityTargetData::ApplyGameplayEffect(UGameplayEffect* GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo)
{
	// TODO: Improve relationship between InstigatorContext and FGameplayAbilityTargetData/FHitResult (or use something different between HitResult)
	

	FGameplayEffectSpec	SpecToApply(GameplayEffect,					// The UGameplayEffect data asset
									InstigatorInfo.Actor.Get(),		// The actor who instigated this
									1.f,							// FIXME: Leveling
									NULL							// FIXME: CurveData override... should we just remove this?
									);
	if (HasHitResult())
	{
		SpecToApply.InstigatorContext.AddHitResult(*GetHitResult());
	}

	if (HasOrigin())
	{
		SpecToApply.InstigatorContext.AddOrigin(GetOrigin().GetLocation());
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


FGameplayAbilityTargetDataHandle FGameplayAbilityTargetingLocationInfo::MakeTargetDataHandleFromHitResult(TWeakObjectPtr<UGameplayAbility> Ability, FHitResult HitResult) const
{
	if (LocationType == EGameplayAbilityTargetingLocationType::Type::SocketTransform)
	{
		const FGameplayAbilityActorInfo* ActorInfo = Ability.IsValid() ? Ability.Get()->GetCurrentActorInfo() : NULL;
		AActor* AISourceActor = ActorInfo ? (ActorInfo->Actor.IsValid() ? ActorInfo->Actor.Get() : NULL) : NULL;
		UAnimInstance* AnimInstance = ActorInfo ? ActorInfo->AnimInstance.Get() : NULL;
		USkeletalMeshComponent* AISourceComponent = AnimInstance ? AnimInstance->GetOwningComponent() : NULL;

		if (AISourceActor && AISourceComponent)
		{
			FGameplayAbilityTargetData_Mesh* ReturnData = new FGameplayAbilityTargetData_Mesh();
			ReturnData->SourceActor = AISourceActor;
			ReturnData->SourceComponent = AISourceComponent;
			ReturnData->SourceSocketName = SourceSocketName;
			ReturnData->TargetPoint = HitResult.Location;
			return FGameplayAbilityTargetDataHandle(ReturnData);
		}
	}

	/** Note: These are cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
	FGameplayAbilityTargetData_SingleTargetHit* ReturnData = new FGameplayAbilityTargetData_SingleTargetHit();
	ReturnData->HitResult = HitResult;
	return FGameplayAbilityTargetDataHandle(ReturnData);
}

FGameplayAbilityTargetDataHandle FGameplayAbilityTargetingLocationInfo::MakeTargetDataHandleFromActors(TArray<AActor*> TargetActors) const
{
	/** Note: This is cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
	FGameplayAbilityTargetData_ActorArray* ReturnData = new FGameplayAbilityTargetData_ActorArray();
	ReturnData->TargetActorArray = TargetActors;
	ReturnData->SourceLocation = *this;
	return FGameplayAbilityTargetDataHandle(ReturnData);
}

bool FGameplayAbilityTargetDataHandle::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{

	UScriptStruct * ScriptStruct = Data.IsValid() ? Data->GetScriptStruct() : NULL;
	Ar << ScriptStruct;

	if (ScriptStruct)
	{
		if (Ar.IsLoading())
		{
			// For now, just always reset/reallocate the data when loading.
			// Longer term if we want to generalize this and use it for property replication, we should support
			// only reallocating when necessary
			check(!Data.IsValid());

			FGameplayAbilityTargetData * NewData = (FGameplayAbilityTargetData*)FMemory::Malloc(ScriptStruct->GetCppStructOps()->GetSize());
			ScriptStruct->InitializeScriptStruct(NewData);

			Data = TSharedPtr<FGameplayAbilityTargetData>(NewData);
		}

		void* ContainerPtr = Data.Get();

		if (ScriptStruct->StructFlags & STRUCT_NetSerializeNative)
		{
			ScriptStruct->GetCppStructOps()->NetSerialize(Ar, Map, bOutSuccess, Data.Get());
		}
		else
		{
			// This won't work since UStructProperty::NetSerializeItem is deprecrated.
			//	1) we have to manually crawl through the topmost struct's fields since we don't have a UStructProperty for it (just the UScriptProperty)
			//	2) if there are any UStructProperties in the topmost struct's fields, we will assert in UStructProperty::NetSerializeItem.
			
			ABILITY_LOG(Fatal, TEXT("FGameplayAbilityTargetDataHandle::NetSerialize called on data struct %s without a native NetSerialize"), *ScriptStruct->GetName());

			for (TFieldIterator<UProperty> It(ScriptStruct); It; ++It)
			{
				if (It->PropertyFlags & CPF_RepSkip)
				{
					continue;
				}

				void * PropertyData = It->ContainerPtrToValuePtr<void*>(ContainerPtr);

				It->NetSerializeItem(Ar, Map, PropertyData);
			}
		}
	}
	

	//ABILITY_LOG(Warning, TEXT("FGameplayAbilityTargetDataHandle Serialized: %s"), ScriptStruct ? *ScriptStruct->GetName() : TEXT("NULL") );

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetingLocationInfo::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << LocationType;

	switch (LocationType)
	{
	case EGameplayAbilityTargetingLocationType::ActorTransform:
		Ar << SourceActor;
		break;
	case EGameplayAbilityTargetingLocationType::SocketTransform:
		Ar << SourceComponent;
		Ar << SourceSocketName;
		break;
	case EGameplayAbilityTargetingLocationType::LiteralTransform:
		Ar << LiteralTransform;
		break;
	default:
		check(false);		//This case should not happen
		break;
	}

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_LocationInfo::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	SourceLocation.NetSerialize(Ar, Map, bOutSuccess);
	TargetLocation.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_ActorArray::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	SourceLocation.NetSerialize(Ar, Map, bOutSuccess);
	Ar << TargetActorArray;

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_Mesh::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	//SourceActor can be used as a backup if the component isn't found.
	Ar << SourceActor;
	Ar << SourceComponent;
	Ar << SourceSocketName;
	TargetPoint.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}

bool FGameplayAbilityTargetData_SingleTargetHit::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	Ar << HitResult.Actor;

	HitResult.Location.NetSerialize(Ar, Map, bOutSuccess);
	HitResult.Normal.NetSerialize(Ar, Map, bOutSuccess);

	bOutSuccess = true;
	return true;
}



// -----------------------------------------------------------------


/**
 *	Helper methods for adding GaAmeplayCues without having to go through GameplayEffects.
 *	For now, none of these will happen predictively. We can eventually build this out more to 
 *	work with the PredictionKey system.
 */

void UGameplayAbility::ExecuteGameplayCue(FGameplayTag GameplayCueTag)
{
	if (ensure(CurrentActorInfo) && CurrentActorInfo->IsNetAuthority())
	{
		CurrentActorInfo->AbilitySystemComponent->ExecuteGameplayCue(GameplayCueTag);
	}
}

void UGameplayAbility::AddGameplayCue(FGameplayTag GameplayCueTag)
{
	if (ensure(CurrentActorInfo) && CurrentActorInfo->IsNetAuthority())
	{
		CurrentActorInfo->AbilitySystemComponent->AddGameplayCue(GameplayCueTag);
	}
}

void UGameplayAbility::RemoveGameplayCue(FGameplayTag GameplayCueTag)
{
	if (ensure(CurrentActorInfo) && CurrentActorInfo->IsNetAuthority())
	{
		CurrentActorInfo->AbilitySystemComponent->RemoveGameplayCue(GameplayCueTag);
	}
}
