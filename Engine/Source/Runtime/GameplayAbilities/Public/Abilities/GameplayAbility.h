// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "Runtime/Engine/Classes/Animation/AnimInstance.h"
#include "GameplayAbilityTargetTypes.h"
#include "GameplayAbilityTypes.h"
#include "GameplayEffect.h"
#include "GameplayAbility.generated.h"

/**
 * UGameplayAbility
 *	
 *	Abilities define custom gameplay logic that can be activated or triggered.
 *	
 *	The main features provided by the AbilitySystem for GameplayAbilities are: 
 *		-CanUse functionality:
 *			-Cooldowns
 *			-Resources (mana, stamina, etc)
 *			-etc
 *			
 *		-Replication support
 *			-Client/Server communication for ability activation
 *			-Client prediction for ability activation
 *			
 *		-Instancing support
 *			-Abilities can be non-instanced (default)
 *			-Instanced per owner
 *			-Instanced per execution
 *			
 *		-Basic, extendable support for:
 *			-Input binding
 *			-'Giving' abilities (that can be used) to actors
 *	
 *	
 *	 
 *	
 *	The intention is for programmers to create these non instanced abilities in C++. Designers can then
 *	extend them as data assets (E.g., they can change default properties, they cannot implement blueprint graphs).
 *	
 *	See GameplayAbility_Montage for example.
 *		-Plays a montage and applies a GameplayEffect to its target while the montage is playing.
 *		-When finished, removes GameplayEffect.
 *		
 *	
 *	Note on replication support:
 *		-Non instanced abilities have limited replication support. 
 *			-Cannot have state (obviously) so no replicated properties
 *			-RPCs on the ability class are not possible either.
 *			
 *			-However: generic RPC functionality can be achieved through the UAbilitySystemAttribute.
 *				-E.g.: ServerTryActivateAbility(class UGameplayAbility* AbilityToActivate, int32 PredictionKey)
 *				
 *	A lot is possible with non instanced abilities but care must be taken.
 *	
 *	
 *	To support state or event replication, an ability must be instanced. This can be done with the InstancingPolicy property.
 *	
 *
 *	
 */


/** TriggerData */
USTRUCT()
struct FAbilityTriggerData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=TriggerData)
	FGameplayTag TriggerTag;
};

/**
 *	Abilities define custom gameplay logic that can be activated by players or external game logic.
 */
UCLASS(Blueprintable)
class GAMEPLAYABILITIES_API UGameplayAbility : public UObject
{
	GENERATED_UCLASS_BODY()

	friend class UAbilitySystemComponent;
	friend class UGameplayAbilitySet;

public:

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	The important functions:
	//	
	//		CanActivateAbility()	- const function to see if ability is activatable. Callable by UI etc
	//
	//		TryActivateAbility()	- Attempts to activate the ability. Calls CanActivateAbility(). Input events can call this directly.
	//								- Also handles instancing-per-execution logic and replication/prediction calls.
	//		
	//		CallActivate()			- Protected, non virtual function. Does some boilerplate 'pre activate' stuff, then calls Activate()
	//
	//		Activate()				- What the abilities *does*. This is what child classes want to override.
	//	
	//		Commit()				- Commits reources/cooldowns etc. Activate() must call this!
	//		
	//		CancelAbility()			- Interrupts the ability (from an outside source).
	//									-We may want to add some info on what/who cancelled.
	//
	//		EndAbility()			- The ability has ended. This is intended to be called by the ability to end itself.
	//	
	// ----------------------------------------------------------------------------------------------------------------
	
	/** Input binding. Base implementation calls TryActivateAbility */
	virtual void InputPressed(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** Input binding. Base implementation does nothing */
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** Returns true if this ability can be activated right now. Has no side effects */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

	/** Returns true if this ability can be triggered right now. Has no side effects */
	virtual bool ShouldAbilityRespondToEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) const;
	
	float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const;
		
	EGameplayAbilityInstancingPolicy::Type GetInstancingPolicy() const
	{
		return InstancingPolicy;
	}

