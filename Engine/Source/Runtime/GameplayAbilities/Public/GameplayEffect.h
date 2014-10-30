// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagAssetInterface.h"
#include "AbilitySystemLog.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectAggregator.h"
#include "GameplayEffectCalculation.h"
#include "GameplayEffect.generated.h"

struct FActiveGameplayEffect;

class UGameplayEffect;
class UGameplayEffectTemplate;
class UAbilitySystemComponent;
class UGameplayModCalculation;

/** Enumeration for options of where to capture gameplay attributes from for gameplay effects */
UENUM()
enum class EGameplayEffectAttributeCaptureSource : uint8
{
	Source,	// Source (caster) of the gameplay effect
	Target	// Target (recipient) of the gameplay effect
};

// Repurposing(?) this to be the inplace/custom/scoped modifier used by FGameplayEffectExecutionDefinition
//	I would expect this to be the thing that is built off to be a "scalable float/attribute reference/custom magnitude" thing.
//	This would potentially need to be snapshot/not
// 
USTRUCT()
struct GAMEPLAYABILITIES_API FExtensionAttributeModifierInfo
{
	GENERATED_USTRUCT_BODY()

	FExtensionAttributeModifierInfo()
		: ModifierOp( EGameplayModOp::Additive )
	{

	}

	/** The Attribute we modify or the GE we modify modifies. */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	FGameplayAttribute Attribute;

	/** The numeric operation of this modifier: Override, Add, Multiply, etc  */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	TEnumAsByte<EGameplayModOp::Type> ModifierOp;

	/** How much this modifies what it is applied to */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	FScalableFloat Magnitude;

	/** Source of the gameplay attribute */
	UPROPERTY(EditDefaultsOnly, Category = Capture)
	EGameplayEffectAttributeCaptureSource Source;

	UPROPERTY(EditDefaultsOnly, Category = GameplayModifier)
	FGameplayTagRequirements	SourceTags;

	UPROPERTY(EditDefaultsOnly, Category = GameplayModifier)
	FGameplayTagRequirements	TargetTags;
};


// Should this just go away now?
USTRUCT()
struct GAMEPLAYABILITIES_API FGameplayModifierCallback
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	TSubclassOf<class UGameplayEffectExtension> ExtensionClass;

	/** Modifications to attributes on the source instigator to be used in the extension class */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	TArray<FExtensionAttributeModifierInfo> SourceAttributeModifiers;

	/** Modifications to attributes on the target to be used in the extension class */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	TArray<FExtensionAttributeModifierInfo> TargetAttributeModifiers;
};

USTRUCT()
struct FGameplayEffectStackingCallbacks
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditDefaultsOnly, Category = GEStack)
	TArray<TSubclassOf<class UGameplayEffectStackingExtension> >	ExtensionClasses;
};





/** Struct defining gameplay attribute capture options for gameplay effects */
USTRUCT()
struct FGameplayEffectAttributeCaptureDefinition
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectAttributeCaptureDefinition()
	{

	}

	FGameplayEffectAttributeCaptureDefinition(FGameplayAttribute InAttribute, EGameplayEffectAttributeCaptureSource InSource, bool InSnapshot)
		: AttributeToCapture(InAttribute), AttributeSource(InSource), bSnapshot(InSnapshot)
	{

	}

	/** Gameplay attribute to capture */
	UPROPERTY(EditDefaultsOnly, Category=Capture)
	FGameplayAttribute AttributeToCapture;

	/** Source of the gameplay attribute */
	UPROPERTY(EditDefaultsOnly, Category=Capture)
	EGameplayEffectAttributeCaptureSource AttributeSource;

	/** Whether the attribute should be snapshotted or not */
	UPROPERTY(EditDefaultsOnly, Category=Capture)
	bool bSnapshot;

	/** Equality/Inequality operators */
	bool operator==(const FGameplayEffectAttributeCaptureDefinition& Other) const;
	bool operator!=(const FGameplayEffectAttributeCaptureDefinition& Other) const;

	FString ToSimpleString() const;
};

/** Structure for defining custom logic when a gameplay effect execution happens */
USTRUCT()
struct FGameplayEffectExecutionDefinition
{
	GENERATED_USTRUCT_BODY()

	void OnObjectPostEditChange(UObject* Object, FPropertyChangedEvent& PropertyChangedEvent);

