// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagsModule.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilityTask.h"

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

// TODO: polymorphic payload
void UGameplayAbility::SendGameplayEvent(FGameplayTag EventTag, FGameplayEventData Payload)
{
	if (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::Predictive)
	{
		Payload.PredictionKey = CurrentActivationInfo.GetPredictionKeyForNewAction();
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
		AActor* OwnerActor = Cast<AActor>(GetOuter());
		if (ensure(OwnerActor))
		{
			UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
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

bool UGameplayAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	SetCurrentActorInfo(Handle, ActorInfo);

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

bool UGameplayAbility::CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	// Last chance to fail (maybe we no longer have resources to commit since we after we started this ability activation)
	if (!CommitCheck(ActorInfo, ActivationInfo))
	{
		return false;
	}

	CommitExecute(Handle, ActorInfo, ActivationInfo);

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

void UGameplayAbility::CommitExecute(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	ApplyCooldown(Handle, ActorInfo, ActivationInfo);

	ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void UGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	EndAbility(Handle, ActorInfo, ActivationInfo);
}

void UGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
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

	// Unblock ability tags we previously blocked
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->UnBlockAbilitiesWithTags(BlockAbilitiesWithTag);
	}

	// Tell owning AbilitySystemComponent that we ended so it can do stuff (including MarkPendingKill us)
	ActorInfo->AbilitySystemComponent->NotifyAbilityEnded(Handle, this);
}

void UGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	SetCurrentInfo(Handle, ActorInfo, ActivationInfo);

	if (HasBlueprintActivate)
	{
		// A Blueprinted ActivateAbility function must call CommitAbility somewhere in its execution chain.
		K2_ActivateAbility();
	}
	else
	{
		// Native child classes may want to override ActivateAbility and do something like this:

		// Do stuff...

		if (CommitAbility(Handle, ActorInfo, ActivationInfo))		// ..then commit the ability...
		{			
			//	Then do more stuff...
		}
	}
}

void UGameplayAbility::PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	UAbilitySystemComponent* Comp = ActorInfo->AbilitySystemComponent.Get();

	Comp->CancelAbilitiesWithTags(CancelAbilitiesWithTag, Handle, ActorInfo, ActivationInfo, this);
	Comp->BlockAbilitiesWithTags(BlockAbilitiesWithTag);

	bIsActive = true;
	Comp->NotifyAbilityActivated(Handle, this);
}

void UGameplayAbility::CallActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	PreActivate(Handle, ActorInfo, ActivationInfo);
	ActivateAbility(Handle, ActorInfo, ActivationInfo);
}

void UGameplayAbility::ConfirmActivateSucceed()
{
	// On instanced abilities, update CurrentActivationInfo and call any registered delegates.
	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		PostNetInit();
		check(CurrentActorInfo);
		CurrentActivationInfo.SetActivationConfirmed();

		OnConfirmDelegate.Broadcast(this);
		OnConfirmDelegate.Clear();
	}
}

void UGameplayAbility::InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
}

void UGameplayAbility::InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	OnInputRelease.Broadcast(this);
	OnInputRelease.Clear();
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

void UGameplayAbility::ApplyCooldown(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (CooldownGameplayEffect)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CooldownGameplayEffect, 1.f);
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

void UGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (CostGameplayEffect)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, CostGameplayEffect, 1.f);
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

