// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTagAssetInterface.h"
#include "Runtime/Engine/Classes/Animation/AnimInstance.h"
#include "AbilitySystemTypes.h"
#include "GameplayAbilityTypes.h"
#include "GameplayAbility.generated.h"


/**
* UGameplayAbility
*	The GameplayAbility baseclass. This base class is a data asset and can be used to create non-instanced abilities.
*	Instanced abilities should derive off UGameplyAbility_Instanced.
*/
UCLASS()
class GAMEPLAYABILITIES_API UGameplayAbility : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:

	UFUNCTION(Client, Reliable)
	void ClientActivateAbilitySucceed(int32 PredictionKey);

	virtual void ClientActivateAbilitySucceed_Internal(int32 PredictionKey);

	bool IsSupportedForNetworking() const OVERRIDE;
	
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

	/**
	 * Attempts to activate the ability.
	 *	-This function calls CanActivateAbility
	 *	-This function handles instancing
	 *	-This function handles networking and prediction
	 *	-If all goes well, CallActivateAbility is called next.
	 */
	bool TryActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, int32 PredictionKey = 0, UGameplayAbility ** OutInstancedAbility = NULL);
	
	/** Returns true if this ability can be activated right now. Has no side effects */
	virtual bool CanActivateAbility(const FGameplayAbilityActorInfo* ActorInfo) const;

	// -----------------------------------------------

	/** Input binding. Base implementation calls TryActivateAbility */
	virtual void InputPressed(int32 InputID, const FGameplayAbilityActorInfo* ActorInfo);

	/** Input binding. Base implementation does nothing */
	virtual void InputReleased(int32 InputID, const FGameplayAbilityActorInfo* ActorInfo);

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Outward facing API for other systems
	//
	// ----------------------------------------------------------------------------------------------------------------

	float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo* ActorInfo) const;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Standardized State
	//
	// ----------------------------------------------------------------------------------------------------------------	

	UPROPERTY(EditDefaultsOnly, Category=Advanced)
	TEnumAsByte<EGameplayAbilityNetExecutionPolicy::Type>	NetExecutionPolicy;

	/** This GameplayEffect represents the cooldown. It will be applied when the ability is commited and the ability cannot be used again until it is expired. */
	UPROPERTY(EditDefaultsOnly, Category=Cooldowns)
	class UGameplayEffect * CooldownGameplayEffect;

	/** This GameplayEffect represents the cost (mana, stamina, etc) of the ability. It will be applied when the ability is commited. */
	UPROPERTY(EditDefaultsOnly, Category=Costs)
	class UGameplayEffect * CostGameplayEffect;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Ability exclusion / cancelling
	//
	// ----------------------------------------------------------------------------------------------------------------
	
	/** Abilities with these tags are canelled when this ability is executed */
	UPROPERTY(EditDefaultsOnly, Category = Tags)
	FGameplayTagContainer CancelAbilitiesWithTag;

	/** This ability has these tags */
	UPROPERTY(EditDefaultsOnly, Category = Tags)
	FGameplayTagContainer AbilityTags;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Animation callbacks
	//
	// ----------------------------------------------------------------------------------------------------------------

	void MontageBranchPoint_AbilityDecisionStop(const FGameplayAbilityActorInfo* ActorInfo) const;

	void MontageBranchPoint_AbilityDecisionStart(const FGameplayAbilityActorInfo* ActorInfo) const;

	// ----------------------------------------------------------------------------------------------------------------

	virtual void PredictiveActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	virtual void ServerTryActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	virtual void ClientActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	// ----------------------------------------------------------------------------------------------------------------

	virtual EGameplayAbilityInstancingPolicy::Type GetInstancingPolicy() const
	{
		return EGameplayAbilityInstancingPolicy::NonInstanced;
	}

	virtual EGameplayAbilityReplicationPolicy::Type GetReplicationPolicy() const
	{
		return EGameplayAbilityReplicationPolicy::ReplicateNone;
	}

	// ----------------------------------------------------------------------------------------------------------------

	virtual UWorld* GetWorld() const OVERRIDE
	{
		return GetOuter()->GetWorld();
	}

protected:

	/**
	 * The main function that defines what an ability does.
	 *  -Child classes will want to override this
	 *  -This function must call CommitAbility()
	 */

	virtual void ActivateAbility(const FGameplayAbilityActorInfo * ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** 
	 * Attempts to commit the ability (spend resources, etc). This our last chance to fail.
	 *	-Child classes that override ActivateAbility must call this themselves!
	 */

	virtual bool CommitAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);
	
	/** Do boilerplate init stuff and then call ActivateAbility */
	void PreActivate(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	void CallActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	void CallPredictiveActivateAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	virtual void EndAbility(const FGameplayAbilityActorInfo* ActorInfo);

	/** 
	 * The last chance to fail before commiting
	 *	-This will usually be the same as CanActivateAbility. Some abilities may need to do extra checks here if they are consuming extra stuff in CommitExecute
	 */
	virtual bool CommitCheck(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** Does the commmit atomically (consume resources, do cooldowns, etc) */
	virtual void CommitExecute(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	/** Destroys intanced-per-execution abilities. Instance-per-actor abilities should 'reset'. Non instance abilities - what can we do? */
	virtual void CancelAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) { }

	// --------------------------------------------------------------------------------


	// --------------------------------------------------------------------------------

	/** Checks cooldown. returns true if we can be used again. False if not */
	bool	CheckCooldown(const FGameplayAbilityActorInfo* ActorInfo) const;

	/** Applies CooldownGameplayEffect to the target */
	void	ApplyCooldown(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	bool	CheckCost(const FGameplayAbilityActorInfo* ActorInfo) const;

	void	ApplyCost(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo);

	// --------------------------------------------------------------------------------

	UPROPERTY(EditDefaultsOnly, Category = Tags)
	bool Prediction;

	friend class UAbilitySystemComponent;
};