// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Engine/Classes/Animation/AnimInstance.h"
#include "GameplayAbility.generated.h"

class UAnimInstance;
class UAttributeComponent;

UENUM(BlueprintType)
namespace EGameplayAbilityReplication
{
	enum Type
	{
		// Numeric
		Predictive		UMETA(DisplayName = "Predictive"),	// Part of this ability runs predictively on the client.
		Server			UMETA(DisplayName = "Server"),		// This ability must be OK'd by the server before doing anything on a client.
		Client			UMETA(DisplayName = "Client"),		// This ability runs as long the client says it does.
	};
}

USTRUCT()
struct SKILLSYSTEM_API FGameplayAbilityActorInfo
{
	GENERATED_USTRUCT_BODY()

	TWeakObjectPtr<AActor>	Actor;

	TWeakObjectPtr<UAnimInstance>	AnimInstance;

	TWeakObjectPtr<UAttributeComponent>	AttributeComponent;

	void InitFromActor(AActor *Actor);
};

USTRUCT(BlueprintType)
struct FGameplayAbilityHandle
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityHandle()
	: Handle(INDEX_NONE)
	{

	}

	FGameplayAbilityHandle(int32 InHandle)
		: Handle(InHandle)
	{

	}

	bool IsValid() const
	{
		return Handle != INDEX_NONE;
	}

	FGameplayAbilityHandle GetNextHandle()
	{
		return FGameplayAbilityHandle(Handle + 1);
	}

	bool operator==(const FGameplayAbilityHandle& Other) const
	{
		return Handle == Other.Handle;
	}

	bool operator!=(const FGameplayAbilityHandle& Other) const
	{
		return Handle != Other.Handle;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("%d"), Handle);
	}

private:

	UPROPERTY()
	int32 Handle;
};

USTRUCT()
struct FGameplayAbilitySpec
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	int32	Level;

	UPROPERTY()
	class UGameplayAbility * Ability;

	UPROPERTY()
	FGameplayAbilityHandle	Handle;
};

/**
* UGameplayEffect
*	The GameplayEffect definition. This is the data asset defined in the editor that drives everything.
*/
UCLASS()
class SKILLSYSTEM_API UGameplayAbility : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	
	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Primary events that are implemented to define an ability
	//	
	// ----------------------------------------------------------------------------------------------------------------
	
	/** Returns true if this ability can be activated right now. Has no side effects! */
	virtual bool CanActivateAbility(const FGameplayAbilityActorInfo ActorInfo) const;
	
	/** Attempts to commit the ability (spend resources, etc). This our last chance to fail. */
	virtual bool CommitAbility(const FGameplayAbilityActorInfo ActorInfo);

	/** The last chance to fail before commiting */
	virtual bool CommitCheck(const FGameplayAbilityActorInfo ActorInfo);
	
	/** Does the commmit atomically (consume resources, do cooldowns, etc) */
	virtual void CommitExecute(const FGameplayAbilityActorInfo ActorInfo);

	// -----------------------------------------------

	/** Calls ActivateAbility if we CanACtivateAbility */
	virtual bool TryActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	/** (Optionally Does Stuff) -> Commit Ability -> (Optionally Does Stuff) */
	virtual void ActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	/** Input binding. Base implementation calls TryActivateAbility */
	virtual void InputPressed(int32 InputID, const FGameplayAbilityActorInfo ActorInfo);

	/** Input binding. Base implementation does nothing */
	virtual void InputReleased(int32 InputID, const FGameplayAbilityActorInfo ActorInfo);

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Outward facing API for other systems
	//
	// ----------------------------------------------------------------------------------------------------------------

	float GetCooldownTimeRemaining(const FGameplayAbilityActorInfo ActorInfo) const;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Standardized State
	//
	// ----------------------------------------------------------------------------------------------------------------	

	UPROPERTY(EditDefaultsOnly, Category=Replication)
	TEnumAsByte<EGameplayAbilityReplication::Type>	ReplicationType;

	/** This GameplayEffect represents the cooldown. It will be applied when the ability is commited and the ability cannot be used again until it is expired. */
	UPROPERTY(EditDefaultsOnly, Category=Cooldowns)
	class UGameplayEffect * CooldownGameplayEffect;

	/** This GameplayEffect represents the cost (mana, stamina, etc) of the ability. It will be applied when the ability is commited. */
	UPROPERTY(EditDefaultsOnly, Category=Costs)
	class UGameplayEffect * CostGameplayEffect;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	Animation callbacks
	//
	// ----------------------------------------------------------------------------------------------------------------

	void MontageBranchPoint_AbilityDecisionStop(const FGameplayAbilityActorInfo ActorInfo) const;

	void MontageBranchPoint_AbilityDecisionStart(const FGameplayAbilityActorInfo ActorInfo) const;

	// ----------------------------------------------------------------------------------------------------------------

	virtual void PredictiveActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	virtual void ServerTryActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	virtual void ClientActivateAbility(const FGameplayAbilityActorInfo ActorInfo);

	// ----------------------------------------------------------------------------------------------------------------

	virtual bool AllowInstancing() const
	{
		return false;
	}

	virtual UWorld* GetWorld() const OVERRIDE
	{
		return GetOuter()->GetWorld();
	}

private:

	/** Checks cooldown. returns true if we can be used again. False if not */
	bool	CheckCooldown(const FGameplayAbilityActorInfo ActorInfo) const;

	/** Applies CooldownGameplayEffect to the target */
	void	ApplyCooldown(const FGameplayAbilityActorInfo ActorInfo);

	bool	CheckCost(const FGameplayAbilityActorInfo ActorInfo) const;

	void	ApplyCost(const FGameplayAbilityActorInfo ActorInfo);
};