/** Convenience method for abilities to get owning actor - useful for aiming abilities */
AActor* UGameplayAbility::GetOwningActorFromActorInfo() const
{
	check(CurrentActorInfo);
	return CurrentActorInfo->Actor.Get();
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
FGameplayEffectSpecHandle UGameplayAbility::GetOutgoingGameplayEffectSpec(UGameplayEffect* GameplayEffect, float Level) const
{
	check(CurrentActorInfo && CurrentActorInfo->AbilitySystemComponent.IsValid());
	return CurrentActorInfo->AbilitySystemComponent->GetOutgoingSpec(GameplayEffect, Level);
}

/** Fixme: Naming is confusing here */

bool UGameplayAbility::K2_CommitAbility()
{
	check(CurrentActorInfo);
	return CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
}

void UGameplayAbility::K2_EndAbility()
{
	check(CurrentActorInfo);
	
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
}

// --------------------------------------------------------------------

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStop(const FGameplayAbilityActorInfo* ActorInfo) const
{

}

void UGameplayAbility::MontageBranchPoint_AbilityDecisionStart(const FGameplayAbilityActorInfo* ActorInfo) const
{

}

// --------------------------------------------------------------------

FGameplayAbilityTargetingLocationInfo UGameplayAbility::MakeTargetLocationInfoFromOwnerActor()
{
	FGameplayAbilityTargetingLocationInfo ReturnLocation;
	ReturnLocation.LocationType = EGameplayAbilityTargetingLocationType::ActorTransform;
	ReturnLocation.SourceActor = GetActorInfo().Actor.Get();
	return ReturnLocation;
}

FGameplayAbilityTargetingLocationInfo UGameplayAbility::MakeTargetLocationInfoFromOwnerSkeletalMeshComponent(FName SocketName)
{
	FGameplayAbilityTargetingLocationInfo ReturnLocation;
	ReturnLocation.LocationType = EGameplayAbilityTargetingLocationType::SocketTransform;
	ReturnLocation.SourceComponent = GetActorInfo().AnimInstance.IsValid() ? GetActorInfo().AnimInstance.Get()->GetOwningComponent() : NULL;
	ReturnLocation.SourceSocketName = SocketName;
	return ReturnLocation;
}

//----------------------------------------------------------------------

void UGameplayAbility::TaskStarted(UAbilityTask* NewTask)
{
	ActiveTasks.Add(NewTask);
}

void UGameplayAbility::ConfirmTaskByInstanceName(FName InstanceName, bool bEndTask)
{
	TArray<TWeakObjectPtr<UAbilityTask>> NamedTasks = ActiveTasks.FilterByPredicate<FAbilityInstanceNamePredicate>(FAbilityInstanceNamePredicate(InstanceName));
	for (int32 i = NamedTasks.Num() - 1; i >= 0; --i)
	{
		if (NamedTasks[i].IsValid())
		{
			UAbilityTask* CurrentTask = NamedTasks[i].Get();
			CurrentTask->ExternalConfirm(bEndTask);
		}
	}
}

void UGameplayAbility::EndOrCancelTasksByInstanceName()
{
	for (int32 j = 0; j < EndTaskInstanceNames.Num(); ++j)
	{
		FName InstanceName = EndTaskInstanceNames[j];
		TArray<TWeakObjectPtr<UAbilityTask>> NamedTasks = ActiveTasks.FilterByPredicate<FAbilityInstanceNamePredicate>(FAbilityInstanceNamePredicate(InstanceName));
		for (int32 i = NamedTasks.Num() - 1; i >= 0; --i)
		{
			if (NamedTasks[i].IsValid())
			{
				UAbilityTask* CurrentTask = NamedTasks[i].Get();
				CurrentTask->EndTask();
			}
		}
	}
	EndTaskInstanceNames.Empty();

	for (int32 j = 0; j < CancelTaskInstanceNames.Num(); ++j)
	{
		FName InstanceName = CancelTaskInstanceNames[j];
		TArray<TWeakObjectPtr<UAbilityTask>> NamedTasks = ActiveTasks.FilterByPredicate<FAbilityInstanceNamePredicate>(FAbilityInstanceNamePredicate(InstanceName));
		for (int32 i = NamedTasks.Num() - 1; i >= 0; --i)
		{
			if (NamedTasks[i].IsValid())
			{
				UAbilityTask* CurrentTask = NamedTasks[i].Get();
				CurrentTask->ExternalCancel();
			}
		}
	}
	CancelTaskInstanceNames.Empty();
}

void UGameplayAbility::EndTaskByInstanceName(FName InstanceName)
{
	//Avoid race condition by delaying for one frame
	EndTaskInstanceNames.AddUnique(InstanceName);
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGameplayAbility::EndOrCancelTasksByInstanceName);
}

void UGameplayAbility::CancelTaskByInstanceName(FName InstanceName)
{
	//Avoid race condition by delaying for one frame
	CancelTaskInstanceNames.AddUnique(InstanceName);
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UGameplayAbility::EndOrCancelTasksByInstanceName);
}

void UGameplayAbility::TaskEnded(UAbilityTask* Task)
{
	ActiveTasks.Remove(Task);
}

/**
 *	Helper methods for adding GaAmeplayCues without having to go through GameplayEffects.
 *	For now, none of these will happen predictively. We can eventually build this out more to 
 *	work with the PredictionKey system.
 */

