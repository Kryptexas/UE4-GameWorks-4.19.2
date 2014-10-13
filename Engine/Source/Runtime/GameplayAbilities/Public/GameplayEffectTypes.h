// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagAssetInterface.h"
#include "AttributeSet.h"
#include "GameplayPrediction.h"
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

FString EGameplayModOpToString(int32 Type);

FString EGameplayModToString(int32 Type);

FString EGameplayModEffectToString(int32 Type);

FString EGameplayEffectCopyPolicyToString(int32 Type);

FString EGameplayEffectStackingPolicyToString(int32 Type);

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
 * FGameplayEffectContext
 *	Data struct for an instigator and related data. This is still being fleshed out. We will want to track actors but also be able to provide some level of tracking for actors that are destroyed.
 *	We may need to store some positional information as well.
 */
USTRUCT()
struct GAMEPLAYABILITIES_API FGameplayEffectContext
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectContext()
		: Instigator(NULL)
		, EffectCauser(NULL)
		, InstigatorAbilitySystemComponent(NULL)
		, bHasWorldOrigin(false)
	{
	}

	FGameplayEffectContext(AActor* InInstigator, AActor* InEffectCauser)
		: Instigator(NULL)
		, EffectCauser(NULL)
		, InstigatorAbilitySystemComponent(NULL)
	{
		AddInstigator(InInstigator, InEffectCauser);
	}

	virtual ~FGameplayEffectContext()
	{
	}

	/** Returns the list of gameplay tags applicable to this effect, defaults to the owner's tags */
	virtual void GetOwnedGameplayTags(OUT FGameplayTagContainer &TagContainer) const
	{
		IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Instigator);
		if (TagInterface)
		{
			TagInterface->GetOwnedGameplayTags(TagContainer);
		}
	}

	/** Sets the instigator and effect causer. Instigator is who owns the ability that spawned this, EffectCauser is the actor that is the physical source of the effect, such as a weapon. They can be the same. */
	virtual void AddInstigator(class AActor *InInstigator, class AActor *InEffectCauser);

	/** Returns the immediate instigator that applied this effect */
	virtual AActor* GetInstigator() const
	{
		return Instigator;
	}

	/** Returns the ability system component of the instigator of this effect */
	virtual UAbilitySystemComponent* GetInstigatorAbilitySystemComponent() const
	{
		return InstigatorAbilitySystemComponent;
	}

	/** Returns the physical actor tied to the application of this effect */
	virtual AActor* GetEffectCauser() const
	{
		return EffectCauser;
	}

	/** Should always return the original instigator that started the whole chain. Subclasses can override what this does */
	virtual AActor* GetOriginalInstigator() const
	{
		return Instigator;
	}

	/** Returns the ability system component of the instigator that started the whole chain */
	virtual UAbilitySystemComponent* GetOriginalInstigatorAbilitySystemComponent() const
	{
		return InstigatorAbilitySystemComponent;
	}

	virtual void AddHitResult(const FHitResult InHitResult);

	virtual const FHitResult* GetHitResult() const
	{
		if (HitResult.IsValid())
		{
			return HitResult.Get();
		}
		return NULL;
	}

	virtual void AddOrigin(FVector InOrigin);

	virtual const FVector& GetOrigin() const
	{
		return WorldOrigin;
	}

	virtual bool HasOrigin() const
	{
		return bHasWorldOrigin;
	}

	virtual FString ToString() const
	{
		return Instigator ? Instigator->GetName() : FString(TEXT("NONE"));
	}

	virtual UScriptStruct* GetScriptStruct() const
	{
		return FGameplayEffectContext::StaticStruct();
	}

	/** Creates a copy of this context, used to duplicate for later modifications */
	virtual FGameplayEffectContext* Duplicate() const
	{
		FGameplayEffectContext* NewContext = new FGameplayEffectContext();
		*NewContext = *this;
		if (GetHitResult())
		{
			// Does a deep copy of the hit result
			NewContext->AddHitResult(*GetHitResult());
		}
		return NewContext;
	}

	virtual bool IsLocallyControlled() const;

	virtual bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

protected:

	/** Instigator actor, the actor that owns the ability system component */
	UPROPERTY()
	AActor* Instigator;

	/** The physical actor that actually did the damage, can be a weapon or projectile */
	UPROPERTY()
	AActor* EffectCauser;

	/** The ability system component that's bound to instigator */
	UPROPERTY(NotReplicated)
	UAbilitySystemComponent* InstigatorAbilitySystemComponent;

	/** Trace information - may be NULL in many cases */
	TSharedPtr<FHitResult>	HitResult;

	UPROPERTY()
	FVector	WorldOrigin;

	UPROPERTY()
	bool bHasWorldOrigin;
};