	EGameplayAbilityReplicationPolicy::Type GetReplicationPolicy() const
	{
		return ReplicationPolicy;
	}

	EGameplayAbilityNetExecutionPolicy::Type GetNetExecutionPolicy() const
	{
		return NetExecutionPolicy;
	}

	/** Gets the current actor info bound to this ability - can only be called on instanced abilities. */
	const FGameplayAbilityActorInfo* GetCurrentActorInfo() const
	{
		check(IsInstantiated());
		return CurrentActorInfo;
	}

	/** Gets the current activation info bound to this ability - can only be called on instanced abilities. */
	FGameplayAbilityActivationInfo GetCurrentActivationInfo() const
	{
		check(IsInstantiated());
		return CurrentActivationInfo;
	}

	/** Gets the current AbilitySpecHandle- can only be called on instanced abilities. */
	FGameplayAbilitySpecHandle GetCurrentAbilitySpecHandle() const
	{
		check(IsInstantiated());
		return CurrentSpecHandle;
	}

	/** Retrieves the actual AbilitySpec for this ability. Can only be called on instanced abilities. */
	FGameplayAbilitySpec* GetCurrentAbilitySpec() const;

	virtual UWorld* GetWorld() const override
	{
		if (!IsInstantiated())
		{
			// If we are a CDO, we must return nullptr instead of calling Outer->GetWorld() to fool UObject::ImplementsGetWorld.
			return nullptr;
		}
		return GetOuter()->GetWorld();
	}

	int32 GetFunctionCallspace(UFunction* Function, void* Parameters, FFrame* Stack) override;

	bool CallRemoteFunction(UFunction* Function, void* Parameters, FOutParmRec* OutParms, FFrame* Stack) override;

	void PostNetInit();

	/** Returns true if the ability is currently active */
	bool IsActive() const;

	/** This ability has these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags)
	FGameplayTagContainer AbilityTags;

	/** Callback for when this ability has been confirmed by the server */
	FGenericAbilityDelegate	OnConfirmDelegate;

	void TaskStarted(UAbilityTask* NewTask);

	void TaskEnded(UAbilityTask* Task);

	/** GenericCallback for Input being released. Auto cleared on broadcast */
	FGenericAbilityDelegate	OnInputRelease;

	/** Is this ability triggered from TriggerData (or is it triggered explicitly through input/game code) */
	bool IsTriggered() const;

protected:

	// --------------------------------------
	//	ShouldAbilityRespondToEvent
	// --------------------------------------