	// @todo: Ideally this would be an interface, but can't store one in a UPROPERTY. Need to figure out where we want custom
	// execution to come from, as it could potentially be multiple places in reality. Might want native code, might want a blueprint graph,
	// might want to implement it inline in the gameplay effect graph (maybe? would need to be static or instancing becomes a mess), might
	// even just want it to be a "custom calculation" inline with fancy math operations.
	UPROPERTY(EditDefaultsOnly, Category=Execution)
	TSubclassOf<UGameplayEffectCalculation> CalculationClass;
	
	// @todo: This area should also be able to capture attributes because it's going to likely want them for the execution, as well as for
	// scoped modifiers. Need to decide where they live/how this looks in the UI/etc.
	// 
	
	

	// FGameplayEffectAttributeCaptureDefinition

	/** Modifiers that are applied "in place" during the execution calculation */
	UPROPERTY(EditDefaultsOnly, Category = Execution)
	TArray<FExtensionAttributeModifierInfo>	CalculationModifiers;
};

/** The runtime/mutable data of an ExecutionDefinition */
USTRUCT()
struct FGameplayEffectExecutionSpec
{
	GENERATED_USTRUCT_BODY()

	//FAggregatorRef	
};

// @todo: Enum to choose between types; Details customization to hide the other one
// @todo: Add Math Expression parser
USTRUCT()
struct FGameplayEffectMagnitude
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectMagnitude()
	: ScalableFloatMagnitude()
	, CalculationClassMagnitude(nullptr)
	{
	}

	UPROPERTY(EditDefaultsOnly, Category=Magnitude)
	FScalableFloat ScalableFloatMagnitude;

	UPROPERTY(EditDefaultsOnly, Category=Magnitude)
	TSubclassOf<UGameplayModCalculation> CalculationClassMagnitude;

	/** Attributes to capture for usage in custom calculations for magnitude */
	UPROPERTY(EditDefaultsOnly, Category=Magnitude)
	TArray<FGameplayEffectAttributeCaptureDefinition> CustomMagnitudeCalculationAttributes;
};

/**
 * FGameplayModifierInfo
 *	Tells us "Who/What we" modify
 *	Does not tell us how exactly
 *
 */
USTRUCT()
struct GAMEPLAYABILITIES_API FGameplayModifierInfo
{
	GENERATED_USTRUCT_BODY()

	FGameplayModifierInfo()	
	: ModifierOp(EGameplayModOp::Additive)
	{

	}

	/** The Attribute we modify or the GE we modify modifies. */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier, meta=(FilterMetaTag="HideFromModifiers"))
	FGameplayAttribute Attribute;

	/** The numeric operation of this modifier: Override, Add, Multiply, etc  */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	TEnumAsByte<EGameplayModOp::Type> ModifierOp;

	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier, meta=(DisplayName="Custom Ops"))
	TArray<FGameplayModifierCallback> Callbacks;

	/** How much this modifies what it is applied to */
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	FScalableFloat Magnitude; // Not modified from defaults

	// @todo: Rename/fix-up/etc.
	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	FGameplayEffectMagnitude MagnitudeV2;

	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	FGameplayTagRequirements	SourceTags;

	UPROPERTY(EditDefaultsOnly, Category=GameplayModifier)
	FGameplayTagRequirements	TargetTags;


	FString ToSimpleString() const
	{
		return FString::Printf(TEXT("%s BaseVaue: %s"), *EGameplayModOpToString(ModifierOp), *Magnitude.ToSimpleString());
	}
};

/**
 * FGameplayEffectCue
 *	This is a cosmetic cue that can be tied to a UGameplayEffect. 
 *  This is essentially a GameplayTag + a Min/Max level range that is used to map the level of a GameplayEffect to a normalized value used by the GameplayCue system.
 */
