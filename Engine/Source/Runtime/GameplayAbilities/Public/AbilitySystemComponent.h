// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayCueInterface.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystemComponent.generated.h"

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
 *		-Provides management of instanced abilities (something must hold onto them)
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
 *		-Provides methods for getting AttributeSets
 *  
 * 
 */


/**
 *	The core ActorComponent for interfacing with the GameplayAbilities System
 */
UCLASS(ClassGroup=AbilitySystem, hidecategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class GAMEPLAYABILITIES_API UAbilitySystemComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_UCLASS_BODY()

	/** Used to register callbacks to confirm/cancel input */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityConfirmOrCancel);

	friend FGameplayEffectSpec;
	friend class AAbilitySystemDebugHUD;

	virtual ~UAbilitySystemComponent();

	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/** Checks to see if we should be active (ticking). Called after something changes that would cause us to tick or not tick. */
	void UpdateShouldTick();

	/** Finds existing AttributeSet */
	template <class T >
	const T*	GetSet()
	{
		return (T*)GetAttributeSubobject(T::StaticClass());
	}

	template <class T >
	const T*	GetSetChecked()
	{
		return (T*)GetAttributeSubobjectChecked(T::StaticClass());
	}

	/** Adds a new AttributeSet (initialized to default values) */
	template <class T >
	const T*  AddSet()
	{
		return (T*)GetOrCreateAttributeSubobject(T::StaticClass());
	}

	/** Adds a new AttributeSet that is a DSO (created by called in their CStor) */
	template <class T>
	const T*	AddDefaultSubobjectSet(T* Subobject)
	{
		SpawnedAttributes.Add(Subobject);
		return Subobject;
	}

	const UAttributeSet* InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable);

	UFUNCTION(BlueprintCallable, Category="Skills", meta=(FriendlyName="InitStats"))
	void K2_InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable);
		

	UPROPERTY(EditAnywhere, Category="AttributeTest")
	TArray<FAttributeDefaults>	DefaultStartingData;

	UPROPERTY(Replicated)
	TArray<UAttributeSet*>	SpawnedAttributes;

	void SetNumericAttribute(const FGameplayAttribute &Attribute, float NewFloatValue);
	float GetNumericAttribute(const FGameplayAttribute &Attribute);

	// -- Replication -------------------------------------------------------------------------------------------------

	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;
	
	virtual void GetSubobjectsWithStableNamesForNetworking(TArray<UObject*>& Objs) override;

	/** PredictionKeys, see more info in GameplayPrediction.h */
	UPROPERTY(ReplicatedUsing=OnRep_PredictionKey)
	FPredictionKey	ReplicatedPredictionKey;

	FPredictionKey	ScopedPedictionKey;

	UFUNCTION()
	void OnRep_PredictionKey();

	struct FPendingAbilityInfo
	{
		bool operator==(const FPendingAbilityInfo& Other) const
		{
			return PredictionKey == Other.PredictionKey	&& Handle == Other.Handle;
		}

		FPredictionKey	PredictionKey;
		FGameplayAbilitySpecHandle Handle;
	};

	// This is a list of GameplayAbilities that are predicted by the client and were triggered by abilities that were also predicted by the client
	// When the server version of the predicted ability executes it should trigger copies of these and the copies will be associated with the correct prediction keys
	TArray<FPendingAbilityInfo> PendingClientAbilities;

	enum class EAbilityExecutionState : uint8
	{
		Executing,
		Succeeded,
		Failed,
	};

	struct FExecutingAbilityInfo
	{
		FExecutingAbilityInfo() : State(EAbilityExecutionState::Executing) {};

		bool operator==(const FExecutingAbilityInfo& Other) const
		{
			return PredictionKey == Other.PredictionKey	&& State == Other.State;
		}

		FPredictionKey PredictionKey;
		EAbilityExecutionState State;
		FGameplayAbilitySpecHandle Handle;
	};

	TArray<FExecutingAbilityInfo> ExecutingServerAbilities;

	// ----------------------------------------------------------------------------------------------------------------
	//
	//	GameplayEffects	
	//	
	// ----------------------------------------------------------------------------------------------------------------

	// --------------------------------------------
	// Primary outward facing API for other systems:
	// --------------------------------------------
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec& GameplayEffect, UAbilitySystemComponent *Target, FModifierQualifier BaseQualifier = FModifierQualifier());
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToSelf(OUT FGameplayEffectSpec& GameplayEffect, FModifierQualifier BaseQualifier = FModifierQualifier());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects)
	bool RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle);

	/** Get an outgoing GameplayEffectSpec that is ready to be applied to other things. */
	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	FGameplayEffectSpecHandle GetOutgoingSpec(UGameplayEffect* GameplayEffect, float Level) const;

	/** Create an EffectContext for the owner of this AbilitySystemComponent */
	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	FGameplayEffectContextHandle GetEffectContext() const;

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

	// --------------------------------------------
	// Tags
	// --------------------------------------------
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;

	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer, bool bCountEmptyAsMatch = true) const override;

	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer, bool bCountEmptyAsMatch = true) const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	/** Allow events to be registered for specific gameplay tags being added or removed */
	FOnGameplayEffectTagCountChanged& RegisterGameplayTagEvent(FGameplayTag Tag);

	FOnGameplayAttributeChange& RegisterGameplayAttributeEvent(FGameplayAttribute Attribute);

	// --------------------------------------------
	// Possibly useful but not primary API functions:
	// --------------------------------------------
	
	FOnActiveGameplayEffectRemoved* OnGameplayEffectRemovedDelegate(FActiveGameplayEffectHandle Handle);

	FActiveGameplayEffectHandle ApplyGameplayEffectToTarget(UGameplayEffect *GameplayEffect, UAbilitySystemComponent *Target, float Level = FGameplayEffectLevelSpec::INVALID_LEVEL, FModifierQualifier BaseQualifier = FModifierQualifier());

	UFUNCTION(BlueprintCallable, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToSelf"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, FGameplayEffectContextHandle EffectContext);
	
	FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext, FModifierQualifier BaseQualifier = FModifierQualifier());

	int32 GetNumActiveGameplayEffect() const;

	void AddDependancyToAttribute(FGameplayAttribute Attribute, const TWeakPtr<FAggregator> InDependant);

	void SetBaseAttributeValueFromReplication(float NewValue, FGameplayAttribute Attribute);

	/** Tests if all modifiers in this GameplayEffect will leave the attribute > 0.f */
	bool CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext);

	// Generic 'Get expected magnitude (list) if I was to apply this outgoing or incoming'

	// Get duration or magnitude (list) of active effects
	//		-Get duration of CD
	//		-Get magnitude + duration of a movespeed buff

	TArray<float> GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const;

	TArray<float> GetActiveEffectsDuration(const FActiveGameplayEffectQuery Query) const;

	/** Removes all active effects that match given query */
	void RemoveActiveEffects(const FActiveGameplayEffectQuery Query);

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
	 

	// GameplayCues can come from GameplayEffectSpecs

	UFUNCTION(NetMulticast, unreliable)
	void NetMulticast_InvokeGameplayCueExecuted_FromSpec(const FGameplayEffectSpec Spec, FPredictionKey PredictionKey);

	void InvokeGameplayCueEvent(const FGameplayEffectSpec &Spec, EGameplayCueEvent::Type EventType);

	// GameplayCues can also come on their own. These take an optional effect context to pass through hit result, etc

	void ExecuteGameplayCue(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayEffectContextHandle EffectContext = FGameplayEffectContextHandle());

	void AddGameplayCue(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayEffectContextHandle EffectContext = FGameplayEffectContextHandle());
	
	void RemoveGameplayCue(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey);

	UFUNCTION(NetMulticast, unreliable)
	void NetMulticast_InvokeGameplayCueExecuted(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayEffectContextHandle EffectContext);

	UFUNCTION(NetMulticast, unreliable)
	void NetMulticast_InvokeGameplayCueAdded(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey, FGameplayEffectContextHandle EffectContext);

	UFUNCTION(NetMulticast, unreliable)
	void NetMulticast_InvokeGameplayCueRemoved(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey);

	void InvokeGameplayCueEvent(const FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, FGameplayEffectContextHandle EffectContext = FGameplayEffectContextHandle());

	/** Allows polling to see if a GameplayCue is active. We expect most GameplayCue handling to be event based, but some cases we may need to check if a GamepalyCue is active (Animation Blueprint for example) */
	UFUNCTION(BlueprintCallable, Category="GameplayCue", meta=(GameplayTagFilter="GameplayCue"))
	bool IsGameplayCueActive(const FGameplayTag GameplayCueTag) const;


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

	/** Grants Ability. Returns handle that can be used in TryActivateAbility, etc. */
	FGameplayAbilitySpecHandle GiveAbility(FGameplayAbilitySpec AbilitySpec);

	/** Attempts to activate the given ability */
	bool TryActivateAbility(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey = FPredictionKey(), UGameplayAbility ** OutInstancedAbility = nullptr, FOnGameplayAbilityEnded* OnGameplayAbilityEndedDelegate = nullptr);

	void TriggerAbilityFromGameplayEvent(FGameplayAbilitySpecHandle AbilityToTrigger, FGameplayAbilityActorInfo* ActorInfo, FGameplayTag Tag, FGameplayEventData* Payload, UAbilitySystemComponent& Component);

	/** Wipes all 'given' abilities. */
	void ClearAllAbilities();

	/** Removes the specified ability */
	void ClearAbility(const FGameplayAbilitySpecHandle& Handle);

	/** Will be called from GiveAbility or from OnRep. Initializes events (triggers and inputs) with the given ability */
	void OnGiveAbility(const FGameplayAbilitySpec AbilitySpec);

	UGameplayAbility* CreateNewInstanceOfAbility(FGameplayAbilitySpec& Spec, UGameplayAbility* Ability);

	void CancelAbilities(const FGameplayTagContainer* WithTags=nullptr, const FGameplayTagContainer* WithoutTags=nullptr, UGameplayAbility* Ignore=nullptr);

	void BlockAbilitiesWithTags(const FGameplayTagContainer Tags);
	void UnBlockAbilitiesWithTags(const FGameplayTagContainer Tags);

	void BlockAbilityByInputID(int32 InputID);
	void UnBlockAbilityByInputID(int32 InputID);
	
	/** FUll list of all instance-per-execution gameplay abilities associated with this component */
	UPROPERTY()
	TArray<UGameplayAbility*>	AllReplicatedInstancedAbilities;

	void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability);

	/**
	 *	The abilities we can activate. 
	 *		-This will include CDOs for non instanced abilities and per-execution instanced abilities. 
	 *		-Actor-instanced abilities will be the actual instance (not CDO)
	 *		
	 *	This array is not vital for things to work. It is a convenience thing for 'giving abilities to the actor'. But abilities could also work on things
	 *	without an AbilitySystemComponent. For example an ability could be written to execute on a StaticMeshActor. As long as the ability doesn't require 
	 *	instancing or anything else that the AbilitySystemComponent would provide, then it doesn't need the component to function.
	 */

	UPROPERTY(ReplicatedUsing=OnRep_ActivateAbilities, BlueprintReadOnly, Category = "Abilities")
	TArray<FGameplayAbilitySpec>	ActivatableAbilities;

	FGameplayAbilitySpec* FindAbilitySpecFromHandle(FGameplayAbilitySpecHandle Handle);

	FGameplayAbilitySpec* FindAbilitySpecFromInputID(int32 InputID);
	
	UFUNCTION()
	void	OnRep_ActivateAbilities();

	UFUNCTION(Server, reliable, WithValidation)
	void	ServerTryActivateAbility(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey PredictionKey);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilityFailed(FGameplayAbilitySpecHandle AbilityToActivate, int16 PredictionKey);

	void	OnClientActivateAbilityFailed(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey::KeyType PredictionKey);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilitySucceed(FGameplayAbilitySpecHandle AbilityToActivate,int16 PredictionKey);
	

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

	virtual void BindToInputComponent(UInputComponent* InputComponent);
	
	virtual void BindAbilityActivationToInputComponent(UInputComponent* InputComponent, FGameplayAbiliyInputBinds BindInfo);

	void AbilityInputPressed(int32 InputID);

	void AbilityInputReleased(int32 InputID);

	void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec);

	/** Sent by abilities when they *need* to tell server when activation input is released. (Not sent by defauly only sent when using AbilityTask_WaitInputRelease) */
	UFUNCTION(Server, reliable, WithValidation)
	void ServerInputRelease(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey ScopedPedictionKey);

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InputConfirm();

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InputCancel();

	FAbilityConfirmOrCancel	ConfirmCallbacks;
	FAbilityConfirmOrCancel	CancelCallbacks;

	FGenericAbilityDelegate AbilityActivatedCallbacks;
	FGenericAbilityDelegate AbilityCommitedCallbacks;

	void HandleGameplayEvent(FGameplayTag EventTag, FGameplayEventData* Payload);

	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle > > GameplayEventTriggeredAbilities;

	void NotifyAbilityCommit(UGameplayAbility* Ability);
	void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability);


	UPROPERTY()
	TArray<AGameplayAbilityTargetActor*>	SpawnedTargetActors;

	/** Any active targeting actors will be told to stop and return current targeting data */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TargetConfirm();

	/** Any active targeting actors will be stopped and canceled, not returning any targeting data */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void TargetCancel();

	// ----------------------------------------------------------------------------------------------------------------

	/** Adds a UAbilityTask task to the list of tasks to be ticked */
	void TaskStarted(UAbilityTask* NewTask);

	/** Removes a UAbilityTask task from the list of tasks to be ticked */
	void TaskEnded(UAbilityTask* Task);

	// ----------------------------------------------------------------------------------------------------------------
	//	AnimMontage Support
	//	
	//	TODO:
	//	-Continously update RepAnimMontageInfo on server for join in progress clients.
	//	-Some missing functionality may still be needed (GetCurrentSectionTime, SetPlayRate, etc)	
	// ----------------------------------------------------------------------------------------------------------------	

	/** Plays a montage and handles replication and prediction based on passed in ability/activation info */
	float PlayMontage(UGameplayAbility* AnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None);

	/** Plays a montage without updating replication/prediction structures. Used by simulated proxies when replication tells them to play a montage. */
	float PlayMontageSimulated(UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None);

	/** Stops whatever montage is currently playing. Expectation is caller should only be stoping it if they are the current animating ability (or have good reason not to check) */
	void CurrentMontageStop();

	/** Jumps current montage to given section. Expectation is caller should only be stoping it if they are the current animating ability (or have good reason not to check) */
	void CurrentMontageJumpToSection(FName SectionName);

	/** Sets current montages next section name. Expectation is caller should only be stoping it if they are the current animating ability (or have good reason not to check) */
	void CurrentMontageSetNextSectionName(FName FromSectionName, FName ToSectionName);

	/** Returns true if the passed in ability is the current animating ability */
	bool IsAnimatingAbility(UGameplayAbility* Ability) const;

	/** Returns the current animating ability */
	UGameplayAbility* GetAnimatingAbility();

	/** Returns montage that is currently playing */
	UAnimMontage* GetCurrentMontage() const;

	/** Get SectionID of currently playing AnimMontage */
	int32 GetCurrentMontageSectionID() const;

	/** Get SectionName of currently playing AnimMontage */
	FName GetCurrentMontageSectionName() const;

	/** Get length in time of current section */
	float GetCurrentMontageSectionLength() const;

	/** Returns amount of time left in current section */
	float GetCurrentMontageSectionTimeLeft() const;

