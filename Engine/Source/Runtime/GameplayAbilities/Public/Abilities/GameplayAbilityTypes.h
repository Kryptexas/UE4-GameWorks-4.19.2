// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayAbilityTypes.generated.h"

class UGameplayEffect;
class UAnimInstance;
class UAbilitySystemComponent;
class UGameplayAbility;
class AGameplayAbilityTargetActor;
class UAbilityTask;
class UAttributeSet;


UENUM(BlueprintType)
namespace EGameplayAbilityInstancingPolicy
{
	/**
	 *	How the ability is instanced when executed. This limits what an ability can do in its implementation. For example, a NonInstanced
	 *	Ability cannot have state. It is probably unsafe for an InstancedPerActor ability to have latent actions, etc.
	 */

	enum Type
	{
		NonInstanced,			// This ability is never instanced. Anything that executes the ability is operating on the CDO.
		InstancedPerActor,		// Each actor gets their own instance of this ability. State can be saved, replication is possible.
		InstancedPerExecution,	// We instance this ability each time it is executed. Replication possible but not recommended.
	};
}

UENUM(BlueprintType)
namespace EGameplayAbilityNetExecutionPolicy
{
	/**
	*	How does an ability execute on the network. Does a client "ask and predict", "ask and wait", "don't ask"
	*/

	enum Type
	{
		Predictive		UMETA(DisplayName = "Predictive"),	// Part of this ability runs predictively on the client.
		Server			UMETA(DisplayName = "Server"),		// This ability must be OK'd by the server before doing anything on a client.
		Client			UMETA(DisplayName = "Client"),		// This ability runs as long the client says it does.
	};
}

UENUM(BlueprintType)
namespace EGameplayAbilityReplicationPolicy
{
	/**
	*	How an ability replicates state/events to everyone on the network.
	*/

	enum Type
	{
		ReplicateNone		UMETA(DisplayName = "Replicate None"),	// We don't replicate the instance of the ability to anyone.
		ReplicateAll		UMETA(DisplayName = "Replicate All"),	// We replicate the instance of the ability to everyone (even simulating clients).
		ReplicateOwner		UMETA(DisplayName = "Replicate Owner"),	// Only replicate the instance of the ability to the owner.
	};
}

UENUM(BlueprintType)
namespace EGameplayAbilityTargetingLocationType
{
	/**
	*	What type of location calculation to use when an ability asks for our transform.
	*/

	enum Type
	{
		LiteralTransform		UMETA(DisplayName = "Literal Transform"),		// We report an actual raw transform. This is also the final fallback if other methods fail.
		ActorTransform			UMETA(DisplayName = "Actor Transform"),			// We pull the transform from an associated actor directly.
		SocketTransform			UMETA(DisplayName = "Socket Transform"),		// We aim from a named socket on the player's skeletal mesh component.
	};
}

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
struct FGameplayAbilityInputIDPair
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityInputIDPair()
	: Ability(nullptr), InputID(-1) { }

	FGameplayAbilityInputIDPair(UGameplayAbility* InAbility, int32 InInputID)
	: Ability(InAbility), InputID(InInputID) { }

	UPROPERTY()
	UGameplayAbility* Ability;

	UPROPERTY()
	int32	InputID;
};


UENUM(BlueprintType)
namespace EGameplayAbilityActivationMode
{
	enum Type
	{
		Authority,			// We are the authority activating this ability
		NonAuthority,		// We are not the authority but aren't predicting yet. This is a mostly invalid state to be in.

		Predicting,			// We are predicting the activation of this ability
		Confirmed,			// We are not the authority, but the authority has confirmed this activation
	};
}