	/** Returns true if this ability can be activated right now. Has no side effects */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, FriendlyName = "ShouldAbilityRespondToEvent")
	virtual bool K2_ShouldAbilityRespondToEvent(FGameplayEventData Payload) const;

	bool HasBlueprintShouldAbilityRespondToEvent;
		
	// --------------------------------------
	//	CanActivate
	// --------------------------------------
	
	/** Returns true if this ability can be activated right now. Has no side effects */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, FriendlyName="CanActivateAbility")
	virtual bool K2_CanActivateAbility(FGameplayAbilityActorInfo ActorInfo) const;

	bool HasBlueprintCanUse;

	// --------------------------------------
	//	ActivateAbility
	// --------------------------------------

	/**
	 * The main function that defines what an ability does.
	 *  -Child classes will want to override this
	 *  -This function must call CommitAbility()
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Ability, FriendlyName = "ActivateAbility")
	virtual void K2_ActivateAbility();	

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	bool HasBlueprintActivate;

	void CallActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** Called on a predictive ability when the server confirms its execution */
	virtual void ConfirmActivateSucceed();

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void SendGameplayEvent(FGameplayTag EventTag, FGameplayEventData Payload);

	// --------------------------------------
	//	CommitAbility
	// --------------------------------------

	/**
	 * Attempts to commit the ability (spend resources, etc). This our last chance to fail.
	 *	-Child classes that override ActivateAbility must call this themselves!
	 */
	UFUNCTION(BlueprintCallable, Category = Ability, FriendlyName = "CommitAbility")
	virtual bool K2_CommitAbility();

	virtual bool CommitAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/**
	 * The last chance to fail before commiting
	 *	-This will usually be the same as CanActivateAbility. Some abilities may need to do extra checks here if they are consuming extra stuff in CommitExecute
	 */
	virtual bool CommitCheck(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	// --------------------------------------
	//	CommitExecute
	// --------------------------------------

	UFUNCTION(BlueprintImplementableEvent, Category = Ability, FriendlyName="CommitExecute")
	virtual void K2_CommitExecute();

	/** Does the commmit atomically (consume resources, do cooldowns, etc) */
	virtual void CommitExecute(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** Do boilerplate init stuff and then call ActivateAbility */
	void PreActivate(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	// --------------------------------------
	//	CancelAbility
	// --------------------------------------

	/** Destroys intanced-per-execution abilities. Instance-per-actor abilities should 'reset'. Non instance abilities - what can we do? */
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** Destroys intanced-per-execution abilities. Instance-per-actor abilities should 'reset'. Non instance abilities - what can we do? */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void ConfirmTaskByInstanceName(FName InstanceName, bool bEndTask);

	/** Internal function, cancels all the tasks we asked to cancel last frame (by instance name). */
	void EndOrCancelTasksByInstanceName();
	TArray<FName> CancelTaskInstanceNames;

	/** Internal function, cancels all the tasks we asked to cancel last frame (by instance name). */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void EndTaskByInstanceName(FName InstanceName);
	TArray<FName> EndTaskInstanceNames;

	/** Destroys instanced-per-execution abilities. Instance-per-actor abilities should 'reset'. Non instance abilities - what can we do? */
	UFUNCTION(BlueprintCallable, Category = Ability)
	void CancelTaskByInstanceName(FName InstanceName);

	// -------------------------------------
	//	EndAbility
	// -------------------------------------

	UFUNCTION(BlueprintCallable, Category = Ability, FriendlyName="EndAbility")
	virtual void K2_EndAbility();

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	// -------------------------------------
	//	GameplayCue
	//	Abilities can invoke GameplayCues without having to create GameplayEffects
	// -------------------------------------
	
	UFUNCTION(BlueprintCallable, Category = Ability, meta=(GameplayTagFilter="GameplayCue"))
	virtual void ExecuteGameplayCue(FGameplayTag GameplayCueTag);

	UFUNCTION(BlueprintCallable, Category = Ability, meta=(GameplayTagFilter="GameplayCue"))
	virtual void AddGameplayCue(FGameplayTag GameplayCueTag);

	UFUNCTION(BlueprintCallable, Category = Ability, meta=(GameplayTagFilter="GameplayCue"))
	virtual void RemoveGameplayCue(FGameplayTag GameplayCueTag);


	// -------------------------------------


	bool IsInstantiated() const
	{
		return !HasAllFlags(RF_ClassDefaultObject);
	}

	void SetCurrentActorInfo(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const
	{
		if (IsInstantiated())
		{
			CurrentActorInfo = ActorInfo;
			CurrentSpecHandle = Handle;
		}
	}

	void SetCurrentInfo(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo)
	{
		if (IsInstantiated())
		{
			CurrentActivationInfo = ActivationInfo;
			CurrentActorInfo = ActorInfo;
			CurrentSpecHandle = Handle;
		}
	}

	UFUNCTION(BlueprintCallable, Category=Ability)
	FGameplayAbilityActorInfo GetActorInfo() const;

	UFUNCTION(BlueprintCallable, Category = Ability)
	USkeletalMeshComponent* GetOwningComponentFromActorInfo() const;

	UFUNCTION(BlueprintCallable, Category=Ability)
	FGameplayEffectSpecHandle GetOutgoingSpec(UGameplayEffect* GameplayEffect) const;	

	bool IsSupportedForNetworking() const override;

	/** Checks cooldown. returns true if we can be used again. False if not */
	bool	CheckCooldown(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** Applies CooldownGameplayEffect to the target */
	void	ApplyCooldown(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	bool	CheckCost(const FGameplayAbilityActorInfo* ActorInfo) const;

	void	ApplyCost(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	// -----------------------------------------------	

	UPROPERTY(EditDefaultsOnly, Category = Advanced)
	TEnumAsByte<EGameplayAbilityReplicationPolicy::Type> ReplicationPolicy;

	UPROPERTY(EditDefaultsOnly, Category = Advanced)
	TEnumAsByte<EGameplayAbilityInstancingPolicy::Type>	InstancingPolicy;						

	/** This is information specific to this instance of the ability. E.g, whether it is predicting, authoring, confirmed, etc. */
	UPROPERTY(BlueprintReadOnly, Category = Ability)
	FGameplayAbilityActivationInfo	CurrentActivationInfo;

	UPROPERTY(EditDefaultsOnly, Category=Advanced)
	TEnumAsByte<EGameplayAbilityNetExecutionPolicy::Type> NetExecutionPolicy;

	/** This GameplayEffect represents the cooldown. It will be applied when the ability is committed and the ability cannot be used again until it is expired. */
	UPROPERTY(EditDefaultsOnly, Category=Cooldowns)
	class UGameplayEffect* CooldownGameplayEffect;

	/** This GameplayEffect represents the cost (mana, stamina, etc) of the ability. It will be applied when the ability is committed. */
	UPROPERTY(EditDefaultsOnly, Category=Costs)
	class UGameplayEffect* CostGameplayEffect;

	/** Triggers to determine if this ability should execute in response to an event */
	UPROPERTY(EditDefaultsOnly, Category = Triggers)
	TArray<FAbilityTriggerData> AbilityTriggers;
	
	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Ability exclusion / canceling
	//
	// ----------------------------------------------------------------------------------------------------------------
	
	/** Abilities with these tags are canceled when this ability is executed */
	UPROPERTY(EditDefaultsOnly, Category = Tags)
	FGameplayTagContainer CancelAbilitiesWithTag;

	/** Abilities with these tags are blocked while this ability is active */
	UPROPERTY(EditDefaultsOnly, Category = Tags)
	FGameplayTagContainer BlockAbilitiesWithTag;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Ability Tasks
	//
	// ----------------------------------------------------------------------------------------------------------------
	
	TArray<TWeakObjectPtr<UAbilityTask> >	ActiveTasks;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Animation callbacks (still WIP)
	//
	// ----------------------------------------------------------------------------------------------------------------

	void MontageBranchPoint_AbilityDecisionStop(const FGameplayAbilityActorInfo* ActorInfo) const;

	void MontageBranchPoint_AbilityDecisionStart(const FGameplayAbilityActorInfo* ActorInfo) const;

	UFUNCTION(BlueprintPure, Category = Ability, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	FGameplayAbilityTargetingLocationInfo MakeTargetLocationInfoFromOwnerActor();

	UFUNCTION(BlueprintPure, Category = Ability, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	FGameplayAbilityTargetingLocationInfo MakeTargetLocationInfoFromOwnerSkeletalMeshComponent(FName SocketName);

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	
	//
	// 
	// ----------------------------------------------------------------------------------------------------------------
	
	/** Returns current level of the Ability */
	UFUNCTION(BlueprintCallable, Category = Ability, meta = (HidePin = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	int32 GetAbilityLevel() const;

	/** Returns current ability level for non instanced abilities. You m ust call this version in these contexts! */
	int32 GetAbilityLevel(FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo) const;

protected:

	/** 
	 *  This is shared, cached information about the thing using us
	 *	 E.g, Actor*, MovementComponent*, AnimInstance, etc.
	 *	 This is hopefully allocated once per actor and shared by many abilities.
	 *	 The actual struct may be overridden per game to include game specific data.
	 *	 (E.g, child classes may want to cast to FMyGameAbilityActorInfo)
	 */
	mutable const FGameplayAbilityActorInfo* CurrentActorInfo;

	mutable FGameplayAbilitySpecHandle CurrentSpecHandle;

	/** True if the ability is currently active. For instance per owner abilities */
	bool bIsActive;
};