protected:

	/** Called when a prediction key that played a montage is rejected */
	void OnPredictiveMontageRejected(UAnimMontage* PredictiveMontage);

	/** Copy LocalAnimMontageInfo into RepAnimMontageInfo */
	void AnimMontage_UpdateReplicatedData();

	/** Data structure for replicating montage info to simulated clients */
	UPROPERTY(ReplicatedUsing=OnRep_ReplicatedAnimMontage)
	FGameplayAbilityRepAnimMontage RepAnimMontageInfo;

	/** Data structure for montages that were instigated locally (everything if server, predictive if client. replicated if simulated proxy) */
	UPROPERTY()
	FGameplayAbilityLocalAnimMontage LocalAnimMontageInfo;

	UFUNCTION()
	void OnRep_ReplicatedAnimMontage();

	/** RPC function called from CurrentMontageSetNextSectopnName, replicates to other clients */
	UFUNCTION(reliable, server, WithValidation)
	void ServerCurrentMontageSetNextSectionName(UAnimMontage* ClientAnimMontage, float ClientPosition, FName SectionName, FName NextSectionName);

	/** RPC function called from CurrentMontageJumpToSection, replicates to other clients */
	UFUNCTION(reliable, server, WithValidation)
	void ServerCurrentMontageJumpToSectionName(UAnimMontage* ClientAnimMontage, FName SectionName);

	// -----------------------------------------------------------------------------