template<>
struct TStructOpsTypeTraits< FGameplayEffectContext > : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true,
		WithCopy = true		// Necessary so that TSharedPtr<FHitResult> Data is copied around
	};
};

/**
 * Handle that wraps a FGameplayEffectContext or subclass, to allow it to be polymorphic and replicate properly
 */
USTRUCT(BlueprintType)
struct FGameplayEffectContextHandle
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectContextHandle()
	{
	}

	/** Constructs from an existing context, should be allocated by new */
	FGameplayEffectContextHandle(FGameplayEffectContext* DataPtr)
	{
		Data = TSharedPtr<FGameplayEffectContext>(DataPtr);
	}

	/** Sets from an existing context, should be allocated by new */
	void operator=(FGameplayEffectContext* DataPtr)
	{
		Data = TSharedPtr<FGameplayEffectContext>(DataPtr);
	}

	void Clear()
	{
		Data.Reset();
	}

	bool IsValid() const
	{
		return Data.IsValid();
	}

	FGameplayEffectContext* Get()
	{
		return IsValid() ? Data.Get() : NULL;
	}

	/** Returns the list of gameplay tags applicable to this effect, defaults to the owner's tags */
	void GetOwnedGameplayTags(OUT FGameplayTagContainer &TagContainer) const
	{
		if (IsValid())
		{
			Data->GetOwnedGameplayTags(TagContainer);
		}
	}

	/** Sets the instigator and effect causer. Instigator is who owns the ability that spawned this, EffectCauser is the actor that is the physical source of the effect, such as a weapon. They can be the same. */
	void AddInstigator(class AActor *InInstigator, class AActor *InEffectCauser)
	{
		if (IsValid())
		{
			Data->AddInstigator(InInstigator, InEffectCauser);
		}
	}

	/** Returns the immediate instigator that applied this effect */
	virtual AActor* GetInstigator() const
	{
		if (IsValid())
		{
			return Data->GetInstigator();
		}
		return NULL;
	}

	/** Returns the ability system component of the instigator of this effect */
	virtual UAbilitySystemComponent* GetInstigatorAbilitySystemComponent() const
	{
		if (IsValid())
		{
			return Data->GetInstigatorAbilitySystemComponent();
		}
		return NULL;
	}

	/** Returns the physical actor tied to the application of this effect */
	virtual AActor* GetEffectCauser() const
	{
		if (IsValid())
		{
			return Data->GetEffectCauser();
		}
		return NULL;
	}

	/** Should always return the original instigator that started the whole chain. Subclasses can override what this does */
	AActor* GetOriginalInstigator() const
	{
		if (IsValid())
		{
			return Data->GetOriginalInstigator();
		}
		return NULL;
	}

	/** Returns the ability system component of the instigator that started the whole chain */
	UAbilitySystemComponent* GetOriginalInstigatorAbilitySystemComponent() const
	{
		if (IsValid())
		{
			return Data->GetOriginalInstigatorAbilitySystemComponent();
		}
		return NULL;
	}

	/** Returns if the instigator is locally controlled */
	bool IsLocallyControlled() const
	{
		if (IsValid())
		{
			return Data->IsLocallyControlled();
		}
		return false;
	}

	void AddHitResult(const FHitResult InHitResult)
	{
		if (IsValid())
		{
			Data->AddHitResult(InHitResult);
		}
	}

	const FHitResult* GetHitResult() const
	{
		if (IsValid())
		{
			return Data->GetHitResult();
		}
		return NULL;
	}

	void AddOrigin(FVector InOrigin)
	{
		if (IsValid())
		{
			Data->AddOrigin(InOrigin);
		}
	}

	virtual const FVector& GetOrigin() const
	{
		if (IsValid())
		{
			return Data->GetOrigin();
		}
		return FVector::ZeroVector;
	}

	virtual bool HasOrigin() const
	{
		if (IsValid())
		{
			return Data->HasOrigin();
		}
		return false;
	}

	FString ToString() const
	{
		return IsValid() ? Data->ToString() : FString(TEXT("NONE"));
	}

	/** Creates a deep copy of this handle, used before modifying */
	FGameplayEffectContextHandle Duplicate() const
	{
		if (IsValid())
		{
			FGameplayEffectContext* NewContext = Data->Duplicate();
			return FGameplayEffectContextHandle(NewContext);
		}
		else
		{
			return FGameplayEffectContextHandle();
		}
	}

	/** Comparison operator */
	bool operator==(FGameplayEffectContextHandle const& Other) const
	{
		if (Data.IsValid() != Other.Data.IsValid())
		{
			return false;
		}
		if (Data.Get() != Other.Data.Get())
		{
			return false;
		}
		return true;
	}

	/** Comparison operator */
	bool operator!=(FGameplayEffectContextHandle const& Other) const
	{
		return !(FGameplayEffectContextHandle::operator==(Other));
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

private:

	TSharedPtr<FGameplayEffectContext> Data;
};

