// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagAssetInterface.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.generated.h"

#define SKILL_SYSTEM_AGGREGATOR_DEBUG 1

#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	#define SKILL_AGG_DEBUG( Format, ... ) *FString::Printf(Format, ##__VA_ARGS__)
#else
	#define SKILL_AGG_DEBUG( Format, ... ) NULL
#endif

class UAbilitySystemComponent;

struct FGameplayEffectSpec;
struct FGameplayEffectModCallbackData;

UENUM(BlueprintType)
namespace EGameplayModOp
{
	enum Type
	{		
		// Numeric
		Additive = 0		UMETA(DisplayName="Add"),
		Multiplicitive		UMETA(DisplayName="Multiply"),
		Division			UMETA(DisplayName="Divide"),

		// Other
		Override 			UMETA(DisplayName="Override"),	// This should always be the first non numeric ModOp
		Callback			UMETA(DisplayName="Custom"),

		// This must always be at the end
		Max					UMETA(DisplayName="Invalid")
	};
}

/**
 * Tells us what thing a GameplayModifier modifies.
 */
UENUM(BlueprintType)
namespace EGameplayMod
{
	enum Type
	{
		Attribute = 0,		// Modifies this Attributes
		OutgoingGE,			// Modifies Outgoing Gameplay Effects (that modify this Attribute)
		IncomingGE,			// Modifies Incoming Gameplay Effects (that modify this Attribute)
		ActiveGE,			// Modifies currently active Gameplay Effects

		// This must always be at the end
		Max					UMETA(DisplayName="Invalid")
	};
}

/**
 * Tells us what thing a GameplayEffect provides immunity to. This must mirror the values in EGameplayMod
 */
UENUM(BlueprintType)
namespace EGameplayImmunity
{
	enum Type
	{
		None = 0,			// Does not provide immunity
		OutgoingGE,			// Provides immunity to outgoing GEs
		IncomingGE,			// Provides immunity to incoming GEs
		ActiveGE,			// Provides immunity from currently active GEs

		// This must always be at the end
		Max					UMETA(DisplayName="Invalid")
	};
}

static_assert((int32)EGameplayMod::OutgoingGE == (int32)EGameplayImmunity::OutgoingGE, "EGameplayMod::OutgoingGE and EGameplayImmunity::OutgoingGE must match. Did you forget to modify one of them?");
static_assert((int32)EGameplayMod::IncomingGE == (int32)EGameplayImmunity::IncomingGE, "EGameplayMod::IncomingGE and EGameplayImmunity::IncomingGE must match. Did you forget to modify one of them?");
static_assert((int32)EGameplayMod::ActiveGE == (int32)EGameplayImmunity::ActiveGE, "EGameplayMod::ActiveGE and EGameplayImmunity::ActiveGE must match. Did you forget to modify one of them?");
static_assert((int32)EGameplayMod::Max == (int32)EGameplayImmunity::Max, "EGameplayMod and EGameplayImmunity must cover the same cases. Did you forget to modify one of them?");

/**
 * Tells us what a GameplayEffect modifies when being applied to another GameplayEffect
 */
UENUM(BlueprintType)
namespace EGameplayModEffect
{
	enum Type
	{
		Magnitude			= 0x01,		// Modifies magnitude of a GameplayEffect (Always default for Attribute mod)
		Duration			= 0x02,		// Modifies duration of a GameplayEffect
		ChanceApplyTarget	= 0x04,		// Modifies chance to apply GameplayEffect to target
		ChanceExecuteEffect	= 0x08,		// Modifies chance to execute GameplayEffect on GameplayEffect
		LinkedGameplayEffect= 0x10,		// Adds a linked GameplayEffect to a GameplayEffect

		// This must always be at the end
		All					= 0xFF		UMETA(DisplayName="Invalid")
	};
}

/**
 * Tells us how to handle copying gameplay effect when it is applied.
 *	Default means to use context - e.g, OutgoingGE is are always snapshots, IncomingGE is always Link
 *	AlwaysSnapshot vs AlwaysLink let mods themselves override
 */
UENUM(BlueprintType)
namespace EGameplayEffectCopyPolicy
{
	enum Type
	{
		Default = 0			UMETA(DisplayName="Default"),
		AlwaysSnapshot		UMETA(DisplayName="AlwaysSnapshot"),
		AlwaysLink			UMETA(DisplayName="AlwaysLink")
	};
}

UENUM(BlueprintType)
namespace EGameplayEffectStackingPolicy
{
	enum Type
	{
		Unlimited = 0		UMETA(DisplayName = "NoRule"),
		Highest				UMETA(DisplayName = "Strongest"),
		Lowest				UMETA(DisplayName = "Weakest"),
		Replaces			UMETA(DisplayName = "MostRecent"),
		Callback			UMETA(DisplayName = "Custom"),

		// This must always be at the end
		Max					UMETA(DisplayName = "Invalid")
	};
}


/**
 * This handle is required for things outside of FActiveGameplayEffectsContainer to refer to a specific active GameplayEffect
 *	For example if a skill needs to create an active effect and then destroy that specific effect that it created, it has to do so
 *	through a handle. a pointer or index into the active list is not sufficient.
 */
USTRUCT(BlueprintType)
struct FActiveGameplayEffectHandle
{
	GENERATED_USTRUCT_BODY()

	FActiveGameplayEffectHandle()
		: Handle(INDEX_NONE)
	{

	}