/**
*	FGameplayAbilityActorInfo
*
*	Cached data associated with an Actor using an Ability.
*		-Initialized from an AActor* in InitFromActor
*		-Abilities use this to know what to actor upon. E.g., instead of being coupled to a specific actor class.
*		-These are generally passed around as pointers to support polymorphism.
*		-Projects can override UAbilitySystemGlobals::AllocAbilityActorInfo to override the default struct type that is created.
*
*/
USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityActorInfo
{
	GENERATED_USTRUCT_BODY()

	virtual ~FGameplayAbilityActorInfo() {}

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<AActor>	Actor;

	/** PlayerController associated with this actor. This will often be null! */
	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<APlayerController>	PlayerController;

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<UAnimInstance>	AnimInstance;

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<UAbilitySystemComponent>	AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	TWeakObjectPtr<UMovementComponent>	MovementComponent;

	bool IsLocallyControlled() const;

	bool IsNetAuthority() const;

	virtual void InitFromActor(AActor *Actor, UAbilitySystemComponent* InAbilitySystemComponent);
};

/**
*	Handle for Targeting Data. This servers two main purposes:
*		-Avoid us having to copy around the full targeting data structure in Blueprints
*		-Allows us to leverage polymorphism in the target data structure
*		-Allows us to implement NetSerialize and replicate by value between clients/server
*
*		-Avoid using UObjects could be used to give us polymorphism and by reference passing in blueprints.
*		-However we would still be screwed when it came to replication
*
*		-Replication by value
*		-Pass by reference in blueprints
*		-Polymophism in TargetData structure
*
*/

USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetDataHandle
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityTargetDataHandle() { }
	FGameplayAbilityTargetDataHandle(struct FGameplayAbilityTargetData* DataPtr)
		: Data(DataPtr)
	{

	}

	TSharedPtr<FGameplayAbilityTargetData>	Data;

	void Clear()
	{
		Data.Reset();
	}

	bool IsValid() const
	{
		return Data.IsValid();
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	/** Comparison operator */
	bool operator==(FGameplayAbilityTargetDataHandle const& Other) const
	{
		// Both invalid structs or both valid and Pointer compare (???) // deep comparison equality
		bool bBothValid = IsValid() && Other.IsValid();
		bool bBothInvalid = !IsValid() && !Other.IsValid();
		return (bBothInvalid || (bBothValid && (Data.Get() == Other.Data.Get())));
	}

	/** Comparison operator */
	bool operator!=(FGameplayAbilityTargetDataHandle const& Other) const
	{
		return !(FGameplayAbilityTargetDataHandle::operator==(Other));
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetDataHandle> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithCopy = true,		// Necessary so that TSharedPtr<FGameplayAbilityTargetData> Data is copied around
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
	};
};

/**
*	FGameplayAbilityActorInfo
*
*	Contains data used to determine locations for targeting systems.
*
*/

USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetingLocationInfo
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityTargetingLocationInfo()
	: LocationType(EGameplayAbilityTargetingLocationType::LiteralTransform)
	{
	};

	void operator=(const FGameplayAbilityTargetingLocationInfo& Other)
	{
		LocationType = Other.LocationType;
		LiteralTransform = Other.LiteralTransform;
		SourceActor = Other.SourceActor;
		SourceComponent = Other.SourceComponent;
		SourceSocketName = Other.SourceSocketName;
	}

