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
UCLASS(HeaderGroup=Component, ClassGroup=SkillSystem, hidecategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class SKILLSYSTEM_API UAttributeComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	virtual ~UAttributeComponent();

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
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec &GameplayEffect, UAttributeComponent *Target, FModifierQualifier BaseQualifier = FModifierQualifier()) const;
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &GameplayEffect, FModifierQualifier BaseQualifier = FModifierQualifier());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects)
	bool RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle);

	float GetGameplayEffectDuration(FActiveGameplayEffectHandle Handle) const;

	// Not happy with this interface but don't see a better way yet. How should outside code (UI, etc) ask things like 'how much is this gameplay effect modifying my damage by'
	// (most likely we want to catch this on the backend - when damage is applied we can get a full dump/history of how the number got to where it is. But still we may need polling methods like below (how much would my damage be)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects)
	float GetGameplayEffectMagnitude(FActiveGameplayEffectHandle Handle, FGameplayAttribute Attribute) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects)
	float GetGameplayEffectMagnitudeByTag(FActiveGameplayEffectHandle InHandle, FName GameplayTag) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects)
	bool IsGameplayEffectActive(FActiveGameplayEffectHandle InHandle) const;
	

	// --------------------------------------------
	// Possibly useful but not primary API functions:
	// --------------------------------------------

	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToTarget(UGameplayEffect *GameplayEffect, UAttributeComponent *Target, float Level = FGameplayEffectLevelSpec::INVALID_LEVEL, FModifierQualifier BaseQualifier = FModifierQualifier()) const;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToSelf"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToSelf(UGameplayEffect *GameplayEffect, float Level, AActor *Instigator);
	
	FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(UGameplayEffect *GameplayEffect, float Level, AActor *Instigator, FModifierQualifier BaseQualifier = FModifierQualifier() );

	int32 GetNumActiveGameplayEffect() const;

	void AddDependancyToAttribute(FGameplayAttribute Attribute, const TWeakPtr<FAggregator> InDependant);

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
