// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.
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


DECLARE_MULTICAST_DELEGATE_OneParam(FTargetingRejectedConfirmation, int32);

/**
 *	The core ActorComponent for interfacing with the GameplayAbilities System
 */
UCLASS(ClassGroup=AbilitySystem, hidecategories=(Object,LOD,Lighting,Transform,Sockets,TextureStreaming), editinlinenew, meta=(BlueprintSpawnableComponent))
class GAMEPLAYABILITIES_API UAbilitySystemComponent : public UActorComponent, public IGameplayTagAssetInterface
{
	GENERATED_UCLASS_BODY()

	/** Used to register callbacks to ability-key input */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAbilityAbilityKey, /*UGameplayAbility*, Ability, */int32, InputID);

	/** Used to register callbacks to confirm/cancel input */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FAbilityConfirmOrCancel);

	friend struct FActiveGameplayEffectAction_Add;
	friend FGameplayEffectSpec;
	friend class AAbilitySystemDebugHUD;

	virtual ~UAbilitySystemComponent();

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void OnComponentDestroyed() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction) override;

	/** Checks to see if we should be active (ticking). Called after something changes that would cause us to tick or not tick. */
	void UpdateShouldTick();

	/** Finds existing AttributeSet */
	template <class T >
	const T*	GetSet() const
	{
		return (T*)GetAttributeSubobject(T::StaticClass());
	}

	/** Finds existing AttributeSet. Asserts if it isn't there. */
	template <class T >
	const T*	GetSetChecked() const
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

	/**
	* Does this ability system component have this attribute?
	*
	* @param Attribute	Handle of the gameplay effect to retrieve target tags from
	*
	* @return true if Attribute is valid and this ability system component contains an attribute set that contains Attribute. Returns false otherwise.
	*/
	bool HasAttributeSetForAttribute(FGameplayAttribute Attribute) const;

	const UAttributeSet* InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable);

	UFUNCTION(BlueprintCallable, Category="Skills", meta=(FriendlyName="InitStats"))
	void K2_InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable);
		

	UPROPERTY(EditAnywhere, Category="AttributeTest")
	TArray<FAttributeDefaults>	DefaultStartingData;

	UPROPERTY(Replicated)
	TArray<UAttributeSet*>	SpawnedAttributes;

	/** Sets the base value of an attribute. Existing active modifiers are NOT cleared and will act upon the new base value. */
	void SetNumericAttributeBase(const FGameplayAttribute &Attribute, float NewBaseValue);

	/**
	 *	Applies an inplace mod to the given attribute. This correctly update the attribute's aggregator, updates the attribute set property,
	 *	and invokes the OnDirty callbacks.
	 *	
	 *	This does not invoke Pre/PostGameplayEffectExecute calls on the attribute set. This does no tag checking, application requirements, immunity, etc.
	 *	No GameplayEffectSpec is created or is applied!
	 *
	 *	This should only be used in cases where applying a real GameplayEffectSpec is too slow or not possible.
	 */
	void ApplyModToAttribute(const FGameplayAttribute &Attribute, TEnumAsByte<EGameplayModOp::Type> ModifierOp, float ModifierMagnitude);

	/** Returns current (final) value of an attribute */
	float GetNumericAttribute(const FGameplayAttribute &Attribute);

	virtual void DisplayDebug(class UCanvas* Canvas, const class FDebugDisplayInfo& DebugDisplay, float& YL, float& YPos);

	// -- Replication -------------------------------------------------------------------------------------------------

	virtual bool ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags) override;
	
	/** Force owning actor to update it's replication, to make sure that gameplay cues get sent down quickly. Override to change how aggressive this is */
	virtual void ForceReplication();

	virtual void GetSubobjectsWithStableNamesForNetworking(TArray<UObject*>& Objs) override;

	/** PredictionKeys, see more info in GameplayPrediction.h */
	UPROPERTY(ReplicatedUsing=OnRep_PredictionKey)
	FPredictionKey	ReplicatedPredictionKey;

	FPredictionKey	ScopedPredictionKey;

	FPredictionKey GetPredictionKeyForNewAction() const
	{
		return ScopedPredictionKey.IsValidForMorePrediction() ? ScopedPredictionKey : FPredictionKey();
	}

	/** Do we have a valid prediction key to do more predictive actions with */
	bool CanPredict() const
	{
		return ScopedPredictionKey.IsValidForMorePrediction();
	}

	bool HasAuthorityOrPredictionKey(const FGameplayAbilityActivationInfo* ActivationInfo) const;	

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
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec& GameplayEffect, UAbilitySystemComponent *Target, FPredictionKey PredictionKey=FPredictionKey());
	FActiveGameplayEffectHandle ApplyGameplayEffectSpecToSelf(OUT FGameplayEffectSpec& GameplayEffect, FPredictionKey PredictionKey = FPredictionKey());

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameplayEffects)
	bool RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle);

	/** Get an outgoing GameplayEffectSpec that is ready to be applied to other things. */
	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	FGameplayEffectSpecHandle MakeOutgoingSpec(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level, FGameplayEffectContextHandle Context) const;

	/** Get an outgoing GameplayEffectSpec that is ready to be applied to other things. */
	UFUNCTION(BlueprintCallable, Category = GameplayEffects, meta=(DeprecatedFunction, DeprecationMessage = "Use new MakeOutgoingSpec"))
	FGameplayEffectSpecHandle GetOutgoingSpec(const UGameplayEffect* GameplayEffect, float Level = 1.f) const;

	/** Overloaded version that allows Context to be specified */
	FGameplayEffectSpecHandle GetOutgoingSpec(const UGameplayEffect* GameplayEffect, float Level, FGameplayEffectContextHandle Context) const;

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

	/**
	 * Get the source tags from the gameplay spec represented by the specified handle, if possible
	 * 
	 * @param Handle	Handle of the gameplay effect to retrieve source tags from
	 * 
	 * @return Source tags from the gameplay spec represented by the handle, if possible
	 */
	const FGameplayTagContainer* GetGameplayEffectSourceTagsFromHandle(FActiveGameplayEffectHandle Handle) const;

	/**
	 * Get the target tags from the gameplay spec represented by the specified handle, if possible
	 * 
	 * @param Handle	Handle of the gameplay effect to retrieve target tags from
	 * 
	 * @return Target tags from the gameplay spec represented by the handle, if possible
	 */
	const FGameplayTagContainer* GetGameplayEffectTargetTagsFromHandle(FActiveGameplayEffectHandle Handle) const;

	/**
	 * Populate the specified capture spec with the data necessary to capture an attribute from the component
	 * 
	 * @param OutCaptureSpec	[OUT] Capture spec to populate with captured data
	 */
	void CaptureAttributeForGameplayEffect(OUT FGameplayEffectAttributeCaptureSpec& OutCaptureSpec);
	
	// --------------------------------------------
	// Callbacks / Notifies
	// (these need to be at the UObject level so we can safetly bind, rather than binding to raw at the ActiveGameplayEffect/Container level which is unsafe if the AbilitySystemComponent were killed).
	// --------------------------------------------

	void OnAttributeAggregatorDirty(FAggregator* Aggregator, FGameplayAttribute Attribute);

	void OnMagnitudeDependancyChange(FActiveGameplayEffectHandle Handle, const FAggregator* ChangedAggregator);

	/** This ASC has successfully applied a GE to something (potentially itself) */
	void OnGameplayEffectAppliedToTarget(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);

	void OnGameplayEffectAppliedToSelf(UAbilitySystemComponent* Source, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);

	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnGameplayEffectAppliedDelegate, UAbilitySystemComponent*, const FGameplayEffectSpec&, FActiveGameplayEffectHandle);

	FOnGameplayEffectAppliedDelegate OnGameplayEffectAppliedDelegateToSelf;

	FOnGameplayEffectAppliedDelegate OnGameplayEffectAppliedDelegateToTarget;

	// --------------------------------------------
	// Tags
	// --------------------------------------------
	virtual bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const override;

	virtual bool HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer, bool bCountEmptyAsMatch = true) const override;

	virtual bool HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer, bool bCountEmptyAsMatch = true) const override;

	virtual void GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const override;

	/** 	 
	 *  Allows GameCode to add loose gameplaytags which are not backed by a GameplayEffect. 
	 *
	 *	Tags added this way are not replicated! 
	 *	
	 *	It is up to the calling GameCode to make sure these tags are added on clients/server where necessary
	 */

	void AddLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count=1);

	void AddLooseGameplayTags(const FGameplayTagContainer& GameplayTag, int32 Count = 1);

	void RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	void RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTag, int32 Count = 1);

	/** Allow events to be registered for specific gameplay tags being added or removed */
	FOnGameplayEffectTagCountChanged& RegisterGameplayTagEvent(FGameplayTag Tag);

	/** Returns multicast delegate that is invoked whenever a tag is added or removed (but not if just count is increased. Only for 'new' and 'removed' events) */
	FOnGameplayEffectTagCountChanged& RegisterGenericGameplayTagEvent();

	FOnGameplayAttributeChange& RegisterGameplayAttributeEvent(FGameplayAttribute Attribute);

	// --------------------------------------------
	// System Attributes
	// --------------------------------------------
	
	UPROPERTY(meta=(SystemGameplayAttribute="true"))
	float OutgoingDuration;

	UPROPERTY(meta = (SystemGameplayAttribute = "true"))
	float IncomingDuration;

	static UProperty* GetOutgoingDurationProperty();
	static UProperty* GetIncomingDurationProperty();

	static const FGameplayEffectAttributeCaptureDefinition& GetOutgoingDurationCapture();
	static const FGameplayEffectAttributeCaptureDefinition& GetIncomingDurationCapture();


	// --------------------------------------------
	// Additional Helper Functions
	// --------------------------------------------
	
	FOnActiveGameplayEffectRemoved* OnGameplayEffectRemovedDelegate(FActiveGameplayEffectHandle Handle);

	UFUNCTION(BlueprintCallable, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToTarget"))
	FActiveGameplayEffectHandle BP_ApplyGameplayEffectToTarget(TSubclassOf<UGameplayEffect> GameplayEffectClass, UAbilitySystemComponent *Target, float Level, FGameplayEffectContextHandle Context);

	UFUNCTION(BlueprintCallable, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToTarget"), meta=(DeprecatedFunction, DeprecationMessage = "Use new ApplyGameplayEffectToSelf"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToTarget(UGameplayEffect *GameplayEffect, UAbilitySystemComponent *Target, float Level, FGameplayEffectContextHandle Context);

	FActiveGameplayEffectHandle ApplyGameplayEffectToTarget(UGameplayEffect *GameplayEffect, UAbilitySystemComponent *Target, float Level = UGameplayEffect::INVALID_LEVEL, FGameplayEffectContextHandle Context = FGameplayEffectContextHandle(), FPredictionKey PredictionKey = FPredictionKey());

	UFUNCTION(BlueprintCallable, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToSelf"))
	FActiveGameplayEffectHandle BP_ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float Level, FGameplayEffectContextHandle EffectContext);

	UFUNCTION(BlueprintCallable, Category = GameplayEffects, meta=(FriendlyName = "ApplyGameplayEffectToSelf"), meta=(DeprecatedFunction, DeprecationMessage = "Use new ApplyGameplayEffectToSelf"))
	FActiveGameplayEffectHandle K2_ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, FGameplayEffectContextHandle EffectContext);
	
	FActiveGameplayEffectHandle ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext, FPredictionKey PredictionKey = FPredictionKey());

	// Returns the number of gameplay effects that are currently active on this ability system component
	int32 GetNumActiveGameplayEffects() const;

	void SetBaseAttributeValueFromReplication(float NewValue, FGameplayAttribute Attribute);

	/** Tests if all modifiers in this GameplayEffect will leave the attribute > 0.f */
	bool CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext);

	// Generic 'Get expected magnitude (list) if I was to apply this outgoing or incoming'

	// Get duration or magnitude (list) of active effects
	//		-Get duration of CD
	//		-Get magnitude + duration of a movespeed buff

	TArray<float> GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const;

	TArray<float> GetActiveEffectsDuration(const FActiveGameplayEffectQuery Query) const;

	UFUNCTION(BlueprintCallable, Category = GameplayEffects)
	void RemoveActiveEffectsWithTags(FGameplayTagContainer Tags);

	/** Removes all active effects that match given query */
	void RemoveActiveEffects(const FActiveGameplayEffectQuery Query);

	void OnRestackGameplayEffects();	
	
	void PrintAllGameplayEffects() const;

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

	void ExecuteGameplayCue(const FGameplayTag GameplayCueTag, FGameplayEffectContextHandle EffectContext = FGameplayEffectContextHandle());

	void AddGameplayCue(const FGameplayTag GameplayCueTag, FGameplayEffectContextHandle EffectContext = FGameplayEffectContextHandle());
	
	void RemoveGameplayCue(const FGameplayTag GameplayCueTag);

	/** Removes any GameplayCue added on its own, i.e. not as part of a GameplayEffect. */
	void RemoveAllGameplayCues();

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

	void GetActivateableGameplayAbilitySpecsByAllMatchingTags(const FGameplayTagContainer& GameplayTagContainer, TArray < struct FGameplayAbilitySpec* >& MatchingGameplayAbilities) const;

	/** Attempts to activate a gameplay ability that matches the given tag. Returns true if anything is activated */
	UFUNCTION(BlueprintCallable, Category = "Abilities")
	bool TryActivateAbilityByTag(const FGameplayTagContainer& GameplayTagContainer);

	/** Attempts to activate the given ability */
	bool TryActivateAbility(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey InPredictionKey = FPredictionKey(), UGameplayAbility ** OutInstancedAbility = nullptr, FOnGameplayAbilityEnded* OnGameplayAbilityEndedDelegate = nullptr, const FGameplayEventData* TriggerEventData = nullptr);

	bool TriggerAbilityFromGameplayEvent(FGameplayAbilitySpecHandle AbilityToTrigger, FGameplayAbilityActorInfo* ActorInfo, FGameplayTag Tag, const FGameplayEventData* Payload, UAbilitySystemComponent& Component);

	/** Wipes all 'given' abilities. */
	void ClearAllAbilities();

	/** Removes the specified ability */
	void ClearAbility(const FGameplayAbilitySpecHandle& Handle);

	/** Cancel all abilities with the specified tags. Will not cancel the Ignore instance */
	void CancelAbilities(const FGameplayTagContainer* WithTags=nullptr, const FGameplayTagContainer* WithoutTags=nullptr, UGameplayAbility* Ignore=nullptr);

	/** 
	 * Called from ability activation or native code, will apply the correct ability blocking tags and cancel existing abilities. Subclasses can override the behavior 
	 * @param AbilityTags The tags of the ability that has block and cancel flags
	 * @param RequestingAbility The gameplay ability requesting the change, can be NULL for native events
	 * @param bEnableBlockTags If true will enable the block tags, if false will disable the block tags
	 * @param BlockTags What tags to block
	 * @param bExecuteCancelTags If true will cancel abilities matching tags
	 * @param CancelTags what tags to cancel
	 */
	virtual void ApplyAbilityBlockAndCancelTags(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bEnableBlockTags, const FGameplayTagContainer& BlockTags, bool bExecuteCancelTags, const FGameplayTagContainer& CancelTags);

	/** Called when an ability is cancellable or not. Doesn't do anything by default, can be overridden to tie into gameplay events */
	virtual void HandleChangeAbilityCanBeCanceled(const FGameplayTagContainer& AbilityTags, UGameplayAbility* RequestingAbility, bool bCanBeCanceled) {}

	/** Returns true if any passed in tags are blocked */
	bool AreAbilityTagsBlocked(const FGameplayTagContainer& Tags) const;

	void BlockAbilitiesWithTags(const FGameplayTagContainer& Tags);
	void UnBlockAbilitiesWithTags(const FGameplayTagContainer& Tags);

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
protected:
	UPROPERTY(ReplicatedUsing=OnRep_ActivateAbilities, BlueprintReadOnly, Category = "Abilities")
	FGameplayAbilitySpecContainer	ActivatableAbilities;

	/** Will be called from GiveAbility or from OnRep. Initializes events (triggers and inputs) with the given ability */
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec);

	/** Will be called from RemoveAbility or from OnRep. Unbinds inputs with the given ability */
	virtual void OnRemoveAbility(FGameplayAbilitySpec& AbilitySpec);

	/** Called from ClearAbility, ClearAllAbilities or OnRep. Clears any triggers that should no longer exist. */
	void CheckForClearedAbilities();

	/** Cancel a specific ability spec */
	void CancelAbilitySpec(FGameplayAbilitySpec& Spec, UGameplayAbility* Ignore);

	/** Creates a new instance of an ability, storing it in the spec */
	UGameplayAbility* CreateNewInstanceOfAbility(FGameplayAbilitySpec& Spec, const UGameplayAbility* Ability);