public:
	//Not sure if we want to distinguish between where we actually aim from (should probably always be the user's camera or actor origin) and where we trace from.
	FTransform GetTargetingTransform() const		//This is split out so we can
	{
		//Return or calculate based on LocationType.
		switch (LocationType)
		{
		case EGameplayAbilityTargetingLocationType::ActorTransform:
			if (SourceActor)
			{
				return SourceActor->GetTransform();
			}
			break;
		case EGameplayAbilityTargetingLocationType::SocketTransform:
			if (SourceComponent)
			{
				return SourceComponent->GetSocketTransform(SourceSocketName);		//Bad socket name will just return component transform anyway, so we're safe
			}
			break;
		case EGameplayAbilityTargetingLocationType::LiteralTransform:
			return LiteralTransform;
		default:
			check(false);		//This case should not happen
			break;
		}
		//Error
		return FTransform::Identity;
	}

	FGameplayAbilityTargetDataHandle MakeTargetDataHandleFromHitResult(TWeakObjectPtr<UGameplayAbility> Ability, FHitResult HitResult) const;

	FGameplayAbilityTargetDataHandle MakeTargetDataHandleFromActors(TArray<AActor*> TargetActors) const;

	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Targeting)
	TEnumAsByte<EGameplayAbilityTargetingLocationType::Type> LocationType;

	/** A literal world transform can be used, if one has been calculated outside of the actor using the ability. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Targeting)
	FTransform LiteralTransform;

	/** A source actor is needed for Actor-based targeting, but not for Socket-based targeting. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Targeting)
	AActor* SourceActor;

	/** Socket-based targeting requires a skeletal mesh component to check for the named socket. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Targeting)
	USkeletalMeshComponent* SourceComponent;

	/** If SourceComponent is valid, this is the name of the socket transform that will be used. If no Socket is provided, SourceComponent's transform will be used. */
	UPROPERTY(BlueprintReadWrite, meta = (ExposeOnSpawn = true), Category = Targeting)
	FName SourceSocketName;

	// -------------------------------------

	virtual FString ToString() const
	{
		return TEXT("FGameplayAbilityTargetingLocationInfo");
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct()
	{
		return FGameplayAbilityTargetingLocationInfo::StaticStruct();
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetingLocationInfo> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};


/**
*	FGameplayAbilityActivationInfo
*
*	Data tied to a specific activation of an ability.
*		-Tell us whether we are the authority, if we are predicting, confirmed, etc.
*		-Holds current and previous PredictionKey
*		-Generally not meant to be subclassed in projects.
*		-Passed around by value since the struct is small.
*/
USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityActivationInfo
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityActivationInfo()
	: ActivationMode(EGameplayAbilityActivationMode::Authority)
	, PrevPredictionKey(0)
	, CurrPredictionKey(0)
	{

	}

	FGameplayAbilityActivationInfo(AActor * InActor, uint32 InPrevPredictionKey, uint32 InCurrPredictionKey)
		: PrevPredictionKey(InPrevPredictionKey)
		, CurrPredictionKey(InCurrPredictionKey)
	{
		// On Init, we are either Authority or NonAuthority. We haven't been given a PredictionKey and we haven't been confirmed.
		// NonAuthority essentially means 'I'm not sure what how I'm going to do this yet'.
		ActivationMode = (InActor->Role == ROLE_Authority ? EGameplayAbilityActivationMode::Authority : EGameplayAbilityActivationMode::NonAuthority);
	}

	FGameplayAbilityActivationInfo(EGameplayAbilityActivationMode::Type InType, uint32 InPrevPredictionKey = 0, uint32 InCurrPredictionKey = 0)
		: ActivationMode(InType)
		, PrevPredictionKey(InPrevPredictionKey)
		, CurrPredictionKey(InCurrPredictionKey)
	{
	}


	void GeneratePredictionKey(UAbilitySystemComponent * Component) const;

	void SetActivationConfirmed();

	UPROPERTY(BlueprintReadOnly, Category = "ActorInfo")
	mutable TEnumAsByte<EGameplayAbilityActivationMode::Type>	ActivationMode;

	UPROPERTY()
	mutable uint32 PrevPredictionKey;

	UPROPERTY()
	mutable uint32 CurrPredictionKey;
};


// ----------------------------------------------------

/**
*	A generic structure for targeting data. We want generic functions to produce this data and other generic
*	functions to consume this data.
*
*	We expect this to be able to hold specific actors/object reference and also generic location/direction/origin
*	information.
*
*	Some example producers:
*		-Overlap/Hit collision event generates TargetData about who was hit in a melee attack
*		-A mouse input causes a hit trace and the actor infront of the crosshair is turned into TargetData
*		-A mouse input causes TargetData to be generated from the owner's crosshair view origin/direction
*		-An AOE/aura pulses and all actors in a radius around the instigator are added to TargetData
*		-Panzer Dragoon style 'painting' targeting mode
*		-MMORPG style ground AOE targeting style (potentially both a location on the ground and actors that were targeted)
*
*	Some example consumers:
*		-Apply a GameplayEffect to all actors in TargetData
*		-Find closest actor from all in TargetData
*		-Call some function on all actors in TargetData
*		-Filter or merge TargetDatas
*		-Spawn a new actor at a TargetData location
*
*
*
*	Maybe it is better to distinguish between actor list targeting vs positional targeting data?
*		-AOE/aura type of targeting data blurs the line
*
*
*/