USTRUCT()
struct FGameplayEffectCue
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectCue()
		: MinLevel(0.f)
		, MaxLevel(0.f)
	{
	}

	FGameplayEffectCue(const FGameplayTag& InTag, float InMinLevel, float InMaxLevel)
		: MinLevel(InMinLevel)
		, MaxLevel(InMaxLevel)
	{
		GameplayCueTags.AddTag(InTag);
	}

	/** The attribute to use as the source for cue magnitude. If none use level */
	UPROPERTY(EditDefaultsOnly, Category = GameplayCue)
	FGameplayAttribute MagnitudeAttribute;

	/** The minimum level that this Cue supports */
	UPROPERTY(EditDefaultsOnly, Category = GameplayCue)
	float	MinLevel;

	/** The maximum level that this Cue supports */
	UPROPERTY(EditDefaultsOnly, Category = GameplayCue)
	float	MaxLevel;

	/** Tags passed to the gameplay cue handler when this cue is activated */
	UPROPERTY(EditDefaultsOnly, Category = GameplayCue, meta = (Categories="GameplayCue"))
	FGameplayTagContainer GameplayCueTags;

	float NormalizeLevel(float InLevel)
	{
		float Range = MaxLevel - MinLevel;
		if (Range <= KINDA_SMALL_NUMBER)
		{
			return 1.f;
		}

		return FMath::Clamp((InLevel - MinLevel) / Range, 0.f, 1.0f);
	}
};

/**
 * UGameplayEffect
 *	The GameplayEffect definition. This is the data asset defined in the editor that drives everything.
 */
UCLASS(BlueprintType)
class GAMEPLAYABILITIES_API UGameplayEffect : public UDataAsset, public IGameplayTagAssetInterface
{

public:
	GENERATED_UCLASS_BODY()

	/** Infinite duration */
	static const float INFINITE_DURATION;

	/** No duration; Time specifying instant application of an effect */
	static const float INSTANT_APPLICATION;

	/** Constant specifying that the combat effect has no period and doesn't check for over time application */
	static const float NO_PERIOD;

	/** No Level/Level not set */
	static const float INVALID_LEVEL;

#if WITH_EDITORONLY_DATA
	/** Template to derive starting values and editing customization from */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Template)
	UGameplayEffectTemplate*	Template;

	/** When false, show a limited set of properties for editing, based on the template we are derived from */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Template)
	bool ShowAllProperties;
#endif

	/** Duration in seconds. 0.0 for instantaneous effects; -1.0 for infinite duration. */
	UPROPERTY(EditDefaultsOnly, Category=GameplayEffect)
	FScalableFloat	Duration;

	/** Period in seconds. 0.0 for non-periodic effects */
	UPROPERTY(EditDefaultsOnly, Category=GameplayEffect)
	FScalableFloat	Period;
	
	/** Array of modifiers that will affect the target of this effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=GameplayEffect)
	TArray<FGameplayModifierInfo> Modifiers;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayEffect)
	TArray<FGameplayEffectExecutionDefinition>	Executions;

	/** Probability that this gameplay effect will be applied to the target actor (0.0 for never, 1.0 for always) */
	UPROPERTY(EditDefaultsOnly, Category=Application, meta=(GameplayAttribute="True"))
	FScalableFloat	ChanceToApplyToTarget;

	/** Probability that this gameplay effect will execute on another GE after it has been successfully applied to the target actor (0.0 for never, 1.0 for always) */
	UPROPERTY(EditDefaultsOnly, Category = Application, meta = (GameplayAttribute = "True"))
	FScalableFloat	ChanceToExecuteOnGameplayEffect;

	/** other gameplay effects that will be applied to the target of this effect if this effect applies */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameplayEffect)
	TArray<UGameplayEffect*> TargetEffects;

	// ------------------------------------------------
	// Gameplay tag interface
	// ------------------------------------------------

	/** Overridden to return requirements tags */
	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	void ValidateGameplayEffect();

	/** Container of gameplay tags to be cleared upon effect application; Any active effects with these tags that can be cleared, will be */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Tags)
	FGameplayTagContainer ClearTagsContainer;

	virtual void PostLoad() override;

	// ----------------------------------------------

	/** Cues to trigger non-simulated reactions in response to this GameplayEffect such as sounds, particle effects, etc */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Display)
	TArray<FGameplayEffectCue>	GameplayCues;

	/** Description of this gameplay effect. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Display)
	FText Description;

	// ----------------------------------------------

	/** Specifies the rule used to stack this GameplayEffect with other GameplayEffects. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	TEnumAsByte<EGameplayEffectStackingPolicy::Type>	StackingPolicy;

	/** An identifier for the stack. Both names and stacking policy must match for GameplayEffects to stack with each other. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	FName StackedAttribName;

	/** Specifies a custom stacking rule if one is needed. */
	UPROPERTY(EditDefaultsOnly, Category = Stacking)
	TSubclassOf<class UGameplayEffectStackingExtension> StackingExtension;



	// ----------------------------------------------------------------------
	//		Tag pass: properties that make sense "right now" (remove this comment DaveR/BillyB)
	// ----------------------------------------------------------------------
	

	/** The GameplayEffect's Tags: tags the the GE *has* and DOES NOT give to the actor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags, meta=(DisplayName="GameplayEffectAssetTag"))
	FGameplayTagContainer GameplayEffectTags;

	
	/** "These tags are applied to the actor I am applied to" */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags)
	FGameplayTagContainer OwnedTagsContainer;
	
	/** Once Applied, these tags requirements are used to determined if the GameplayEffect is "on" or "off". A GameplayEffect can be off and do nothing, but still applied. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags)
	FGameplayTagRequirements OngoingTagRequirements;

	/** Tag requirements for this GameplayEffect to be applied to a target. This is pass/fail at the time of application. If fail, this GE fails to apply. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Tags)
	FGameplayTagRequirements ApplicationTagRequirements;

};

// GE_REMOVE this may go away? Or still useful for storing 'Final results of shit we calculated'?
struct FGameplayModifierEvaluatedData
{
	FGameplayModifierEvaluatedData()
		: Magnitude(0.f)
		, IsValid(false)
	{
	}

	FGameplayModifierEvaluatedData(float InMagnitude, FActiveGameplayEffectHandle InHandle = FActiveGameplayEffectHandle())
		: Magnitude(InMagnitude)
		, Handle(InHandle)
		, IsValid(true)
	{
	}

	FGameplayTagContainer	SourceTags;
	FGameplayTagContainer	TargetTags;

	float	Magnitude;
	FActiveGameplayEffectHandle	Handle;	// Handle of the active gameplay effect that originated us. Will be invalid in many cases
	bool IsValid;
};

/**
 * Modifier Specification
 *	-Const data (FGameplayModifierInfo) tells us what we modify, what we can modify
 *	-Mutable Aggregated data tells us how we modify (magnitude).
 *  
 * Modifiers can be modified. A modifier spec holds these modifications along with a reference to the const data about the modifier.
 * 
 */
