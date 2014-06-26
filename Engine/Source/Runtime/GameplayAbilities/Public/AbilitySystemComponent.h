// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayEffect.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemComponent.generated.h"

GAMEPLAYABILITIES_API DECLARE_LOG_CATEGORY_EXTERN(LogAbilitySystemComponent, Log, All);

/** Used to register callbacks to confirm/cancel input */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityConfirmOrCancel);


USTRUCT()
struct GAMEPLAYABILITIES_API FAttributeDefaults
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category="AttributeTest")
	TSubclassOf<class UAttributeSet> Attributes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AttributeTest")
	class UDataTable*	DefaultStartingTable;
};

/** 
 *	UAbilitySystemComponent	
 *
 *	A component to easily interface with the 3 aspects of the AbilitySystem:
 *		-GameplayAbilities
 *		-GameplayEffects
 *		-GameplayAttributes
 *		
 *	This component will make life easier for interfacing with these subsystems, but is not completely required. The main functions are:
 *	
 *	GameplayAbilities:
 *		-Provides a way to give/assign abilities that can be used (by a player or AI for example)
 *		-Provides managment of instanced abilities (something must hold onto them)
 *		-Provides replication functionality
 *			-Ability state must always be replicated on the UGameplayAbility itself, but UAbilitySystemComponent can provide RPC replication
 *			for non-instanced gameplay abilities. (Explained more in GameplayAbility.h).
 *			
 *	GameplayEffects:
 *		-Provides an FActiveGameplayEffectsContainer for holding active GameplayEffects
 *		-Provides methods for apply GameplayEffect to a target or to self
 *		-Provides wrappers for querying information in FActiveGameplayEffectsContainers (duration, magnitude, etc)
 *		-Provides methods for clearing/remove GameplayEffects
 *		
 *	GameplayAttributes
 *		-Provides methods for allocating and initializing attribute sets
 *		-Provides methods for getting ATtributeSets
 *  
 * 
 */