USTRUCT()
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	virtual ~FGameplayAbilityTargetData() { }

	void ApplyGameplayEffect(UGameplayEffect *GameplayEffect, const FGameplayAbilityActorInfo InstigatorInfo);

	virtual TArray<AActor*>	GetActors() const
	{
		return TArray<AActor*>();
	}

	// -------------------------------------

	virtual bool HasHitResult() const
	{
		return false;
	}

	virtual const FHitResult* GetHitResult() const
	{
		return NULL;
	}

	// -------------------------------------

	virtual bool HasOrigin() const
	{
		return false;
	}

	virtual FTransform GetOrigin() const
	{
		return FTransform::Identity;
	}

	// -------------------------------------

	virtual bool HasEndPoint() const
	{
		return false;
	}

	virtual FVector GetEndPoint() const
	{
		return FVector::ZeroVector;
	}

	// -------------------------------------

	virtual UScriptStruct* GetScriptStruct()
	{
		return FGameplayAbilityTargetData::StaticStruct();
	}

	virtual FString ToString() const;
};

USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetData_LocationInfo : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	/** Generic location data for source */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	FGameplayAbilityTargetingLocationInfo SourceLocation;

	/** Generic location data for target */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	FGameplayAbilityTargetingLocationInfo TargetLocation;

	// -------------------------------------

	virtual bool HasOrigin() const
	{
		return true;
	}

	virtual FTransform GetOrigin() const
	{
		return SourceLocation.GetTargetingTransform();
	}

	// -------------------------------------

	virtual bool HasEndPoint() const
	{
		return true;
	}

	virtual FVector GetEndPoint() const
	{
		return TargetLocation.GetTargetingTransform().GetLocation();
	}

	// -------------------------------------

	virtual UScriptStruct* GetScriptStruct()
	{
		return FGameplayAbilityTargetData_LocationInfo::StaticStruct();
	}

	virtual FString ToString() const override
	{
		return TEXT("FGameplayAbilityTargetData_LocationInfo");
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_LocationInfo> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};

USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetData_ActorArray : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	/** We could be selecting this group of actors from any type of location, so use a generic location type */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	FGameplayAbilityTargetingLocationInfo SourceLocation;

	/** Rather than targeting a single point, this type of targeting selects multiple actors. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	TArray<AActor*> TargetActorArray;

	virtual TArray<AActor*>	GetActors() const override
	{
		return TargetActorArray;
	}

	// -------------------------------------

	virtual bool HasOrigin() const override
	{
		return true;
	}

	virtual FTransform GetOrigin() const override
	{
		return SourceLocation.GetTargetingTransform();		//No single aim-at target, since this is aimed at multiple characters.
	}

	// -------------------------------------

	virtual UScriptStruct* GetScriptStruct() override
	{
		return FGameplayAbilityTargetData_ActorArray::StaticStruct();
	}

	virtual FString ToString() const override
	{
		return TEXT("FGameplayAbilityTargetData_ActorArray");
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_ActorArray> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};

USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetData_Mesh : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityTargetData_Mesh()
	: SourceActor(NULL)
	, SourceComponent(NULL)
	{}


	/** Actor who owns the named component. Actor's location is used as start point if component cannot be found. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	AActor* SourceActor;

	/** Local skeletal mesh component that holds the socket. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	USkeletalMeshComponent* SourceComponent;

	/** If SourceActor and SourceComponent are valid, this is the name of the socket that will be used instead of the actor's location. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Targeting)
	FName SourceSocketName;

	/** Point being targeted. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	FVector_NetQuantize TargetPoint;

	// -------------------------------------

	virtual bool HasOrigin() const override
	{
		return (SourceActor != NULL);
	}

	virtual FTransform GetOrigin() const override
	{
		if (SourceActor)
		{
			FTransform ReturnTransform = SourceActor->GetTransform();
			if (SourceComponent)
			{
				ReturnTransform.SetLocation(SourceComponent->GetSocketLocation(SourceSocketName));
			}
			ReturnTransform.SetRotation((TargetPoint - ReturnTransform.GetLocation()).SafeNormal().Rotation().Quaternion());
			return ReturnTransform;
		}
		return FTransform::Identity;
	}

	// -------------------------------------

	virtual bool HasEndPoint() const override
	{
		return true;
	}

	virtual FVector GetEndPoint() const override
	{
		return TargetPoint;
	}

	// -------------------------------------

	virtual FString ToString() const override
	{
		return TEXT("FGameplayAbilityTargetData_Mesh");
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() override
	{
		return FGameplayAbilityTargetData_Mesh::StaticStruct();
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_Mesh> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};


USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayAbilityTargetData_SingleTargetHit : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	FGameplayAbilityTargetData_SingleTargetHit()
	{ }

	FGameplayAbilityTargetData_SingleTargetHit(const FHitResult InHitResult)
	: HitResult(InHitResult)
	{ }

	// -------------------------------------

	virtual TArray<AActor*>	GetActors() const override
	{
		TArray<AActor*>	Actors;
		if (HitResult.Actor.IsValid())
		{
			Actors.Push(HitResult.Actor.Get());
		}
		return Actors;
	}
	
	// -------------------------------------

	virtual bool HasHitResult() const override
	{
		return true;
	}

	virtual const FHitResult* GetHitResult() const override
	{
		return &HitResult;
	}

	// -------------------------------------

	UPROPERTY()
	FHitResult	HitResult;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess);

	virtual UScriptStruct* GetScriptStruct() override
	{
		return FGameplayAbilityTargetData_SingleTargetHit::StaticStruct();
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_SingleTargetHit> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true	// Fow now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};


USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayEventData
{
	GENERATED_USTRUCT_BODY()

	FGameplayEventData()
	: Instigator(NULL)
	, Target(NULL)
	, PrevPredictionKey(0)
	, CurrPredictionKey(0)
	{
		// do nothing
	}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	AActor* Instigator;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	AActor* Target;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	float Var1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	float Var2;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	float Var3;

	uint32 PrevPredictionKey;
	uint32 CurrPredictionKey;
};

/** 
 *	Structure that tells AbilitySystemComponent what to bind to an InputComponent (see BindAbilityActivationToInputComponent) 
 *	
 */
struct FGameplayAbiliyInputBinds
{
	FGameplayAbiliyInputBinds(FString InConfirmTargetCommand, FString InCancelTargetCommand, FString InEnumName)
	: ConfirmTargetCommand(InConfirmTargetCommand)
	, CancelTargetCommand(InCancelTargetCommand)
	, EnumName(InEnumName)
	{ }

	/** Defines command string that will be bound to Confirm Targeting */
	FString ConfirmTargetCommand;

	/** Defines command string that will be bound to Cancel Targeting */
	FString CancelTargetCommand;

	/** Returns enum to use for ability bings. E.g., "Ability1"-"Ability8" input commands will be bound to ability activations inside the AbiltiySystemComponent */
	FString	EnumName;

	UEnum* GetBindEnum() { return FindObject<UEnum>(ANY_PACKAGE, *EnumName); }
};

/** Generic callback for returning when target data is available */
DECLARE_MULTICAST_DELEGATE_OneParam(FAbilityTargetData, FGameplayAbilityTargetDataHandle);

/** Used for cleaning up predicted data on network clients */
DECLARE_MULTICAST_DELEGATE(FAbilitySystemComponentPredictionKeyClear);

/** Generic delegate for ability 'events'/notifies */
DECLARE_MULTICAST_DELEGATE_OneParam(FGenericAbilityDelegate, UGameplayAbility*);
