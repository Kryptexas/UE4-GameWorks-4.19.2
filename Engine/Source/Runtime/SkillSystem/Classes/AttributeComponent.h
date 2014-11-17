// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayEffect.h"
#include "AttributeComponent.generated.h"

SKILLSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogAttributeComponent, Log, All);

USTRUCT()
struct SKILLSYSTEM_API FAttributeDefaults
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category="AttributeTest")
	TSubclassOf<class UAttributeSet> Attributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AttributeTest")
	class UDataTable*	DefaultStartingTable;
};

/** 
 * 
 */
UCLASS(ClassGroup=SkillSystem, hidecategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class SKILLSYSTEM_API UAttributeComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	friend FGameplayEffectSpec;

	virtual ~UAttributeComponent();

	virtual void InitializeComponent() OVERRIDE;

	template <class T >
	T*	GetSet()
	{
		return (T*)GetAttributeSubobject(T::StaticClass());
	}

	UFUNCTION(BlueprintCallable, Category="Skills")
	UAttributeSet * InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable);

	UFUNCTION(BlueprintCallable, Category="Skills")
	void ModifyStats(TSubclassOf<class UAttributeSet> Attributes, FDataTableRowHandle ModifierHandle);

	UPROPERTY(EditAnywhere, Category="AttributeTest")
	TArray<FAttributeDefaults>	DefaultStartingData;

	UPROPERTY()
	TArray<UAttributeSet*>	SpawnedAttributes;

	UPROPERTY()
	float	TestFloat;

	void SetNumericAttribute(const FGameplayAttribute &Attribute, float NewFloatValue);
	float GetNumericAttribute(const FGameplayAttribute &Attribute);

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	GameplayEffects
	//	(maybe should go in a different component?)
	// ----------------------------------------------------------------------------------------------------------------

	// --------------------------------------------
	// Primary outward facing API for other systems:
	// --------------------------------------------
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec &GameplayEffect, UAttributeComponent *Target, FModifierQualifier BaseQualifier = FModifierQualifier());
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &GameplayEffect, FModifierQualifier BaseQualifier = FModifierQualifier());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects)
	bool RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle);

	float GetGameplayEffectDuration(FActiveGameplayEffectHandle Handle) const;

	float GetGameplayEffectDuration() const;

	// Not happy with this interface but don't see a better way yet. How should outside code (UI, etc) ask things like 'how much is this gameplay effect modifying my damage by'
	// (most likely we want to catch this on the backend - when damage is applied we can get a full dump/history of how the number got to where it is. But still we may need polling methods like below (how much would my damage be)
	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	float GetGameplayEffectMagnitude(FActiveGameplayEffectHandle Handle, FGameplayAttribute Attribute) const;

	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	float GetGameplayEffectMagnitudeByTag(FActiveGameplayEffectHandle InHandle, FName GameplayTag) const;

	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	bool IsGameplayEffectActive(FActiveGameplayEffectHandle InHandle) const;

	// Tags
	UFUNCTION(BlueprintCallable, Category=GameplayEffects)
	bool HasAnyTags(FGameplayTagContainer &Tags);
	

	// --------------------------------------------
	// Possibly useful but not primary API functions:
	// --------------------------------------------

	FActiveGameplayEffectHandle ApplyGameplayEffectToTarget(UGameplayEffect *GameplayEffect, UAttributeComponent *Target, float Level = FGameplayEffectLevelSpec::INVALID_LEVEL, FModifierQualifier BaseQualifier = FModifierQualifier());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToSelf"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator);
	
	FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator, FModifierQualifier BaseQualifier = FModifierQualifier() );

	int32 GetNumActiveGameplayEffect() const;

	void AddDependancyToAttribute(FGameplayAttribute Attribute, const TWeakPtr<FAggregator> InDependant);

	/** Tests if all modifiers in this GameplayEffect will leave the attribute > 0.f */
	bool CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator);

	// Generic 'Get expected mangitude (list) if I was to apply this outoging or incoming'

	// Get duration or magnitude (list) of active effects
	//		-Get duration of CD
	//		-Get magnitude + duration of a movespeed buff

	TArray<float> GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const;

	// --------------------------------------------
	// Temp / Debug
	// --------------------------------------------

	void TEMP_ApplyActiveGameplayEffects();
	
	void PrintAllGameplayEffects() const;

	void TEMP_TimerTest();
	void TEMP_TimerTestCallback(int32 x);
	
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) OVERRIDE;

	void PushGlobalCurveOveride(UCurveTable *OverrideTable)
	{
		if (OverrideTable)
		{
			GlobalCurveDataOverride.Overrides.Push(OverrideTable);
		}
	}

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	GameplayAbilities
	//	
	//	Should this be its own component? What should this do?
	//		-Load/initialize instanced subobjects
	//		-Bind to input component (maybe? where would weapons fit in?)
	//		
	//
	// ----------------------------------------------------------------------------------------------------------------

	/** Creates instances of the abilities that we need but doesn't necessarily  */
	void InitializeAbilities(class UGameplayAbilitySet *Set);

	/** Binds the InputComponent directly to the abilities. This is a convenience for some games. Other games may have abilities on weapons or who need some game logic to decide what ability a button press should go to. */
	void BindInputComponentToAbilities(UInputComponent *InputComponent, UGameplayAbilitySet *AbilitySet);

	UPROPERTY()
	TArray<class UGameplayAbility *>	InstancedAbilities;

	/** All abilities, including those that are shared/non instanced. */
	UPROPERTY(BlueprintReadOnly, Category="Abilities")
	TArray<class UGameplayAbility *>	AllAbilities;

	/** This is probably temp for testing. We want to think a bit more on the API for outside stuff activating abilities */
	UFUNCTION(BlueprintCallable, Category="Abilities")
	bool	ActivateAbility(int32 AbilityIdx);

	// There needs to be a concept of an animating ability. Only one may exist at a time. New requestrs can be queued up, overridden, or ignored.

	UPROPERTY()
	class UGameplayAbility *	AnimatingAbility;

	void	MontageBranchPoint_AbilityDecisionStop();

	void	MontageBranchPoint_AbilityDecisionStart();

	// --- Ability RPCs

	UFUNCTION(Server, WithValidation, Reliable)
	void	ServerTryActivateAbility(class UGameplayAbility * AbilityToActivate);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilityFailed(class UGameplayAbility * AbilityToActivate);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilitySucceed(class UGameplayAbility * AbilityToActivate);
	

