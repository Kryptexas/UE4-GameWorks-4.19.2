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
	{
		static FName FuncName = FName(TEXT("K2_PredictiveActivateAbility"));
		UFunction* PredictiveActivateFunction = GetClass()->FindFunctionByName(FuncName);
		HasBlueprintPredictiveActivate = PredictiveActivateFunction && PredictiveActivateFunction->GetOuter()->IsA(UBlueprintGeneratedClass::StaticClass());
	}
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
		false;
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

bool UGameplayAbility::CommitAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!CommitCheck(ActorInfo, ActivationInfo))
	{
		return false;
	}

	CommitExecute(ActorInfo, ActivationInfo);

	// Fixme: Should we always call this or only if it is implemented? A noop may not hurt but could be bad for perf (storing a HasBlueprintCommit per instance isn't good either)
	K2_CommitExecute();

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
	if (InstancingPolicy == EGameplayAbilityInstancingPolicy::InstancedPerExecution)
	{
		MarkPendingKill();
	}

	// Remove from owning AbilitySystemComponent?
	// generic way of releasing all callbacks!
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
	ActorInfo->AbilitySystemComponent->CancelAbilitiesWithTags(CancelAbilitiesWithTag, ActorInfo, ActivationInfo, this);

	// Become the 'Targeting Ability'
	// These may need to be more robust - does every ability that activates become the targeting ability, or only certain ones?
	ActorInfo->AbilitySystemComponent->SetTargetAbility(this);
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
	SetCurrentInfo(ActorInfo, ActivationInfo);
	
	check(ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting);

	if (HasBlueprintPredictiveActivate)
	{
		K2_PredictiveActivateAbility();
	}
	
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
			return Durations[Durations.Num()-1];
		}
	}

	return 0.f;
}

FGameplayAbilityActorInfo UGameplayAbility::GetActorInfo()
{
	check(CurrentActorInfo);
	return *CurrentActorInfo;
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

void UGameplayAbility::ClientActivateAbilitySucceed_Implementation(int32 PredictionKey)
{
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

void FGameplayAbilityActorInfo::InitFromActor(AActor *InActor)
{
	Actor = InActor;

	UStruct * Struct = FGameplayAbilityActorInfo::StaticStruct();
	UScriptStruct * ScriptStruct = FGameplayAbilityActorInfo::StaticStruct();

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

	AbilitySystemComponent = InActor->FindComponentByClass<UAbilitySystemComponent>();

	MovementComponent = InActor->FindComponentByClass<UMovementComponent>();
}

bool FGameplayAbilityActorInfo::IsLocallyControlled() const
{
	if (PlayerController.IsValid())
	{
		return PlayerController->IsLocalController();
	}

	return false;
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
			
			ABILITY_LOG(Fatal, TEXT("FGameplayAbilityTargetDataHandle::NetSerialize called on data struct %s without a native NetSerilaize"), *ScriptStruct->GetName());

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
	

	ABILITY_LOG(Warning, TEXT("FGameplayAbilityTargetDataHandle Serialized: %s"), ScriptStruct ? *ScriptStruct->GetName() : TEXT("NULL") );

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