UCLASS(ClassGroup=AbilitySystem, hidecategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class GAMEPLAYABILITIES_API UAbilitySystemComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

	friend FGameplayEffectSpec;
	friend class AAbilitySystemDebugHUD;

	virtual ~UAbilitySystemComponent();

	virtual void InitializeComponent() override;

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

	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;
	
	virtual void GetSubobjectsWithStableNamesForNetworking(TArray<UObject*>& Objs) override;

	virtual void PostNetReceive() override;

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

	TArray<TPair<int32, FAbilitySystemComponentPredictionKeyClear> > PredictionDelegates;

	FAbilitySystemComponentPredictionKeyClear &	GetPredictionKeyDelegate(int32 PredictionKey);
	

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	GameplayEffects	
	//	
	// ----------------------------------------------------------------------------------------------------------------

	// --------------------------------------------
	// Primary outward facing API for other systems:
	// --------------------------------------------
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec &GameplayEffect, UAbilitySystemComponent *Target, FModifierQualifier BaseQualifier = FModifierQualifier());
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
	bool HasTag(const FGameplayTag Tag);

	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	bool HasAllTags(FGameplayTagContainer &Tags);

	/** Allow events to be registered for specific gameplay tags being added or removed */
	FOnGameplayEffectTagCountChanged& RegisterGameplayTagEvent(FGameplayTag Tag);

	FOnGameplayAttributeChange& RegisterGameplayAttributeEvent(FGameplayAttribute Attribute);

	// --------------------------------------------
	// Possibly useful but not primary API functions:
	// --------------------------------------------
	
	FOnActiveGameplayEffectRemoved* OnGameplayEffectRemovedDelegate(FActiveGameplayEffectHandle Handle);

	FActiveGameplayEffectHandle ApplyGameplayEffectToTarget(UGameplayEffect *GameplayEffect, UAbilitySystemComponent *Target, float Level = FGameplayEffectLevelSpec::INVALID_LEVEL, FModifierQualifier BaseQualifier = FModifierQualifier());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToSelf"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator);
	
	FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator, FModifierQualifier BaseQualifier = FModifierQualifier() );

	int32 GetNumActiveGameplayEffect() const;

	void AddDependancyToAttribute(FGameplayAttribute Attribute, const TWeakPtr<FAggregator> InDependant);

	/** Tests if all modifiers in this GameplayEffect will leave the attribute > 0.f */
	bool CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator);

	// Generic 'Get expected magnitude (list) if I was to apply this outgoing or incoming'

	// Get duration or magnitude (list) of active effects
	//		-Get duration of CD
	//		-Get magnitude + duration of a movespeed buff

	TArray<float> GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const;

	void OnRestackGameplayEffects();

	// --------------------------------------------
	// Temp / Debug
	// --------------------------------------------

	void TEMP_ApplyActiveGameplayEffects();
	
	void PrintAllGameplayEffects() const;

	void PushGlobalCurveOveride(UCurveTable *OverrideTable)
	{
		if (OverrideTable)
		{
			GlobalCurveDataOverride.Overrides.Push(OverrideTable);
		}
	}

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	GameplayCues
	// 
	// ----------------------------------------------------------------------------------------------------------------


	UFUNCTION(BlueprintImplementableEvent, Category = GameplayCue, meta = (BlueprintInternalUseOnly = "true"))
	void BlueprintCustomHandler(EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters);

	static void DispatchBlueprintCustomHandler(AActor* Actor, UFunction* Func, EGameplayCueEvent::Type EventType, FGameplayCueParameters Parameters);

	// ----------------------------------------------------------------------------------------------------------------

	/**
	 *	GameplayAbilities
	 *	
	 *	The role of the AbilitySystemComponent wrt Abilities is to provide:
	 *		-Management of ability instances (whether per actor or per execution instance).
	 *			-Someone *has* to keep track of these instances.
	 *			-Non instanced abilities *could* be executed without any ability stuff in AbilitySystemComponent.
	 *				They should be able to operate on an GameplayAbilityActorInfo + GameplayAbility.
	 *		
	 *	As convenience it may provide some other features:
	 *		-Some basic input binding (whether instanced or non instanced abilities).
	 *		-Concepts like "this component has these abilities
	 *	
	 */

	UGameplayAbility* CreateNewInstanceOfAbility(UGameplayAbility* Ability);

	void CancelAbilitiesWithTags(const FGameplayTagContainer Tags, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, UGameplayAbility* Ignore);
	
	/** References to non-replicating abilities that we instanced. We need to keep these references to avoid GC */
	UPROPERTY()
	TArray<UGameplayAbility*>	NonReplicatedInstancedAbilities;

	/** References to replicating abilities that we instanced. We need to keep these references to avoid GC */
	UPROPERTY(Replicated)
	TArray<UGameplayAbility*>	ReplicatedInstancedAbilities;

	/**
	 *	The abilities we can activate. 
	 *		-This will include CDOs for non instanced abilities and per-execution instanced abilities. 
	 *		-Actor-instanced abilities will be the actual instance (not CDO)
	 *		
	 *	This array is not vital for things to work. It is a convenience thing for 'giving abilities to the actor'. But abilities could also work on things
	 *	without an AbilitySystemComponent. For example an ability could be written to execute on a StaticMeshActor. As long as the ability doesn't require 
	 *	instancing or anything else that the AbilitySystemComponent would provide, then it doesn't need the component to function.
	 */ 
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Abilities")
	TArray<UGameplayAbility*>	ActivatableAbilities;
	

	UFUNCTION(Server, reliable, WithValidation)
	void	ServerTryActivateAbility(class UGameplayAbility* AbilityToActivate, int32 PredictionKey);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilityFailed(class UGameplayAbility* AbilityToActivate, int32 PredictionKey);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilitySucceed(class UGameplayAbility* AbilityToActivate, int32 PredictionKey);

	// ----------------------------------------------------------------------------------------------------------------

	// This is meant to be used to inhibit activating an ability from an input perspective. (E.g., the menu is pulled up, another game mechanism is consuming all input, etc)
	// This should only be called on locally owned players.
	// This should not be used to game mechanics like silences or disables. Those should be done through gameplay effects.

	UFUNCTION(BlueprintCallable, Category="Abilities")
	bool	GetUserAbilityActivationInhibited() const;
	
	/** Disable or Enable a local user from being able to activate abilities. This should only be used for input/UI etc related inhibition. Do not use for game mechanics. */
	UFUNCTION(BlueprintCallable, Category="Abilities")
	void	SetUserAbilityActivationInhibited(bool NewInhibit);

	bool	UserAbilityActivationInhibited;

	// ----------------------------------------------------------------------------------------------------------------

	virtual void BindToInputComponent(UInputComponent *InputComponent);

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InputConfirm();

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InputCancel();

	FAbilityConfirmOrCancel	ConfirmCallbacks;
	FAbilityConfirmOrCancel	CancelCallbacks;

	FGenericAbilityDelegate AbilityActivatedCallbacks;
	FGenericAbilityDelegate AbilityCommitedCallbacks;

	void NotifyAbilityCommit(UGameplayAbility* Ability);
	void NotifyAbilityActivated(UGameplayAbility* Ability);


	UPROPERTY()
	TArray<AGameplayAbilityTargetActor*>	SpawnedTargetActors;

	/** Any active targeting actors will be told to stop and return current targeting data */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TargetConfirm();

	/** Any active targeting actors will be stopped and cancelled, not returning any targeting data */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TargetCancel();

	// ----------------------------------------------------------------------------------------------------------------

	/** This is temp for testing. We want to think a bit more on the API for outside stuff activating abilities */
	bool	ActivateAbility(TWeakObjectPtr<UGameplayAbility> Ability);

	/** There needs to be a concept of an animating ability. Only one may exist at a time. New requests can be queued up, overridden, or ignored. */
	UPROPERTY()
	UGameplayAbility*	AnimatingAbility;

	void MontageBranchPoint_AbilityDecisionStop();

	void MontageBranchPoint_AbilityDecisionStart();

	// -----------------------------------------------------------------------------

	/** Cached off data about the owning actor that abilities will need to frequently access (movement component, mesh component, anim instance, etc) */
	TSharedPtr<FGameplayAbilityActorInfo>	AbilityActorInfo;

	void InitAbilityActorInfo();

	// -----------------------------------------------------------------------------

	/**
	 *	While these appear to be state, this is actually synchronization events w/ some payload data
	 */

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedConfirm(bool Confirmed);

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedTargetData(FGameplayAbilityTargetDataHandle ReplicatedTargetData);

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedTargetDataCancelled();
	

	void SetTargetAbility(UGameplayAbility* NewTargetingAbility);

	void ConsumeAbilityConfirmCancel();

	void ConsumeAbilityTargetData();

	/** This ability has a 'lock' on replicated targeting data. Only 1 targeting ability can be active at once */
	UPROPERTY()
	UGameplayAbility* TargetingAbility;

	bool ReplicatedConfirmAbility;
	bool ReplicatedCancelAbility;

	FGameplayAbilityTargetDataHandle ReplicatedTargetData;

	/** ReplicatedTargetData was received */
	FAbilityTargetData	ReplicatedTargetDataDelegate;

	/** ReplicatedTargetData was 'cancelled' for this activation */
	FAbilityConfirmOrCancel	ReplicatedTargetDataCancelledDelegate;