private:

	// --------------------------------------------
	// Important internal functions
	// --------------------------------------------

	void ExecutePeriodicEffect(FActiveGameplayEffectHandle	Handle);

	void ExecuteGameplayEffect(const FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext);

	// --------------------------------------------
	// Internal Utility / helper functions
	// --------------------------------------------

	bool AreGameplayEffectApplicationRequirementsSatisfied(const class UGameplayEffect* EffectToAdd, FGameplayEffectInstigatorContext& Instigator) const;

	bool IsOwnerActorAuthoritative() const;

	void OnAttributeGameplayEffectSpecExected(const FGameplayAttribute &Attribute, const struct FGameplayEffectSpec &Spec, struct FGameplayModifierEvaluatedData &Data);

	const FGlobalCurveDataOverride * GetCurveDataOverride() const
	{
		// only return data if we have overrides. NULL if we dont.
		return (GlobalCurveDataOverride.Overrides.Num() > 0 ? &GlobalCurveDataOverride : NULL);
	}

	FGlobalCurveDataOverride	GlobalCurveDataOverride;
	

	// --------------------------------------------
	
	UPROPERTY(ReplicatedUsing=OnRep_GameplayEffects)
	FActiveGameplayEffectsContainer	ActiveGameplayEffects;

	UFUNCTION()
	void OnRep_GameplayEffects();

	// ---------------------------------------------

	UFUNCTION(NetMulticast, unreliable)
	void NetMulticast_InvokeGameplayCueExecuted(const FGameplayEffectSpec Spec);

	void InvokeGameplayCueExecute(const FGameplayEffectSpec &Spec);
	void InvokeGameplayCueActivated(const FGameplayEffectSpec &Spec);
	void InvokeGameplayCueAdded(const FGameplayEffectSpec &Spec);
	void InvokeGameplayCueRemoved(const FGameplayEffectSpec &Spec);

	// ---------------------------------------------

	

protected:

	virtual void OnRegister() OVERRIDE;

	UAttributeSet*	GetAttributeSubobject(UClass *AttributeClass) const;
	UAttributeSet*	GetAttributeSubobjectChecked(UClass *AttributeClass) const;
	UAttributeSet*	GetOrCreateAttributeSubobject(UClass *AttributeClass);

	friend struct FActiveGameplayEffectsContainer;
	friend struct FActiveGameplayEffect;
};