void UGameplayAbility::K2_ExecuteGameplayCue(FGameplayTag GameplayCueTag)
{
	check(CurrentActorInfo);
	CurrentActorInfo->AbilitySystemComponent->ExecuteGameplayCue(GameplayCueTag, CurrentActivationInfo.GetPredictionKeyForNewAction());
}

void UGameplayAbility::K2_AddGameplayCue(FGameplayTag GameplayCueTag)
{
	check(CurrentActorInfo);
	CurrentActorInfo->AbilitySystemComponent->AddGameplayCue(GameplayCueTag, CurrentActivationInfo.GetPredictionKeyForNewAction());
}

void UGameplayAbility::K2_RemoveGameplayCue(FGameplayTag GameplayCueTag)
{
	check(CurrentActorInfo);
	CurrentActorInfo->AbilitySystemComponent->RemoveGameplayCue(GameplayCueTag, CurrentActivationInfo.GetPredictionKeyForNewAction());
}

int32 UGameplayAbility::GetAbilityLevel() const
{
	check(IsInstantiated()); // You should not call this on non instanced abilities.
	check(CurrentActorInfo);
	return GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
}

/** Returns current ability level for non instanced abilities. You m ust call this version in these contexts! */
int32 UGameplayAbility::GetAbilityLevel(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
{
	FGameplayAbilitySpec* Spec = ActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(Handle);
	check(Spec);

	return Spec->Level;
}

FGameplayAbilitySpec* UGameplayAbility::GetCurrentAbilitySpec() const
{
	check(IsInstantiated()); // You should not call this on non instanced abilities.
	check(CurrentActorInfo);
	return CurrentActorInfo->AbilitySystemComponent->FindAbilitySpecFromHandle(CurrentSpecHandle);
}

bool UGameplayAbility::IsTriggered() const
{
	// Assume that if there is triggered data, then we are triggered. 
	// If we need to support abilities that can be both, this will need to be expanded.
	return AbilityTriggers.Num() > 0;
}

// -------------------------------------------------------

FActiveGameplayEffectHandle UGameplayAbility::K2_ApplyGameplayEffectToOwner(const UGameplayEffect* GameplayEffect, int32 GameplayEffectLevel)
{
	check(CurrentActorInfo);
	check(CurrentSpecHandle.IsValid());

	//CurrentActivationInfo = GetCurrentAbilitySpec()->ActivationInfo;

	return ApplyGameplayEffectToOwner(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, GameplayEffect, GameplayEffectLevel);
}

FActiveGameplayEffectHandle UGameplayAbility::ApplyGameplayEffectToOwner(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const UGameplayEffect* GameplayEffect, int32 GameplayEffectLevel)
{
	if (ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Authority || ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting)
	{
		return ActorInfo->AbilitySystemComponent->ApplyGameplayEffectToSelf(GameplayEffect, 1.f, ActorInfo->Actor.Get(), FModifierQualifier().PredictionKey(ActivationInfo.GetPredictionKeyForNewAction()));
	}

	// We cannot apply GameplayEffects in this context. Return an empty handle.
	return FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle UGameplayAbility::K2_ApplyGameplayEffectToTarget(FGameplayAbilityTargetDataHandle Target, const UGameplayEffect* GameplayEffect, int32 GameplayEffectLevel)
{
	return ApplyGameplayEffectToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, Target, GameplayEffect, GameplayEffectLevel);
}

FActiveGameplayEffectHandle UGameplayAbility::ApplyGameplayEffectToTarget(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayAbilityTargetDataHandle Target, const UGameplayEffect* GameplayEffect, int32 GameplayEffectLevel)
{
	FActiveGameplayEffectHandle EffectHandle;
	for (auto Data : Target.Data)
	{
		TArray<FActiveGameplayEffectHandle> EffectHandles = Data->ApplyGameplayEffect(GameplayEffect, ActorInfo, (float)GameplayEffectLevel);
		if (EffectHandles.Num() > 0)
		{
			EffectHandle = EffectHandles[0];
			if (EffectHandles.Num() > 1)
			{
				ABILITY_LOG(Warning, TEXT("ApplyGameplayEffectToTargetData_Single called on TargetData with multiple actor targets. %s"), *GetName());
			}
		}
	}
	return EffectHandle;
}