public:

	/** The actor that owns this component logically */
	UPROPERTY(ReplicatedUsing = OnRep_OwningActor)
	AActor* OwnerActor;

	/** The actor that is the physical representation used for abilities. Can be NULL */
	UPROPERTY(ReplicatedUsing = OnRep_OwningActor)
	AActor* AvatarActor;
	
	UFUNCTION()
	void OnRep_OwningActor();

	/** Cached off data about the owning actor that abilities will need to frequently access (movement component, mesh component, anim instance, etc) */
	TSharedPtr<FGameplayAbilityActorInfo>	AbilityActorInfo;

	/**
	 *	Initialized the Abilities' ActorInfo - the structure that holds information about who we are acting on and who controls us.
	 *      OwnerActor is the actor that logically owns this component.
	 *		AvatarActor is what physical actor in the world we are acting on. Usually a Pawn but it could be a Tower, Building, Turret, etc, may be the same as Owner
	 */
	void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor);

	/** Changes the avatar actor, leaves the owner actor the same */
	void SetAvatarActor(AActor* InAvatarActor);

	/**
	* This is called when the actor that is initialized to this system dies, this will clear that actor from this system and FGameplayAbilityActorInfo
	*/
	void ClearActorInfo();

	/**
	 *	This will refresh the Ability's ActorInfo structure based on the current ActorInfo. That is, AvatarActor will be the same but we will look for new
	 *	AnimInstance, MovementComponent, PlayerController, etc.
	 */	
	void RefreshAbilityActorInfo();

	// -----------------------------------------------------------------------------

	/**
	 *	While these appear to be state, these are actually synchronization events w/ some payload data
	 */

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedConfirm(bool Confirmed, FPredictionKey PredictionKey);

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedTargetData(FGameplayAbilityTargetDataHandle ReplicatedTargetData, FPredictionKey PredictionKey);

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedTargetDataCancelled();

	void ConsumeAbilityConfirmCancel();

	void ConsumeAbilityTargetData();

	bool ReplicatedConfirmAbility;
	bool ReplicatedCancelAbility;

	FGameplayAbilityTargetDataHandle ReplicatedTargetData;

	/** ReplicatedTargetData was received */
	FAbilityTargetData	ReplicatedTargetDataDelegate;

	/** ReplicatedTargetData was 'cancelled' for this activation */
	FAbilityConfirmOrCancel	ReplicatedTargetDataCancelledDelegate;

	/** Tasks that run on simulated proxies */
	UPROPERTY(ReplicatedUsing=OnRep_SimulatedTasks)
	TArray<UAbilityTask*> SimulatedTasks;

	UFUNCTION()
	void OnRep_SimulatedTasks();