private:

	// --------------------------------------------
	// Important internal functions
	// --------------------------------------------

	void ExecutePeriodicEffect(FActiveGameplayEffectHandle	Handle);

	void ExecuteGameplayEffect(const FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext);

	void CheckDurationExpired(FActiveGameplayEffectHandle Handle);

	// --------------------------------------------
	// Internal Utility / helper functions
	// --------------------------------------------

	bool AreGameplayEffectApplicationRequirementsSatisfied(const class UGameplayEffect* EffectToAdd, FGameplayEffectInstigatorContext& Instigator) const;

	bool IsOwnerActorAuthoritative() const;

	void OnAttributeGameplayEffectSpecExected(const FGameplayAttribute &Attribute, const struct FGameplayEffectSpec &Spec, struct FGameplayModifierEvaluatedData &Data);

	const FGlobalCurveDataOverride * GetCurveDataOverride() const
	{
		// only return data if we have overrides. NULL if we don't.
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

	void InvokeGameplayCueEvent(const FGameplayEffectSpec &Spec, EGameplayCueEvent::Type EventType);	
	
	// ---------------------------------------------

	

protected:

	virtual void OnRegister() override;

	UAttributeSet*	GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const;
	UAttributeSet*	GetAttributeSubobjectChecked(const TSubclassOf<UAttributeSet> AttributeClass) const;
	UAttributeSet*	GetOrCreateAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass);

	friend struct FActiveGameplayEffectsContainer;
	friend struct FActiveGameplayEffect;
};