public:

	/** Returns the list of all activatable abilities */
	const TArray<FGameplayAbilitySpec>& GetActivatableAbilities();

	/** Returns an ability spec from a handle. If modifying call MarkAbilitySpecDirty */
	FGameplayAbilitySpec* FindAbilitySpecFromHandle(FGameplayAbilitySpecHandle Handle);

	/** Returns an ability spec from a handle. If modifying call MarkAbilitySpecDirty */
	FGameplayAbilitySpec* FindAbilitySpecFromInputID(int32 InputID);

	/** Call to mark that an ability spec has been modified */
	void MarkAbilitySpecDirty(FGameplayAbilitySpec& Spec);
	
	UFUNCTION()
	void	OnRep_ActivateAbilities();

	UFUNCTION(Server, reliable, WithValidation)
	void	ServerTryActivateAbility(FGameplayAbilitySpecHandle AbilityToActivate, bool InputPressed, FPredictionKey PredictionKey);

	UFUNCTION(Server, reliable, WithValidation)
	void	ServerTryActivateAbilityWithEventData(FGameplayAbilitySpecHandle AbilityToActivate, bool InputPressed, FPredictionKey PredictionKey, FGameplayEventData TriggerEventData);

	/** Replicate that an ability has ended, to the client or server as appropriate */
	void	ReplicateEndAbility(FGameplayAbilitySpecHandle Handle, FGameplayAbilityActivationInfo ActivationInfo, UGameplayAbility* Ability);

	/** Called by ServerEndAbility and ClientEndAbility; avoids code duplication. */
	void	RemoteEndAbility(FGameplayAbilitySpecHandle AbilityToEnd, FGameplayAbilityActivationInfo ActivationInfo);

	UFUNCTION(Server, reliable, WithValidation)
	void	ServerEndAbility(FGameplayAbilitySpecHandle AbilityToEnd, FGameplayAbilityActivationInfo ActivationInfo);

	UFUNCTION(Client, reliable)
	void	ClientEndAbility(FGameplayAbilitySpecHandle AbilityToEnd, FGameplayAbilityActivationInfo ActivationInfo);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilityFailed(FGameplayAbilitySpecHandle AbilityToActivate, int16 PredictionKey);

	void	OnClientActivateAbilityFailed(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey::KeyType PredictionKey);

	UFUNCTION(Client, Reliable)
	void	ClientActivateAbilitySucceed(FGameplayAbilitySpecHandle AbilityToActivate, int16 PredictionKey, FGameplayEventData TriggerEventData);

	/** Attempted to confirm targeting, but the targeting actor rejected it. */
	UFUNCTION(Client, Unreliable)
	void	ClientAbilityNotifyRejected(int32 InputID);


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

	void AbilitySpecInputPressed(FGameplayAbilitySpec& Spec);

	void AbilitySpecInputReleased(FGameplayAbilitySpec& Spec);

	/** Sent by abilities to tell server when activation input is pressed. (Sent by default in order not to short-circuit WaitInputPress/WaitInputRelease tasks) */
	UFUNCTION(Server, reliable, WithValidation)
	void ServerInputPress(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey ScopedPedictionKey);

	/** Sent by abilities to tell server when activation input is released. (Sent by default in order not to short-circuit WaitInputPress/WaitInputRelease tasks) */
	UFUNCTION(Server, reliable, WithValidation)
	void ServerInputRelease(FGameplayAbilitySpecHandle AbilityToActivate, FPredictionKey ScopedPedictionKey);

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InputConfirm();

	UFUNCTION(BlueprintCallable, Category="Abilities")
	void InputCancel();

	FAbilityAbilityKey	AbilityKeyPressCallbacks;
	FAbilityAbilityKey	AbilityKeyReleaseCallbacks;
	FAbilityConfirmOrCancel	ConfirmCallbacks;
	FAbilityConfirmOrCancel	CancelCallbacks;

	FGenericAbilityDelegate AbilityActivatedCallbacks;
	FGenericAbilityDelegate AbilityCommitedCallbacks;

	/** Executes a gameplay event. Returns the number of successful ability activations triggered by the event */
	int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

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

	/** Clear the animating ability that is passed in, if it's still currently animating */
	void ClearAnimatingAbility(UGameplayAbility* Ability);

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

	/** Implementation of ServerTryActivateAbility */
	virtual void InternalServerTryActiveAbility(FGameplayAbilitySpecHandle AbilityToActivate, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData);

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

	/** Abilities that are triggered from a gameplay event */
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle > > GameplayEventTriggeredAbilities;

	/** Abilities that are triggered from a tag being added to the owner */
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle > > OwnedTagTriggeredAbilities;

	/** Callback that is called when an owned tag bound to an ability changes */
	virtual void MonitoredTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** Returns true if the specified ability should be activated from an event in this network mode */
	bool HasNetworkAuthorityToActivateTriggeredAbility(const FGameplayAbilitySpec &Spec) const;

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
	void ServerSetReplicatedAbilityKeyState(int32 InputID, bool Pressed, FPredictionKey PredictionKey);

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedConfirm(bool Confirmed, FPredictionKey PredictionKey);

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedTargetData(FGameplayAbilityTargetDataHandle ReplicatedTargetData, FPredictionKey PredictionKey);

	UFUNCTION(Server, reliable, WithValidation)
	void ServerSetReplicatedTargetDataCancelled();

	void ConsumeAbilityConfirmCancel();

	void ConsumeAbilityTargetData();

	//Note: Ability key state is stored on the ability spec, not here on the ASC with confirm/cancel.
	bool ReplicatedConfirmAbility;
	bool ReplicatedCancelAbility;

	FGameplayAbilityTargetDataHandle ReplicatedTargetData;

	/** ReplicatedTargetData was received */
	FAbilityTargetData	ReplicatedTargetDataDelegate;

	/** ReplicatedTargetData was 'cancelled' for this activation */
	FAbilityConfirmOrCancel	ReplicatedTargetDataCancelledDelegate;

	/** Targeting actor rejected a confirmation attempt */
	FTargetingRejectedConfirmation TargetingRejectedConfirmationDelegate;

	/** Tasks that run on simulated proxies */
	UPROPERTY(ReplicatedUsing=OnRep_SimulatedTasks)
	TArray<UAbilityTask*> SimulatedTasks;

	UFUNCTION()
	void OnRep_SimulatedTasks();