	FActiveGameplayEffectHandle(int32 InHandle)
		: Handle(InHandle)
	{

	}

	bool IsValid() const
	{
		return Handle != INDEX_NONE;
	}

	static FActiveGameplayEffectHandle GenerateNewHandle(UAbilitySystemComponent* OwningComponent);

	UAbilitySystemComponent* GetOwningAbilitySystemComponent();

	bool operator==(const FActiveGameplayEffectHandle& Other) const
	{
		return Handle == Other.Handle;
	}

	bool operator!=(const FActiveGameplayEffectHandle& Other) const
	{
		return Handle != Other.Handle;
	}

	friend uint32 GetTypeHash(const FActiveGameplayEffectHandle& InHandle)
	{
		return InHandle.Handle;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("%d"), Handle);
	}

private:

	UPROPERTY()
	int32 Handle;
};


/**
 * FGameplayEffectInstigatorContext
 *	Data struct for an instigator. This is still being fleshed out. We will want to track actors but also be able to provide some level of tracking for actors that are destroyed.
 *	We may need to store some positional information as well.
 *
 */
USTRUCT(BlueprintType)
struct FGameplayEffectInstigatorContext
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectInstigatorContext()
		: Instigator(NULL)
		, InstigatorAbilitySystemComponent(NULL)
		, HasWorldOrigin(false)
	{
	}

	void GetOwnedGameplayTags(OUT FGameplayTagContainer &TagContainer)
	{
		IGameplayTagAssetInterface* TagInterface = InterfaceCast<IGameplayTagAssetInterface>(Instigator);
		if (TagInterface)
		{
			TagInterface->GetOwnedGameplayTags(TagContainer);
		}
	}

	void AddInstigator(class AActor *InInstigator);

	void AddHitResult(const FHitResult InHitResult);

	void AddOrigin(FVector InOrigin);

	FString ToString() const
	{
		return Instigator ? Instigator->GetName() : FString(TEXT("NONE"));
	}

	/** Should always return the original instigator that started the whole chain */
	AActor* GetOriginalInstigator()
	{
		return Instigator;
	}

	UAbilitySystemComponent* GetOriginalInstigatorAbilitySystemComponent() const
	{
		return InstigatorAbilitySystemComponent;
	}

	bool IsLocallyControlled() const;
	
	/** Instigator controller */
	UPROPERTY()
	AActor* Instigator;

	UPROPERTY(NotReplicated)
	UAbilitySystemComponent* InstigatorAbilitySystemComponent;

	/** Trace information - may be NULL in many cases */
	TSharedPtr<FHitResult>	HitResult;

	UPROPERTY()
	FVector	WorldOrigin;

	bool HasWorldOrigin;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits< FGameplayEffectInstigatorContext > : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		// Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};

// -----------------------------------------------------------

USTRUCT(BlueprintType)
struct FGameplayCueParameters
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	float NormalizedMagnitude;

	UPROPERTY()
	FGameplayEffectInstigatorContext InstigatorContext;

	UPROPERTY()
	FName MatchedTagName;
};

UENUM(BlueprintType)
namespace EGameplayCueEvent
{
	enum Type
	{
		OnActive,		// Called when GameplayCue is activated.
		WhileActive,	// Called when GameplayCue is active, even if it wasn't actually just applied (Join in progress, etc)
		Executed,		// Called when a GameplayCue is executed: instant effects or periodic tick
		Removed			// Called when GameplayCue is removed
	};
}

FString EGameplayModOpToString(int32 Type);

FString EGameplayModToString(int32 Type);

FString EGameplayModEffectToString(int32 Type);

FString EGameplayEffectCopyPolicyToString(int32 Type);

FString EGameplayEffectStackingPolicyToString(int32 Type);


DECLARE_DELEGATE_OneParam(FOnGameplayAttributeEffectExecuted, struct FGameplayModifierEvaluatedData&);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayEffectTagCountChanged, const FGameplayTag, int32 );

DECLARE_MULTICAST_DELEGATE(FOnActiveGameplayEffectRemoved);

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameplayAttributeChange, float ,const FGameplayEffectModCallbackData*);

// -----------------------------------------------------------

/** 
 *	Structure that contains a counted set of GameplayTags. Can optionally include parent tags
 *	
 */
struct FGameplayTagCountContainer
{
	FGameplayTagCountContainer()
	: TagContainerType(EGameplayTagMatchType::Explicit)
	{ }

	FGameplayTagCountContainer(EGameplayTagMatchType::Type InTagContainerType)
	: TagContainerType(InTagContainerType)
	{ }

	bool HasMatchingGameplayTag(FGameplayTag TagToCheck, EGameplayTagMatchType::Type TagMatchType) const;
	bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer, EGameplayTagMatchType::Type TagMatchType, bool bCountEmptyAsMatch = true) const;
	bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer, EGameplayTagMatchType::Type TagMatchType, bool bCountEmptyAsMatch = true) const;
	void UpdateTagMap(const struct FGameplayTagContainer& Container, int32 CountDelta);
	void UpdateTagMap(const struct FGameplayTag& Tag, int32 CountDelta);

	TMap<struct FGameplayTag, FOnGameplayEffectTagCountChanged> GameplayTagEventMap;
	TMap<struct FGameplayTag, int32> GameplayTagCountMap;

	EGameplayTagMatchType::Type TagContainerType;
};