private:

	bool HasNetworkAuthorityToApplyGameplayEffect(const FModifierQualifier QualifierContext) const;

	void ExecutePeriodicEffect(FActiveGameplayEffectHandle	Handle);

	void ExecuteGameplayEffect(FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext);

	void CheckDurationExpired(FActiveGameplayEffectHandle Handle);

	bool AreGameplayEffectApplicationRequirementsSatisfied(const class UGameplayEffect* EffectToAdd, const FGameplayEffectContextHandle& EffectContext) const;

	bool IsOwnerActorAuthoritative() const;

	void OnAttributeGameplayEffectSpecExected(const FGameplayAttribute &Attribute, const struct FGameplayEffectSpec &Spec, struct FGameplayModifierEvaluatedData &Data);

	const FGlobalCurveDataOverride* GetCurveDataOverride() const
	{
		// only return data if we have overrides. NULL if we don't.
		return (GlobalCurveDataOverride.Overrides.Num() > 0 ? &GlobalCurveDataOverride : NULL);
	}

	FGlobalCurveDataOverride	GlobalCurveDataOverride;	

	// --------------------------------------------
	
	UPROPERTY(ReplicatedUsing=OnRep_GameplayEffects)
	FActiveGameplayEffectsContainer	ActiveGameplayEffects;

	UPROPERTY(ReplicatedUsing=OnRep_GameplayEffects)
	FActiveGameplayCueContainer	ActiveGameplayCues;

	/** Abilities with these tags are not able to be activated */
	FGameplayTagCountContainer BlockedAbilityTags;

	/** Tracks abilities that are blocked based on input binding. An ability is blocked if BlockedAbilityBindings[InputID] > 0 */
	UPROPERTY(Transient, Replicated)
	TArray<uint8> BlockedAbilityBindings;

	UFUNCTION()
	void OnRep_GameplayEffects();

	// ---------------------------------------------
	
	
	
	// ---------------------------------------------

	/** Array of currently active UAbilityTasks that require ticking */
	TArray<TWeakObjectPtr<UAbilityTask> >	TickingTasks;	

protected:

	virtual void OnRegister() override;

	const UAttributeSet*	GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const;
	const UAttributeSet*	GetAttributeSubobjectChecked(const TSubclassOf<UAttributeSet> AttributeClass) const;
	const UAttributeSet*	GetOrCreateAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass);

	friend struct FActiveGameplayEffectsContainer;
	friend struct FActiveGameplayEffect;
};