struct FModifierSpec
{
	FModifierSpec(const FGameplayModifierInfo& InInfo);

	// @todo: Need a way to indicate whether the magnitude was properly calculated or not. Asking for a value early (before target is available, etc.
	// will just result in incorrect values)
	float GetEvaluatedMagnitude() const;
	void CalculateMagnitude(OUT struct FGameplayEffectSpec& OwnerSpec);

	// Hard Ref to what we modify, this stuff is const and never changes
	const FGameplayModifierInfo& Info;

	// @todo: Probably need to make the modifier info private so people don't go gunking around in the magnitude
	/** In the event that the modifier spec requires custom magnitude calculations, this is the authoritative, last evaluated value of the magnitude */
	float EvaluatedMagnitude;

	FString ToSimpleString() const
	{
		return Info.ToSimpleString();
	}
};

/** Saves list of modified attributes, to use for gameplay cues or later processing */
USTRUCT()
struct FGameplayEffectModifiedAttribute
{
	GENERATED_USTRUCT_BODY()

	/** The attribute that has been modified */
	UPROPERTY()
	FGameplayAttribute Attribute;

	/** Total magnitude applied to that attribute */
	UPROPERTY()
	float TotalMagnitude;

	FGameplayEffectModifiedAttribute() : TotalMagnitude(0.0f) {}
};

/** Struct used to hold the result of a gameplay attribute capture; Initially seeded by definition data, but then populated by ability system component when appropriate */
USTRUCT()
struct FGameplayEffectAttributeCaptureSpec
{
	// Allow these as friends so they can seed the aggregator, which we don't otherwise want exposed
	friend struct FActiveGameplayEffectsContainer;
	friend class UAbilitySystemComponent;

	GENERATED_USTRUCT_BODY()

	// Constructors
	FGameplayEffectAttributeCaptureSpec();
	FGameplayEffectAttributeCaptureSpec(const FGameplayEffectAttributeCaptureDefinition& InDefinition);

	// @todo:
	// API should provide way to get:
		// Final modified value
		// Base value
		// Bonus value (Final - Base)
		// Mods (optionally matching tag filter)
	
	/** Simple accessor to backing capture definition */
	const FGameplayEffectAttributeCaptureDefinition& GetBackingDefinition() const;

