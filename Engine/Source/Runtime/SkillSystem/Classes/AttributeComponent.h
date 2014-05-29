// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayEffect.h"
#include "SkillSystemTypes.h"
#include "AttributeComponent.generated.h"

SKILLSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogAttributeComponent, Log, All);

DECLARE_MULTICAST_DELEGATE(FAttributeComponentPredictionKeyClear);

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
	friend class ASkillSystemDebugHUD;

	virtual ~UAttributeComponent();

	virtual void InitializeComponent() OVERRIDE;

	/** Finds existing AttributeSet */
	template <class T >
	T*	GetSet()
	{
		return (T*)GetAttributeSubobject(T::StaticClass());
	}

	template <class T >
	T*	GetSetChecked()
	{
		return (T*)GetAttributeSubobjectChecked(T::StaticClass());
	}

	/** Adds a new AttributeSet (initialized to default values) */
	template <class T >
	T*  AddSet()
	{
		return (T*)GetOrCreateAttributeSubobject(T::StaticClass());
	}

	/** Adds a new AttributeSet that is a DSO (created by called in their CStor) */
	template <class T>
	T*	AddDefaultSubobjectSet(TSubobjectPtr<T> Subobject)
	{
		T* Set = Subobject.Get();
		SpawnedAttributes.Add(Set);
		return Set;
	}

	UFUNCTION(BlueprintCallable, Category="Skills")
	UAttributeSet * InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable);

	UFUNCTION(BlueprintCallable, Category="Skills")
	void ModifyStats(TSubclassOf<class UAttributeSet> Attributes, FDataTableRowHandle ModifierHandle);

	UPROPERTY(EditAnywhere, Category="AttributeTest")
	TArray<FAttributeDefaults>	DefaultStartingData;

	UPROPERTY(Replicated)
	TArray<UAttributeSet*>	SpawnedAttributes;

	void SetNumericAttribute(const FGameplayAttribute &Attribute, float NewFloatValue);
	float GetNumericAttribute(const FGameplayAttribute &Attribute);

	// -- Replication -------------------------------------------------------------------------------------------------

	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) OVERRIDE;
	
	virtual void GetSubobjectsWithStableNamesForNetworking(TArray<UObject*>& Objs) OVERRIDE;

	virtual void PostNetReceive() OVERRIDE;

	/**
	 *	Prediction Keys
	 *		PredictionKey is a simple system to allow clients to locally predict interactions within the AbilityComponent while having
	 *		a mechanism for merging results from the server.
	 *		
	 *		There are two main cases:
	 *			-Client predicts something and is wrong. (Mispredictions). He must cleanup stuff that he created or changed.
	 *			-Client predicts something and is right. 
	 *			
	 *			The Client predicts something and asks the server. "Can I do X? PredictionKey=Y".
	 *			Server can reply immediately and directly with yes or no, and include the PredictionKey with that answer.
	 *			
	 *			If No, the client can just immediately cleanup whatever he predicted. This is straightforward.
	 *			
	 *			If Yes, there is a small window where the Yes RPC is received but the actor/component properties have not been updated.
	 *			The client's cannot immediately cleanup his prediction work: he needs to wait for property replication.
	 *			
	 *			This is where ReplicatedPredictionKey comes in. 
	 *			
	 *			When the server allows a predicted action, he sets ReplicatedPredictionKey to this value. The client then looks
	 *			at this replicated value. In OnRep_PredictionKey, the client goes through and cleans up all stuff that was predicted
	 *			and is no longer needed.
	 *			
	 *			-Client calls GetNextPredictionKey() at the start of a predicted action. This gives him a key to use for this 'run'.
	 *			-Client can predict whatever he wants as long as he registers an 'undo' function with GetPredictionKeyDelegate().
	 *			-This delegate is invoked if the client is denied his prediction, or if he is right and property replication catches up.
	 *			
	 *	 
	 */

	UPROPERTY(ReplicatedUsing=OnRep_PredictionKey)
	int32	ReplicatedPredictionKey;

	UPROPERTY(transient)
	int32	LocalPredictionKey;

	int32	GetNextPredictionKey();

	UFUNCTION()
	void OnRep_PredictionKey();

	TArray<TPair<int32, FAttributeComponentPredictionKeyClear> >	PredictionDelegates;
	

	FAttributeComponentPredictionKeyClear &	GetPredictionKeyDelegate(int32 PredictionKey);

	/*
	virtual void OnSubobjectCreatedFromReplication(UObject *NewSubobject) OVERRIDE;
	
	virtual void OnSubobjectDestroyFromReplication(UObject *NewSubobject) OVERRIDE;
	*/

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

	/** This only exists so it can be hooked up to a multicast delegate */
	void RemoveActiveGameplayEffect_NoReturn(FActiveGameplayEffectHandle Handle)
	{
		RemoveActiveGameplayEffect(Handle);
	}

	float GetGameplayEffectDuration(FActiveGameplayEffectHandle Handle) const;

	float GetGameplayEffectDuration() const;

	// Not happy with this interface but don't see a better way yet. How should outside code (UI, etc) ask things like 'how much is this gameplay effect modifying my damage by'
	// (most likely we want to catch this on the backend - when damage is applied we can get a full dump/history of how the number got to where it is. But still we may need polling methods like below (how much would my damage be)
	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	float GetGameplayEffectMagnitude(FActiveGameplayEffectHandle Handle, FGameplayAttribute Attribute) const;

	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	float GetGameplayEffectMagnitudeByTag(FActiveGameplayEffectHandle InHandle, const FGameplayTag& InTag) const;

	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	bool IsGameplayEffectActive(FActiveGameplayEffectHandle InHandle) const;

	// Tags
	UFUNCTION(BlueprintCallable, Category=GameplayEffects)
	bool HasAnyTags(FGameplayTagContainer &Tags);

	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	bool HasAllTags(FGameplayTagContainer &Tags);
	

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

	/**
	 *	GameplayAbilities
	 *	
	 *	The role of the AttributeComponent wrt Abilities is to provide:
	 *		-Management of abiltiy instances (whether per actor or per instance).
	 *			-Someone *has* to keep track of these instances.
	 *			-Non instanced abilities *could* be executed without any ability stuff in attribute component.
	 *				They should be able to operate on an GameplayAbilityActorInfo + GameplayAbility.
	 *		
	 *	As convenience it may provide some other features:
	 *		-Some basic input binding (whether instanced or non instanced abilities).
	 *		-Concepts like "this component has these abilities
	 *	
	 */

	class UGameplayAbility * CreateNewInstanceOfAbility(UGameplayAbility *Ability);

	void CancelAbilitiesWithTags(const FGameplayTagContainer Tags, const FGameplayAbilityActorInfo & ActorInfo, UGameplayAbility * Ignore);
	
	/** References to non-replicating abilities that we instanced. We need to keep these refernces to avoid GC */
	UPROPERTY()
	TArray<class UGameplayAbility *>	NonReplicatedInstancedAbilities;

	/** References to replicating abilities that we instanced. We need to keep these refernces to avoid GC */
	UPROPERTY(Replicated)
	TArray<class UGameplayAbility *>	ReplicatedInstancedAbilities;

	/**
	 *	The abilities we can activate. 
	 *		-This will include CDOs for non instanced abilities and per-execution instanced abilities. 
	 *		-Actor-instanced abilities will be the actual instance (not CDO)
	 *		
	 *	This array is not vital for things to work. It is a convenience thing for 'giving abilties to the actor'. But abilities could also work on things
	 *	without an attribute component. For example an ability could be written to execute on a StaticMeshActor. As long as the ability doesn't require 
	 *	instancing or anything else that the AttributeComponent would provide, then it doesn't need the component to function.
	 */ 
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abilities")
	TArray<class UGameplayAbility *>	ActivatableAbilities;
	

	UFUNCTION(Server, WithValidation, Reliable)
	void	ServerTryActivateAbility(class UGameplayAbility * AbilityToActivate, int32 PredictionKey);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilityFailed(class UGameplayAbility * AbilityToActivate, int32 PredictionKey);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilitySucceed(class UGameplayAbility * AbilityToActivate, int32 PredictionKey);

	// ----------------------------------------------------------------------------------------------------------------

	/** This is probably temp for testing. We want to think a bit more on the API for outside stuff activating abilities */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool	ActivateAbility(int32 AbilityIdx);

	/** There needs to be a concept of an animating ability. Only one may exist at a time. New requestrs can be queued up, overridden, or ignored. */
	UPROPERTY()
	class UGameplayAbility *	AnimatingAbility;

	void	MontageBranchPoint_AbilityDecisionStop();

	void	MontageBranchPoint_AbilityDecisionStart();

	

	// -----------------------------------------------------------------------------
	
	

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

	UAttributeSet*	GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const;
	UAttributeSet*	GetAttributeSubobjectChecked(const TSubclassOf<UAttributeSet> AttributeClass) const;
	UAttributeSet*	GetOrCreateAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass);

	friend struct FActiveGameplayEffectsContainer;
	friend struct FActiveGameplayEffect;
};