template<>
struct TStructOpsTypeTraits<FGameplayEffectContextHandle> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithCopy = true,		// Necessary so that TSharedPtr<FGameplayEffectContext> Data is copied around
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
	};
};


// -----------------------------------------------------------

/** 
 * Qualification context for applying modifiers.
 *	For example a Modifier may be setup in data to only apply to OutgoingGE mods.
 *	The FModifierQualifier is the data structure to hold the 'what type of modifier are we applying' data.
 *
 *  This should ideally only hold data that is outside of FGameplayEffectSpec or FGameplayModifierSpec.
 *	For example, specs know what they can and can't modify. We don't need to duplicate that there. 
 *  FModifierQualifier is meant to hold the data that comes from the calling context that is not intrinsic 
 *  to the existing data structures.
 *
 *	This class uses an optional initialization idiom such that you can do things like:
 *		FModifierQualifier().Handle(InHandle).(InType).etc...
 */
USTRUCT(BlueprintType)
struct FModifierQualifier
{
	GENERATED_USTRUCT_BODY()

	FModifierQualifier()
		: MyType(EGameplayMod::Max)
	{
	}

	// ----------------------------------

	FModifierQualifier& Type(EGameplayMod::Type InType)
	{
		MyType = InType;
		return *this;
	}

	EGameplayMod::Type Type() const
	{
		return MyType;
	}

	// ----------------------------------
	// PredictionKey is used by networking/replication to specify that a GameplayEffect has been predictively created/added.
	
	FModifierQualifier& PredictionKey(FPredictionKey InPredictionKey)
	{
		MyPredictionKey = InPredictionKey;
		return *this;
	}

	FPredictionKey PredictionKey() const
	{
		return MyPredictionKey;
	}

	// ----------------------------------
	// IgnoreHandle - ignore this handle completely. For example when executing an active gameplay effect, we never
	// want to apply it to itself (either as IncomingGE or activeGE). It is ignored in all contexts.
	//

	FModifierQualifier& IgnoreHandle(FActiveGameplayEffectHandle InHandle)
	{
		MyIgnoreHandle = InHandle;
		return *this;
	}

	FActiveGameplayEffectHandle IgnoreHandle() const
	{
		return MyIgnoreHandle;
	}

	// ----------------------------------
	// ExclusiveTarget - sometimes we need to only apply a modifier to a specific active gameplay effect.
	// We may only be able to use the handle if there are multiple instances of the same gameplay effect.
	// This only applies in the context of applying/executing to a target. 'We only modify this active effect'.
	// ExclusiveTarget is not checked in the context of applying outgoing/incoming GE modifiers to the spec.
	//

	FModifierQualifier& ExclusiveTarget(FActiveGameplayEffectHandle InHandle)
	{
		MyExclusiveTargetHandle = InHandle;
		return *this;
	}

	FActiveGameplayEffectHandle ExclusiveTarget() const
	{
		return MyExclusiveTargetHandle;
	}

	// ----------------------------------

	bool TestTarget(FActiveGameplayEffectHandle InHandle) const
	{
		if (MyIgnoreHandle.IsValid() && MyIgnoreHandle == InHandle)
		{
			return false;
		}

		if (MyExclusiveTargetHandle.IsValid() && MyExclusiveTargetHandle != InHandle)
		{
			return false;
		}

		return true;
	}

	FString ToString() const
	{
		return EGameplayModToString(MyType);
	}

private:
	EGameplayMod::Type	MyType;
	FActiveGameplayEffectHandle MyIgnoreHandle;		// Do not modify this handle
	FActiveGameplayEffectHandle MyExclusiveTargetHandle;	// Only modify this handle
	FPredictionKey MyPredictionKey;
};

// -----------------------------------------------------------


USTRUCT(BlueprintType)
struct FGameplayCueParameters
{
	GENERATED_USTRUCT_BODY()

	/** Magnitude of source gameplay effect, normalzed from 0-1. Use this for "how strong is the gameplay effet" (0=min, 1=,max) */
	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	float NormalizedMagnitude;

	/** Raw final magnitude of source gameplay effect. Use this is you need to display numbers or for other informational purposes. */
	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	float RawMagnitude;

	UPROPERTY()
	FGameplayEffectContextHandle EffectContext;

	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	FName MatchedTagName;

	UPROPERTY(BlueprintReadWrite, Category=GameplayCue)
	FGameplayTag OriginalTag;
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