	const FAggregator& GetAggregator() const;
		
private:

	/** Copy of the definition the spec should adhere to for capturing */
	UPROPERTY()
	FGameplayEffectAttributeCaptureDefinition BackingDefinition;

	/** Ref to the aggregator for the captured attribute */
	FAggregatorRef AttributeAggregator;
};

/** Struct used to handle a collection of captured source and target attributes */
USTRUCT()
struct GAMEPLAYABILITIES_API FGameplayEffectAttributeCaptureSpecContainer
{
	GENERATED_USTRUCT_BODY()

public:

	/**
	 * Add a definition to be captured by the owner of the container. Will not add the definition if its exact
	 * match already exists within the container.
	 * 
	 * @param InCaptureDefinition	Definition to capture with
	 */
	void AddCaptureDefinition(const FGameplayEffectAttributeCaptureDefinition& InCaptureDefinition);

	/**
	 * Capture source or target attributes from the specified component. Should be called by the container's owner.
	 * 
	 * @param InAbilitySystemComponent	Component to capture attributes from
	 * @param InCaptureSource			Whether to capture attributes as source or target
	 */
	void CaptureAttributes(class UAbilitySystemComponent* InAbilitySystemComponent, EGameplayEffectAttributeCaptureSource InCaptureSource);

	// @todo:
	// API should provide access to querying specs by definition; Wrapper around @todo API for specs above
	
	const FAggregator& GetAggregator(const FGameplayEffectAttributeCaptureDefinition& InDefinition) const;

	/** Returns whether the container has at least one spec w/o snapshotted attributes */
	bool HasNonSnapshottedAttributes() const;

private:

	/** Captured attributes from the source of a gameplay effect */
	UPROPERTY()
	TArray<FGameplayEffectAttributeCaptureSpec> SourceAttributes;

	/** Captured attributes from the target of a gameplay effect */
	UPROPERTY()
	TArray<FGameplayEffectAttributeCaptureSpec> TargetAttributes;

	/** If true, has at least one capture spec that did not request a snapshot */
	UPROPERTY()
	bool bHasNonSnapshottedAttributes;
};

/**
 * GameplayEffect Specification. Tells us:
 *	-What UGameplayEffect (const data)
 *	-What Level
 *  -Who instigated
 *  
 * FGameplayEffectSpec is modifiable. We start with initial conditions and modifications be applied to it. In this sense, it is stateful/mutable but it
 * is still distinct from an FActiveGameplayEffect which in an applied instance of an FGameplayEffectSpec.
 */