protected:

	/** Actually pushes the final attribute value to the attribute set's property. Should not be called by outside code since this does not go through the attribute aggregator system. */
	void SetNumericAttribute_Internal(const FGameplayAttribute &Attribute, float NewFloatValue);

	bool HasNetworkAuthorityToApplyGameplayEffect(FPredictionKey PredictionKey) const;

	void ExecutePeriodicEffect(FActiveGameplayEffectHandle	Handle);

	void ExecuteGameplayEffect(FGameplayEffectSpec &Spec, FPredictionKey PredictionKey);

	void CheckDurationExpired(FActiveGameplayEffectHandle Handle);

	bool IsOwnerActorAuthoritative() const;

	void OnAttributeGameplayEffectSpecExected(const FGameplayAttribute &Attribute, const struct FGameplayEffectSpec &Spec, struct FGameplayModifierEvaluatedData &Data);

	// --------------------------------------------
	
	// Contains all of the gameplay effects that are currently active on this component
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
	
	// Acceleration map for all gameplay tags (OwnedGameplayTags from GEs and explicit GameplayCueTags)
	FGameplayTagCountContainer GameplayTagCountContainer;

	void UpdateTagMap(const FGameplayTag& BaseTag, int32 CountDelta);
	
	void UpdateTagMap(const FGameplayTagContainer& Container, int32 CountDelta);
	
	// ---------------------------------------------

	/** Array of currently active UAbilityTasks that require ticking */
	TArray<TWeakObjectPtr<UAbilityTask> >	TickingTasks;	

protected:

	virtual void OnRegister() override;

	const UAttributeSet*	GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const;
	const UAttributeSet*	GetAttributeSubobjectChecked(const TSubclassOf<UAttributeSet> AttributeClass) const;
	const UAttributeSet*	GetOrCreateAttributeSubobject(TSubclassOf<UAttributeSet> AttributeClass);

	friend struct FActiveGameplayEffect;
	friend struct FActiveGameplayEffectAction;
	friend struct FActiveGameplayEffectsContainer;
	friend struct FActiveGameplayCue;
	friend struct FActiveGameplayCueContainer;
	friend struct FGameplayAbilitySpec;
	friend struct FGameplayAbilitySpecContainer;
};