USTRUCT()
struct FGameplayEffectSpec
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectSpec()
		: Duration(UGameplayEffect::INSTANT_APPLICATION)
		, Period(UGameplayEffect::NO_PERIOD)
		, ChanceToApplyToTarget(1.f)
		, ChanceToExecuteOnGameplayEffect(1.f)
		, bTopOfStack(false)
		, Level(UGameplayEffect::INVALID_LEVEL)
	{
		// If we initialize a GameplayEffectSpec with no level object passed in.
	}

	FGameplayEffectSpec(const UGameplayEffect* InDef, const FGameplayEffectContextHandle& InEffectContext, float Level = UGameplayEffect::INVALID_LEVEL);
	
	UPROPERTY()
	const UGameplayEffect* Def;
	
	// Replicated
	UPROPERTY()
	FGameplayEffectContextHandle EffectContext; // This tells us how we got here (who / what applied us)

	// Replicated
	UPROPERTY()
	TArray<FGameplayEffectModifiedAttribute> ModifiedAttributes;
	
	/** Attributes captured by the spec that are relevant to custom calculations, potentially in owned modifiers, etc.; NOT replicated to clients */
	UPROPERTY(NotReplicated)
	FGameplayEffectAttributeCaptureSpecContainer CapturedRelevantAttributes;

	// Looks for an existing modified attribute struct, may return NULL
	const FGameplayEffectModifiedAttribute* GetModifiedAttribute(const FGameplayAttribute& Attribute) const;
	FGameplayEffectModifiedAttribute* GetModifiedAttribute(const FGameplayAttribute& Attribute);

	// Adds a new modified attribute struct, will always add so check to see if it exists first
	FGameplayEffectModifiedAttribute* AddModifiedAttribute(const FGameplayAttribute& Attribute);

	// Deletes any modified attributes that aren't needed. Call before replication
	void PruneModifiedAttributes();

	float GetDuration() const;
	float GetPeriod() const;
	float GetChanceToApplyToTarget() const;
	float GetChanceToExecuteOnGameplayEffect() const;
	float GetMagnitude(const FGameplayAttribute &Attribute) const;

	EGameplayEffectStackingPolicy::Type GetStackingType() const;

	// other effects that need to be applied to the target if this effect is successful
	TArray< TSharedRef< FGameplayEffectSpec > > TargetEffectSpecs;

	// The duration in seconds of this effect
	// instantaneous effects should have a duration of UGameplayEffect::INSTANT_APPLICATION
	// effects that last forever should have a duration of UGameplayEffect::INFINITE_DURATION
	UPROPERTY()
	float Duration;

	// The period in seconds of this effect.
	// Nonperiodic effects should have a period of UGameplayEffect::NO_PERIOD
	UPROPERTY()
	float Period;

	// The chance, in a 0.0-1.0 range, that this GameplayEffect will be applied to the target Attribute or GameplayEffect.
	UPROPERTY()
	float ChanceToApplyToTarget;

	UPROPERTY()
	float ChanceToExecuteOnGameplayEffect;

	// This should only be true if this is a stacking effect and at the top of its stack
	// (FIXME: should this be part of the spec or FActiveGameplayEffect?)
	UPROPERTY()
	bool bTopOfStack;

	// Captured Source Tags on GameplayEffectSpec creation.
	//	-Do we ever not snapshot these tags?
	UPROPERTY()
	FGameplayTagContainer	CapturedSourceTags;

	// Tags from the target, captured during execute?
	// Not sure if this is the right place or not, or how 'contextual' tags get injected (headshot, crit, etc)
	// Does this overlap with info in the GameplayEffectContextHandle? Should it just all live there?
	// For now - putting it here so places have one spot to pull from. Need to figure this out long term.
	UPROPERTY()
	FGameplayTagContainer	CapturedTargetTags;
	
	TArray<FModifierSpec> Modifiers;
	
	TArray<FGameplayEffectExecutionSpec>	Executions;

	// GE_REMOVE: this feels strange but need to start standardizing how people get this info.
	float GetModifierMagnitude(const FModifierSpec& ModSpec) const;
	void CalculateModifierMagnitudes();

	FString ToSimpleString() const
	{
		return FString::Printf(TEXT("%s"), *Def->GetName());
	}

	void SetLevel(float InLevel);

	float GetLevel() const;

	void PrintAll() const;

	

private:

	// Replicated	
	UPROPERTY()
	float Level;	
};

/**
 * Active GameplayEffect instance
 *	-What GameplayEffect Spec
 *	-Start time
 *  -When to execute next
 *  -Replication callbacks
 *
 */
USTRUCT()
struct FActiveGameplayEffect : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	FActiveGameplayEffect()
		: StartGameStateTime(0)
		, StartWorldTime(0.f)
		, IsInhibited(true)
	{
	}

	FActiveGameplayEffect(FActiveGameplayEffectHandle InHandle, const FGameplayEffectSpec &InSpec, float CurrentWorldTime, int32 InStartGameStateTime, FPredictionKey InPredictionKey)
		: Handle(InHandle)
		, Spec(InSpec)
		, PredictionKey(InPredictionKey)
		, StartGameStateTime(InStartGameStateTime)
		, StartWorldTime(CurrentWorldTime)
		, IsInhibited(true)
	{
	}

	FActiveGameplayEffectHandle Handle;

	UPROPERTY()
	FGameplayEffectSpec Spec;

	UPROPERTY()
	FPredictionKey	PredictionKey;

	/** Game time this started */
	UPROPERTY()
	int32 StartGameStateTime;

	UPROPERTY(NotReplicated)
	float StartWorldTime;

	// Not sure if this should replicate or not. If replicated, we may have trouble where IsHibited doesn't appear to change when we do tag checks (because it was previously inhibited, but replication made it inhibited).
	UPROPERTY()
	bool IsInhibited;

	FOnActiveGameplayEffectRemoved	OnRemovedDelegate;

	FTimerHandle PeriodHandle;

	FTimerHandle DurationHandle;

	float GetTimeRemaining(float WorldTime)
	{
		float Duration = GetDuration();		
		return (Duration == UGameplayEffect::INFINITE_DURATION ? -1.f : Duration - (WorldTime - StartWorldTime));
	}
	
	float GetDuration() const
	{
		return Spec.GetDuration();
	}

	float GetPeriod() const
	{
		return Spec.GetPeriod();
	}

	void CheckOngoingTagRequirements(const FGameplayTagContainer& OwnerTags, struct FActiveGameplayEffectsContainer& OwningContainer);

	bool CanBeStacked(const FActiveGameplayEffect& Other) const;

	void PrintAll() const;

	void PreReplicatedRemove(const struct FActiveGameplayEffectsContainer &InArray);
	void PostReplicatedAdd(const struct FActiveGameplayEffectsContainer &InArray);
	void PostReplicatedChange(const struct FActiveGameplayEffectsContainer &InArray);

	bool operator==(const FActiveGameplayEffect& Other)
	{
		return Handle == Other.Handle;
	}
};

/**
 *	Generic querying data structure for active GameplayEffects. Lets us ask things like:
 *		Give me duration/magnitude of active gameplay effects with these tags
 *		Give me handles to all activate gameplay effects modifying this attribute.
 *		
 *	Any requirements specified in the query are required: must meet "all" not "one".
 */
USTRUCT()
struct FActiveGameplayEffectQuery
{
	GENERATED_USTRUCT_BODY()

	FActiveGameplayEffectQuery()
		: TagContainer(NULL)
	{
	}

	FActiveGameplayEffectQuery(const FGameplayTagContainer* InTagContainer, FGameplayAttribute InModifyingAttribute = FGameplayAttribute())
		: TagContainer(InTagContainer)
		, ModifyingAttribute(InModifyingAttribute)
	{
	}

	bool Matches(const FActiveGameplayEffect& Effect) const;

	const FGameplayTagContainer* TagContainer;

	/** Matches on GameplayEffects which modify given attribute */
	FGameplayAttribute ModifyingAttribute;
};

/**
 * Active GameplayEffects Container
 *	-Bucket of ActiveGameplayEffects
 *	-Needed for FFastArraySerialization
 *  
 * This should only be used by UAbilitySystemComponent. All of this could just live in UAbilitySystemComponent except that we need a distinct USTRUCT to implement FFastArraySerializer.
 *
 */
USTRUCT()
struct FActiveGameplayEffectsContainer : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY();

	friend struct FActiveGameplayEffect;
	friend class UAbilitySystemComponent;

	FActiveGameplayEffectsContainer() : bNeedToRecalculateStacks(false) {};

	UAbilitySystemComponent* Owner;

	UPROPERTY()
	TArray< FActiveGameplayEffect >	GameplayEffects;

	void RegisterWithOwner(UAbilitySystemComponent* Owner);	
	
	FActiveGameplayEffect& CreateNewActiveGameplayEffect(const FGameplayEffectSpec &Spec, FPredictionKey InPredictionKey);

	FActiveGameplayEffect* GetActiveGameplayEffect(const FActiveGameplayEffectHandle Handle);
		
	void ExecuteActiveEffectsFrom(FGameplayEffectSpec &Spec, FPredictionKey PredictionKey = FPredictionKey() );
	
	void ExecutePeriodicGameplayEffect(FActiveGameplayEffectHandle Handle);	// This should not be outward facing to the skill system API, should only be called by the owning AbilitySystemComponent

	bool RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle);
	
	float GetGameplayEffectDuration(FActiveGameplayEffectHandle Handle) const;

	float GetGameplayEffectMagnitude(FActiveGameplayEffectHandle Handle, FGameplayAttribute Attribute) const;

	// returns true if the handle points to an effect in this container that is not a stacking effect or an effect in this container that does stack and is applied by the current stacking rules
	// returns false if the handle points to an effect that is not in this container or is not applied because of the current stacking rules
	bool IsGameplayEffectActive(FActiveGameplayEffectHandle Handle) const;

	/**
	 * Populate the specified capture spec with the data necessary to capture an attribute from the container
	 * 
	 * @param OutCaptureSpec	[OUT] Capture spec to populate with captured data
	 */
	void CaptureAttributeForGameplayEffect(OUT FGameplayEffectAttributeCaptureSpec& OutCaptureSpec);

	void PrintAllGameplayEffects() const;

	int32 GetNumGameplayEffects() const
	{
		return GameplayEffects.Num();
	}

	void CheckDuration(FActiveGameplayEffectHandle Handle);

	void StacksNeedToRecalculate();

	// recalculates all of the stacks in the current container
	void RecalculateStacking();

	bool NetDeltaSerialize(FNetDeltaSerializeInfo & DeltaParms)
	{
		return FastArrayDeltaSerialize<FActiveGameplayEffect>(GameplayEffects, DeltaParms, *this);
	}

	void PreDestroy();

	bool bNeedToRecalculateStacks;

	// ------------------------------------------------

	bool CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext);
	
	TArray<float> GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const;

	TArray<float> GetActiveEffectsDuration(const FActiveGameplayEffectQuery Query) const;

	void RemoveActiveEffects(const FActiveGameplayEffectQuery Query);

	int32 GetGameStateTime() const;

	float GetWorldTime() const;

	bool HasReceivedEffectWithPredictedKey(FPredictionKey PredictionKey) const;

	bool HasPredictedEffectWithPredictedKey(FPredictionKey PredictionKey) const;
		
	void SetBaseAttributeValueFromReplication(FGameplayAttribute Attribute, float BaseBalue);

	// -------------------------------------------------------------------------------------------

	FOnGameplayAttributeChange& RegisterGameplayAttributeEvent(FGameplayAttribute Attribute);

	void OnOwnerTagChange(FGameplayTag TagChange, int32 NewCount);

private:

	FTimerHandle StackHandle;

	void InternalUpdateNumericalAttribute(FGameplayAttribute Attribute, float NewValue, const FGameplayEffectModCallbackData* ModData);

	bool IsNetAuthority() const;

	/** Called internally to actually remove a GameplayEffect */
	bool InternalRemoveActiveGameplayEffect(int32 Idx);

	/** Called both in server side creation and replication creation/deletion */
	void InternalOnActiveGameplayEffectAdded(FActiveGameplayEffect& Effect);
	void InternalOnActiveGameplayEffectRemoved(const FActiveGameplayEffect& Effect);

	// -------------------------------------------------------------------------------------------

	TMap<FGameplayAttribute, FAggregatorRef>		AttributeAggregatorMap;

	TMap<FGameplayAttribute, FOnGameplayAttributeChange> AttributeChangeDelegates;

	TMap<FGameplayTag, TSet<FActiveGameplayEffectHandle> >	ActiveEffectTagDependencies;

	FAggregatorRef& FindOrCreateAttributeAggregator(FGameplayAttribute Attribute);

	void OnAttributeAggregatorDirty(FAggregator* Aggregator, FGameplayAttribute Attribute);
};

template<>
struct TStructOpsTypeTraits< FActiveGameplayEffectsContainer > : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};


/** Allows blueprints to generate a GameplayEffectSpec once and then reference it by handle, to apply it multiple times/multiple targets. */
USTRUCT(BlueprintType)
struct GAMEPLAYABILITIES_API FGameplayEffectSpecHandle
{
	GENERATED_USTRUCT_BODY()

	FGameplayEffectSpecHandle() { }
	FGameplayEffectSpecHandle(FGameplayEffectSpec* DataPtr)
		: Data(DataPtr)
	{

	}

	TSharedPtr<FGameplayEffectSpec>	Data;

	bool IsValidCache;

	void Clear()
	{
		Data.Reset();
	}

	bool IsValid() const
	{
		return Data.IsValid();
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		ABILITY_LOG(Fatal, TEXT("FGameplayEffectSpecHandle should not be NetSerialized"));
		return false;
	}

	/** Comparison operator */
	bool operator==(FGameplayEffectSpecHandle const& Other) const
	{
		// Both invalid structs or both valid and Pointer compare (???) // deep comparison equality
		bool bBothValid = IsValid() && Other.IsValid();
		bool bBothInvalid = !IsValid() && !Other.IsValid();
		return (bBothInvalid || (bBothValid && (Data.Get() == Other.Data.Get())));
	}

	/** Comparison operator */
	bool operator!=(FGameplayEffectSpecHandle const& Other) const
	{
		return !(FGameplayEffectSpecHandle::operator==(Other));
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayEffectSpecHandle> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithCopy = true,		// Necessary so that TSharedPtr<FGameplayAbilityTargetData> Data is copied around
		WithNetSerializer = true,
		WithIdenticalViaEquality = true,
	};
};
