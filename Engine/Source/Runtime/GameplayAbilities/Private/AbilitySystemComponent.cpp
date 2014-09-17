// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
// ActorComponent.cpp: Actor component implementation.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemComponent.h"
#include "GameplayCueInterface.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/Tasks/AbilityTask.h"

#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

DEFINE_LOG_CATEGORY(LogAbilitySystemComponent);

#define LOCTEXT_NAMESPACE "AbilitySystemComponent"


int32 DebugGameplayCues = 0;
static FAutoConsoleVariableRef CVarDebugGameplayCues(
	TEXT("AbilitySystem.DebugGameplayCues"),
	DebugGameplayCues,
	TEXT("Enables Debugging for GameplayCue events"),
	ECVF_Default
	);

/** Enable to log out all render state create, destroy and updatetransform events */
#define LOG_RENDER_STATE 0

UAbilitySystemComponent::UAbilitySystemComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bWantsInitializeComponent = true;

	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = true; // FIXME! Just temp until timer manager figured out
	PrimaryComponentTick.bCanEverTick = true;
	
	ActiveGameplayEffects.Owner = this;
	ActiveGameplayCues.Owner = this;

	SetIsReplicated(true);

	UserAbilityActivationInhibited = false;
}

UAbilitySystemComponent::~UAbilitySystemComponent()
{
	ActiveGameplayEffects.PreDestroy();
}

const UAttributeSet* UAbilitySystemComponent::InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable)
{
	const UAttributeSet* AttributeObj = NULL;
	if (Attributes)
	{
		AttributeObj = GetOrCreateAttributeSubobject(Attributes);
		if (AttributeObj && DataTable)
		{
			// This const_cast is OK - this is one of the few places we want to directly modify our AttributeSet properties rather
			// than go through a gameplay effect
			const_cast<UAttributeSet*>(AttributeObj)->InitFromMetaDataTable(DataTable);
		}
	}
	return AttributeObj;
}

void UAbilitySystemComponent::K2_InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable)
{
	InitStats(Attributes, DataTable);
}

const UAttributeSet* UAbilitySystemComponent::GetOrCreateAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass)
{
	AActor *OwningActor = GetOwner();
	const UAttributeSet *MyAttributes  = NULL;
	if (OwningActor && AttributeClass)
	{
		MyAttributes = GetAttributeSubobject(AttributeClass);
		if (!MyAttributes)
		{
			MyAttributes = ConstructObject<UAttributeSet>(AttributeClass, OwningActor);
			SpawnedAttributes.AddUnique(MyAttributes);
		}
	}

	return MyAttributes;
}

const UAttributeSet* UAbilitySystemComponent::GetAttributeSubobjectChecked(const TSubclassOf<UAttributeSet> AttributeClass) const
{
	const UAttributeSet *Set = GetAttributeSubobject(AttributeClass);
	check(Set);
	return Set;
}

const UAttributeSet* UAbilitySystemComponent::GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const
{
	for (const UAttributeSet* Set : SpawnedAttributes)
	{
		if (Set && Set->IsA(AttributeClass))
		{
			return Set;
		}
	}
	return NULL;
}

void UAbilitySystemComponent::OnRegister()
{
	Super::OnRegister();

	// Init starting data
	for (int32 i=0; i < DefaultStartingData.Num(); ++i)
	{
		if (DefaultStartingData[i].Attributes && DefaultStartingData[i].DefaultStartingTable)
		{
			UAttributeSet* Attributes = const_cast<UAttributeSet*>(GetOrCreateAttributeSubobject(DefaultStartingData[i].Attributes));
			Attributes->InitFromMetaDataTable(DefaultStartingData[i].DefaultStartingTable);
		}
	}
}

// ---------------------------------------------------------

bool UAbilitySystemComponent::AreGameplayEffectApplicationRequirementsSatisfied(const class UGameplayEffect* EffectToAdd, FGameplayEffectInstigatorContext& Instigator) const
{
	bool bReqsSatisfied = false;
	if (EffectToAdd)
	{
		// Collect gameplay tags from instigator and target to see if requirements are satisfied
		FGameplayTagContainer InstigatorTags;
		Instigator.GetOwnedGameplayTags(InstigatorTags);

		FGameplayTagContainer TargetTags;
		IGameplayTagAssetInterface* OwnerGTA = InterfaceCast<IGameplayTagAssetInterface>(AbilityActorInfo->Actor.Get());
		if (OwnerGTA)
		{
			OwnerGTA->GetOwnedGameplayTags(TargetTags);
		}

		bReqsSatisfied = EffectToAdd->AreApplicationTagRequirementsSatisfied(InstigatorTags, TargetTags);
	}

	return bReqsSatisfied;
}

// ---------------------------------------------------------

bool UAbilitySystemComponent::IsOwnerActorAuthoritative() const
{
	return !IsNetSimulating();
}

bool UAbilitySystemComponent::HasNetworkAuthorityToApplyGameplayEffect(const FModifierQualifier QualifierContext) const
{
	return (IsOwnerActorAuthoritative() || QualifierContext.PredictionKey().IsStale()==false);
}

void UAbilitySystemComponent::SetNumericAttribute(const FGameplayAttribute &Attribute, float NewFloatValue)
{
	const UAttributeSet* AttributeSet = GetAttributeSubobjectChecked(Attribute.GetAttributeSetClass());
	Attribute.SetNumericValueChecked(NewFloatValue, const_cast<UAttributeSet*>(AttributeSet));
}

float UAbilitySystemComponent::GetNumericAttribute(const FGameplayAttribute &Attribute)
{
	const UAttributeSet* AttributeSet = GetAttributeSubobjectChecked(Attribute.GetAttributeSetClass());
	return Attribute.GetNumericValueChecked(AttributeSet);
}

FGameplayEffectSpecHandle UAbilitySystemComponent::GetOutgoingSpec(UGameplayEffect* GameplayEffect) const
{
	SCOPE_CYCLE_COUNTER(STAT_GetOutgoingSpec);
	// Fixme: we should build a map and cache these off. We can invalidate the map when an OutgoingGE modifier is applied or removed from us.

	FGameplayEffectSpec* NewSpec = new FGameplayEffectSpec(GameplayEffect, AbilityActorInfo->Actor.Get(), FGameplayEffectLevelSpec::INVALID_LEVEL, GetCurveDataOverride());
	if (ActiveGameplayEffects.ApplyActiveEffectsTo(*NewSpec, FModifierQualifier().Type(EGameplayMod::OutgoingGE)))
	{
		return FGameplayEffectSpecHandle(NewSpec);
	}

	delete NewSpec;
	return FGameplayEffectSpecHandle(nullptr);
}

FGameplayEffectInstigatorContext UAbilitySystemComponent::GetInstigatorContext() const
{
	FGameplayEffectInstigatorContext Context;
	Context.AddInstigator(AbilityActorInfo->Actor.Get());
	return Context;
}

/** This is a helper function used in automated testing, I'm not sure how useful it will be to gamecode or blueprints */
FActiveGameplayEffectHandle UAbilitySystemComponent::ApplyGameplayEffectToTarget(UGameplayEffect *GameplayEffect, UAbilitySystemComponent *Target, float Level, FModifierQualifier BaseQualifier)
{
	check(GameplayEffect);
	if (HasNetworkAuthorityToApplyGameplayEffect(BaseQualifier))
	{
		FGameplayEffectSpec	Spec(GameplayEffect, AbilityActorInfo->Actor.Get(), Level, GetCurveDataOverride());
		return ApplyGameplayEffectSpecToTarget(Spec, Target, BaseQualifier);
	}

	return FActiveGameplayEffectHandle();
}

/** Helper function since we can't have default/optional values for FModifierQualifier in K2 function */
FActiveGameplayEffectHandle UAbilitySystemComponent::K2_ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator)
{
	return ApplyGameplayEffectToSelf(GameplayEffect, Level, Instigator);
}

/** This is a helper function - it seems like this will be useful as a blueprint interface at the least, but Level parameter may need to be expanded */
FActiveGameplayEffectHandle UAbilitySystemComponent::ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator, FModifierQualifier BaseQualifier)
{
	check(GameplayEffect);

	if (HasNetworkAuthorityToApplyGameplayEffect(BaseQualifier))
	{
		FGameplayEffectSpec	Spec(GameplayEffect, Instigator, Level, GetCurveDataOverride());
		return ApplyGameplayEffectSpecToSelf(Spec, BaseQualifier);
	}

	return FActiveGameplayEffectHandle();
}

float UAbilitySystemComponent::GetGameplayEffectMagnitudeByTag(FActiveGameplayEffectHandle InHandle, const FGameplayTag& InTag) const
{
	return ActiveGameplayEffects.GetGameplayEffectMagnitudeByTag(InHandle, InTag);
}

FOnActiveGameplayEffectRemoved* UAbilitySystemComponent::OnGameplayEffectRemovedDelegate(FActiveGameplayEffectHandle Handle)
{
	FActiveGameplayEffect* ActiveEffect = ActiveGameplayEffects.GetActiveGameplayEffect(Handle);
	if (ActiveEffect)
	{
		return &ActiveEffect->OnRemovedDelegate;
	}

	return nullptr;
}

int32 UAbilitySystemComponent::GetNumActiveGameplayEffect() const
{
	return ActiveGameplayEffects.GetNumGameplayEffects();
}

bool UAbilitySystemComponent::IsGameplayEffectActive(FActiveGameplayEffectHandle InHandle) const
{
	return ActiveGameplayEffects.IsGameplayEffectActive(InHandle);
}

FOnGameplayEffectTagCountChanged& UAbilitySystemComponent::RegisterGameplayTagEvent(FGameplayTag Tag)
{
	return ActiveGameplayEffects.RegisterGameplayTagEvent(Tag);
}

FOnGameplayAttributeChange& UAbilitySystemComponent::RegisterGameplayAttributeEvent(FGameplayAttribute Attribute)
{
	return ActiveGameplayEffects.RegisterGameplayAttributeEvent(Attribute);
}

// ------------------------------------------------------------------------

void UAbilitySystemComponent::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	return ActiveGameplayEffects.GetOwnedGameplayTags(TagContainer);
}

bool UAbilitySystemComponent::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	return ActiveGameplayEffects.HasMatchingGameplayTag(TagToCheck);
}

bool UAbilitySystemComponent::HasAllMatchingGameplayTags(const FGameplayTagContainer& TagContainer, bool bCountEmptyAsMatch) const
{
	return ActiveGameplayEffects.HasAllMatchingGameplayTags(TagContainer, bCountEmptyAsMatch);
}

bool UAbilitySystemComponent::HasAnyMatchingGameplayTags(const FGameplayTagContainer& TagContainer, bool bCountEmptyAsMatch) const
{
	return ActiveGameplayEffects.HasAnyMatchingGameplayTags(TagContainer, bCountEmptyAsMatch);
}

// ------------------------------------------------------------------------

void UAbilitySystemComponent::TEMP_ApplyActiveGameplayEffects()
{
	for (int32 idx=0; idx < ActiveGameplayEffects.GameplayEffects.Num(); ++idx)
	{
		FActiveGameplayEffect& ActiveEffect = ActiveGameplayEffects.GameplayEffects[idx];

		ExecuteGameplayEffect(ActiveEffect.Spec, FModifierQualifier().IgnoreHandle(ActiveEffect.Handle));

		ABILITY_LOG(Log, TEXT("ActiveEffect[%d] %s - Duration: %.2f]"), idx, *ActiveEffect.Spec.ToSimpleString(), ActiveEffect.Spec.GetDuration());
	}
}

FActiveGameplayEffectHandle UAbilitySystemComponent::ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec &Spec, UAbilitySystemComponent *Target, FModifierQualifier BaseQualifier)
{
	if (HasNetworkAuthorityToApplyGameplayEffect(BaseQualifier))
	{
		// Apply outgoing Effects to the Spec.
		// Outgoing immunity may stop the outgoing effect from being applied to the target
		if (ActiveGameplayEffects.ApplyActiveEffectsTo(Spec, FModifierQualifier(BaseQualifier).Type(EGameplayMod::OutgoingGE)))
		{
			return Target->ApplyGameplayEffectSpecToSelf(Spec, BaseQualifier);
		}
	}

	return FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &Spec, FModifierQualifier BaseQualifier)
{
	// Temp, only non instant, non periodic GEs can be predictive
	// Effects with other effects may be a mix so go with non-predictive
	check((BaseQualifier.PredictionKey().IsValidKey() == false) || (Spec.GetPeriod() == UGameplayEffect::NO_PERIOD));

	if (!HasNetworkAuthorityToApplyGameplayEffect(BaseQualifier))
	{
		return FActiveGameplayEffectHandle();
	}

	// Clients should treat predicted instant effects as if they have infinite duration. The effects will be cleaned up later.
	bool bTreatAsInfiniteDuration = GetOwnerRole() != ROLE_Authority && BaseQualifier.PredictionKey().IsValidKey() && Spec.GetDuration() == UGameplayEffect::INSTANT_APPLICATION;

	// check if the effect being applied actually succeeds
	float ChanceToApply = Spec.GetChanceToApplyToTarget();
	if ((ChanceToApply < 1.f - SMALL_NUMBER) && (FMath::FRand() > ChanceToApply))
	{
		return FActiveGameplayEffectHandle();
	}

	// Make sure we create our copy of the spec in the right place first...
	FActiveGameplayEffectHandle	MyHandle;
	bool bInvokeGameplayCueApplied = UGameplayEffect::INFINITE_DURATION == Spec.GetDuration();

	FGameplayEffectSpec* OurCopyOfSpec = NULL;
	TSharedPtr<FGameplayEffectSpec> StackSpec;
	float Duration = bTreatAsInfiniteDuration ? UGameplayEffect::INFINITE_DURATION : Spec.GetDuration();
	{
		if (Duration != UGameplayEffect::INSTANT_APPLICATION)
		{
			// recalculating stacking needs to come before creating the new effect
			if (Spec.GetStackingType() != EGameplayEffectStackingPolicy::Unlimited)
			{
				ActiveGameplayEffects.StacksNeedToRecalculate();
			}
			FActiveGameplayEffect &NewActiveEffect = ActiveGameplayEffects.CreateNewActiveGameplayEffect(Spec, BaseQualifier.PredictionKey());
			MyHandle = NewActiveEffect.Handle;
			OurCopyOfSpec = &NewActiveEffect.Spec;
		}
		
		if (!OurCopyOfSpec)
		{
			StackSpec = TSharedPtr<FGameplayEffectSpec>(new FGameplayEffectSpec(Spec));
			OurCopyOfSpec = StackSpec.Get();
		}

		// Do a 1st order copy of the spec so that we can modify it
		// (the one passed in is owned by the caller, we can't apply our incoming GEs to it)
		// Note that at this point the spec has a bunch of modifiers. Those modifiers may
		// have other modifiers. THOSE modifiers may or may not be copies of whatever.
		//
		// In theory, we don't modify 2nd order modifiers after they are 'attached'
		// Long complex chains can be created but we never say 'Modify a GE that is modding another GE'
		OurCopyOfSpec->MakeUnique();

		// if necessary add a modifier to OurCopyOfSpec to force it to have an infinite duration
		if (bTreatAsInfiniteDuration)
		{
			FGameplayModifierInfo ModInfo;
			ModInfo.ModifierOp = EGameplayModOp::Override;
			ModInfo.Magnitude.SetValue(UGameplayEffect::INFINITE_DURATION);
			ModInfo.EffectType = EGameplayModEffect::Duration;
			ModInfo.ModifierType = EGameplayMod::ActiveGE;
			TSharedPtr<FGameplayEffectLevelSpec> Level(new FGameplayEffectLevelSpec(0.f, OurCopyOfSpec->Def->LevelInfo, NULL));
			FModifierSpec Mod(ModInfo, Level, NULL);
			FModifierQualifier Qualifier;
			Qualifier.Type(EGameplayMod::ActiveGE);

			OurCopyOfSpec->ApplyModifier(Mod, Qualifier, true);
		}
	}

	// Now that we have our own copy, apply our GEs that modify IncomingGEs
	if (!ActiveGameplayEffects.ApplyActiveEffectsTo(*OurCopyOfSpec, FModifierQualifier(BaseQualifier).Type(EGameplayMod::IncomingGE).IgnoreHandle(MyHandle)))
	{
		// We're immune to this effect
		return FActiveGameplayEffectHandle();
	}	
	
	// Now that we have the final version of this effect, actually apply it if its going to be hanging around
	if (Duration != UGameplayEffect::INSTANT_APPLICATION )
	{
		if (Spec.GetPeriod() == UGameplayEffect::NO_PERIOD)
		{
			ActiveGameplayEffects.ApplySpecToActiveEffectsAndAttributes(*OurCopyOfSpec, FModifierQualifier(BaseQualifier).IgnoreHandle(MyHandle));
		}
	}
	
	// We still probably want to apply tags and stuff even if instant?
	if (bInvokeGameplayCueApplied)
	{
		// We both added and activated the GameplayCue here.
		// On the client, who will invoke the gameplay cue from an OnRep, he will need to look at the StartTime to determine
		// if the Cue was actually added+activated or just added (due to relevancy)

		// Fixme: what if we wanted to scale Cue magnitude based on damage? E.g, scale an cue effect when the GE is buffed?
		InvokeGameplayCueEvent(*OurCopyOfSpec, EGameplayCueEvent::OnActive);
		InvokeGameplayCueEvent(*OurCopyOfSpec, EGameplayCueEvent::WhileActive);
	}
	
	// Execute the GE at least once (if instant, this will execute once and be done. If persistent, it was added to ActiveGameplayEffects above)
	
	// Execute if this is an instant application effect
	if (Duration == UGameplayEffect::INSTANT_APPLICATION)
	{
		ExecuteGameplayEffect(*OurCopyOfSpec, FModifierQualifier(BaseQualifier).IgnoreHandle(MyHandle));
	}
	else if (bTreatAsInfiniteDuration)
	{
		// This is an instant application but we are treating it as an infinite duration for prediction. We should still predict the execute GameplayCUE.
		// (in non predictive case, this will happen inside ::ExecuteGameplayEffect)
		InvokeGameplayCueEvent(*OurCopyOfSpec, EGameplayCueEvent::Executed);
	}


	if (Spec.GetPeriod() != UGameplayEffect::NO_PERIOD && Spec.TargetEffectSpecs.Num() > 0)
	{
		ABILITY_LOG(Warning, TEXT("%s is periodic but also applies GameplayEffects to its target. GameplayEffects will only be applied once, not every period."), *Spec.Def->GetPathName());
	}
	// todo: this is ignoring the returned handles, should we put them into a TArray and return all of the handles?
	for (const TSharedRef<FGameplayEffectSpec> TargetSpec : Spec.TargetEffectSpecs)
	{
		ApplyGameplayEffectSpecToSelf(TargetSpec.Get(), BaseQualifier);
	}

	return MyHandle;
}

void UAbilitySystemComponent::ExecutePeriodicEffect(FActiveGameplayEffectHandle	Handle)
{
	ActiveGameplayEffects.ExecutePeriodicGameplayEffect(Handle);
}

void UAbilitySystemComponent::ExecuteGameplayEffect(const FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext)
{
	// Should only ever execute effects that are instant application or periodic application
	// Effects with no period and that aren't instant application should never be executed
	check( (Spec.GetDuration() == UGameplayEffect::INSTANT_APPLICATION || Spec.GetPeriod() != UGameplayEffect::NO_PERIOD) );
	
	ActiveGameplayEffects.ExecuteActiveEffectsFrom(Spec, QualifierContext);
}

void UAbilitySystemComponent::CheckDurationExpired(FActiveGameplayEffectHandle Handle)
{
	ActiveGameplayEffects.CheckDuration(Handle);
}

bool UAbilitySystemComponent::RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle)
{
	return ActiveGameplayEffects.RemoveActiveGameplayEffect(Handle);
}

float UAbilitySystemComponent::GetGameplayEffectDuration(FActiveGameplayEffectHandle Handle) const
{
	return ActiveGameplayEffects.GetGameplayEffectDuration(Handle);
}

float UAbilitySystemComponent::GetGameplayEffectMagnitude(FActiveGameplayEffectHandle Handle, FGameplayAttribute Attribute) const
{
	return ActiveGameplayEffects.GetGameplayEffectMagnitude(Handle, Attribute);
}

void UAbilitySystemComponent::InvokeGameplayCueEvent(const FGameplayEffectSpec &Spec, EGameplayCueEvent::Type EventType)
{
	if (DebugGameplayCues)
	{
		ABILITY_LOG(Warning, TEXT("InvokeGameplayCueEvent: %s"), *Spec.ToSimpleString());
	}

	AActor* ActorOwner = AbilityActorInfo->Actor.Get();
	IGameplayCueInterface* GameplayCueInterface = InterfaceCast<IGameplayCueInterface>(ActorOwner);
	if (!GameplayCueInterface)
	{
		ABILITY_LOG(Warning, TEXT("InvokeGameplayCueEvent %s on Actor %s that is not IGameplayCueInterface"), *Spec.ToSimpleString(), ActorOwner ? *ActorOwner->GetName() : TEXT("NULL"));
		return;
	}

	// FIXME: Replication of level not finished
	float ExecuteLevel =  (Spec.ModifierLevel.IsValid() && Spec.ModifierLevel.Get()->IsValid()) ? Spec.ModifierLevel.Get()->GetLevel() : 1.f;

	FGameplayCueParameters CueParameters;
	CueParameters.InstigatorContext = Spec.InstigatorContext;

	for (FGameplayEffectCue CueInfo : Spec.Def->GameplayCues)
	{
		CueParameters.NormalizedMagnitude = CueInfo.NormalizeLevel(ExecuteLevel);
		GameplayCueInterface->HandleGameplayCues(ActorOwner, CueInfo.GameplayCueTags, EventType, CueParameters);

		if (DebugGameplayCues)
		{
			DrawDebugSphere(GetWorld(), Spec.InstigatorContext.HitResult->Location, 30.f, 32, FColor(255, 0, 0), true, 30.f);
			ABILITY_LOG(Warning, TEXT("   %s"), *CueInfo.GameplayCueTags.ToString());
		}
	}
}

void UAbilitySystemComponent::ExecuteGameplayCue(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey)
{
	if (IsOwnerActorAuthoritative())
	{
		NetMulticast_InvokeGameplayCueExecuted(GameplayCueTag, PredictionKey);
	}
	else if (PredictionKey.IsValidKey())
	{
		InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Executed);
	}
}

void UAbilitySystemComponent::AddGameplayCue(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey)
{
	if (IsOwnerActorAuthoritative())
	{
		ActiveGameplayCues.AddCue(GameplayCueTag);
		NetMulticast_InvokeGameplayCueAdded(GameplayCueTag, PredictionKey);
	}
	else if (PredictionKey.IsValidKey())
	{
		// Allow for predictive gameplaycue events? Needs more thought
		InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::OnActive);
		InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::WhileActive);
	}
}

void UAbilitySystemComponent::RemoveGameplayCue(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey)
{
	if (IsOwnerActorAuthoritative())
	{
		ActiveGameplayCues.RemoveCue(GameplayCueTag);
	}
	else if (PredictionKey.IsValidKey())
	{
		InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Removed);
	}
}

void UAbilitySystemComponent::InvokeGameplayCueEvent(const FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType)
{
	AActor* ActorOwner = AbilityActorInfo->Actor.Get();
	IGameplayCueInterface* GameplayCueInterface = InterfaceCast<IGameplayCueInterface>(ActorOwner);
	if (!GameplayCueInterface)
	{
		return;
	}

	FGameplayCueParameters CueParameters;
	CueParameters.InstigatorContext.AddInstigator(ActorOwner);	// Treat ourselves as the instigator for now. We may need to allow this to be passed in eventually.

	const float NormalizedMagnitude = 1.f;

	GameplayCueInterface->HandleGameplayCue(ActorOwner, GameplayCueTag, EventType, CueParameters);
}

void UAbilitySystemComponent::NetMulticast_InvokeGameplayCueExecuted_FromSpec_Implementation(const FGameplayEffectSpec Spec, FPredictionKey PredictionKey)
{
	if (PredictionKey.IsValidKey() == false)
	{
		InvokeGameplayCueEvent(Spec, EGameplayCueEvent::Executed);
	}
}

void UAbilitySystemComponent::NetMulticast_InvokeGameplayCueExecuted_Implementation(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey)
{
	if (PredictionKey.IsValidKey() == false)
	{
		InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::Executed);
	}
}

void UAbilitySystemComponent::NetMulticast_InvokeGameplayCueAdded_Implementation(const FGameplayTag GameplayCueTag, FPredictionKey PredictionKey)
{
	if (PredictionKey.IsValidKey() == false)
	{
		InvokeGameplayCueEvent(GameplayCueTag, EGameplayCueEvent::OnActive);
	}
}

bool UAbilitySystemComponent::IsGameplayCueActive(const FGameplayTag GameplayCueTag) const
{
	return (ActiveGameplayEffects.HasMatchingGameplayTag(GameplayCueTag) || ActiveGameplayCues.HasMatchingGameplayTag(GameplayCueTag));
}

// ----------------------------------------------------------------------------------------

void UAbilitySystemComponent::AddDependancyToAttribute(FGameplayAttribute Attribute, const TWeakPtr<FAggregator> InDependant)
{
	ActiveGameplayEffects.AddDependancyToAttribute(Attribute, InDependant);
}

bool UAbilitySystemComponent::CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator)
{
	return ActiveGameplayEffects.CanApplyAttributeModifiers(GameplayEffect, Level, Instigator);
}

TArray<float> UAbilitySystemComponent::GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const
{
	return ActiveGameplayEffects.GetActiveEffectsTimeRemaining(Query);
}

void UAbilitySystemComponent::OnRestackGameplayEffects()
{
	ActiveGameplayEffects.RecalculateStacking();
}

// ---------------------------------------------------------------------------------------

void UAbilitySystemComponent::TickingTaskStarted(UAbilityTask* NewTask)
{
	// If this is our first ticking task, set this component as active so it begins ticking
	if (TickingTasks.Num() == 0)
	{
		SetActive(true);
	}
	TickingTasks.AddUnique(NewTask);
}

void UAbilitySystemComponent::TickingTaskEnded(UAbilityTask* Task)
{
	// If we are removing our last ticking task, set this component as inactive so it stops ticking
	TickingTasks.RemoveSingleSwap(Task);
	if (TickingTasks.Num() == 0)
	{
		SetActive(false);
	}
}

// ---------------------------------------------------------------------------------------

void UAbilitySystemComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAbilitySystemComponent, SpawnedAttributes);
	DOREPLIFETIME(UAbilitySystemComponent, ActiveGameplayEffects);
	DOREPLIFETIME(UAbilitySystemComponent, ActiveGameplayCues);
	
	DOREPLIFETIME(UAbilitySystemComponent, ActivatableAbilities);

	DOREPLIFETIME(UAbilitySystemComponent, AbilityActor);

	DOREPLIFETIME(UAbilitySystemComponent, ReplicatedPredictionKey);
}

bool UAbilitySystemComponent::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (const UAttributeSet* Set : SpawnedAttributes)
	{
		if (Set)
		{
			WroteSomething |= Channel->ReplicateSubobject(const_cast<UAttributeSet*>(Set), *Bunch, *RepFlags);
		}
	}

	for (UGameplayAbility* Ability : AllReplicatedInstancedAbilities)
	{
		if (Ability && !Ability->HasAnyFlags(RF_PendingKill))
		{
			WroteSomething |= Channel->ReplicateSubobject(Ability, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UAbilitySystemComponent::GetSubobjectsWithStableNamesForNetworking(TArray<UObject*>& Objs)
{
	for (const UAttributeSet* Set : SpawnedAttributes)
	{
		if (Set && Set->IsNameStableForNetworking())
		{
			Objs.Add(const_cast<UAttributeSet*>(Set));
		}
	}
}

void UAbilitySystemComponent::OnRep_GameplayEffects()
{

}

void UAbilitySystemComponent::OnRep_PredictionKey()
{
	// Every predictive action we've done up to and including the current value of ReplicatedPredictionKey needs to be wiped
	int32 idx = 0;
	for ( ; idx < PredictionDelegates.Num(); ++idx)
	{
		if (PredictionDelegates[idx].Key <= ReplicatedPredictionKey.Current)
		{
			PredictionDelegates[idx].Value.PredictionKeyClearDelegate.Broadcast();
		}
		else
		{
			break;
		}
	}
	if (idx > 0)
	{
		// Remove everyone we called Broadcast on in one call
		PredictionDelegates.RemoveAt(0, idx);
	}
}

UAbilitySystemComponent::FPredictionInfo& UAbilitySystemComponent::GetPredictionKeyDelegate(FPredictionKey::KeyType PredictionKey)
{
	// See if we already have one for this key
	for (auto & Item : PredictionDelegates)
	{
		if (Item.Key == PredictionKey)
		{
			return Item.Value;
		}
	}

	// Create a new one
	TPair<FPredictionKey::KeyType, FPredictionInfo> NewPair;
	NewPair.Key = PredictionKey;
	
	PredictionDelegates.Add(NewPair);
	return PredictionDelegates.Last().Value;
}

// ---------------------------------------------------------------------------------------

void UAbilitySystemComponent::PrintAllGameplayEffects() const
{
	ABILITY_LOG_SCOPE(TEXT("PrintAllGameplayEffects %s"), *GetName());
	ABILITY_LOG(Log, TEXT("Owner: %s. Avatar: %s"), *GetOwner()->GetName(), *AbilityActorInfo->Actor->GetName());
	ActiveGameplayEffects.PrintAllGameplayEffects();
}

void FActiveGameplayEffectsContainer::PrintAllGameplayEffects() const
{
	ABILITY_LOG_SCOPE(TEXT("ActiveGameplayEffects. Num: %d"), GameplayEffects.Num());
	for (const FActiveGameplayEffect& Effect : GameplayEffects)
	{
		Effect.PrintAll();
	}
}

void FActiveGameplayEffect::PrintAll() const
{
	ABILITY_LOG(Log, TEXT("Handle: %s"), *Handle.ToString());
	ABILITY_LOG(Log, TEXT("StartWorldTime: %.2f"), StartWorldTime);
	Spec.PrintAll();
}

void FGameplayEffectSpec::PrintAll() const
{
	ABILITY_LOG_SCOPE(TEXT("GameplayEffectSpec"));
	ABILITY_LOG(Log, TEXT("Def: %s"), *Def->GetName());
	
	ABILITY_LOG(Log, TEXT("Duration: "));
	Duration.PrintAll();

	ABILITY_LOG(Log, TEXT("Period:"));
	Period.PrintAll();

	ABILITY_LOG(Log, TEXT("Modifiers:"));
	for (const FModifierSpec &Mod : Modifiers)
	{
		Mod.PrintAll();
	}
}

void FModifierSpec::PrintAll() const
{
	ABILITY_LOG_SCOPE(TEXT("ModifierSpec"));
	ABILITY_LOG(Log, TEXT("Attribute: %s"), *Info.Attribute.GetName());
	ABILITY_LOG(Log, TEXT("ModifierType: %s"), *EGameplayModToString(Info.ModifierType));
	ABILITY_LOG(Log, TEXT("ModifierOp: %s"), *EGameplayModOpToString(Info.ModifierOp));
	ABILITY_LOG(Log, TEXT("EffectType: %s"), *EGameplayModEffectToString(Info.EffectType));
	ABILITY_LOG(Log, TEXT("RequiredTags: %s"), *Info.RequiredTags.ToString());
	ABILITY_LOG(Log, TEXT("OwnedTags: %s"), *Info.OwnedTags.ToString());
	ABILITY_LOG(Log, TEXT("(Base) Magnitude: %s"), *Info.Magnitude.ToSimpleString());

	Aggregator.PrintAll();
}

void FAggregatorRef::PrintAll() const
{
	if (!WeakPtr.IsValid())
	{
		ABILITY_LOG(Log, TEXT("Invalid AggregatorRef"));
		return;
	}

	if (SharedPtr.IsValid())
	{
		ABILITY_LOG(Log, TEXT("HardRef AggregatorRef"));
	}
	else
	{
		ABILITY_LOG(Log, TEXT("SoftRef AggregatorRef"));

	}
	
	Get()->PrintAll();
}

void FAggregator::PrintAll() const
{
	ABILITY_LOG_SCOPE(TEXT("FAggregator 0x%X"), this);

#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	ABILITY_LOG(Log, TEXT("DebugStr: %s"), *DebugString);
	ABILITY_LOG(Log, TEXT("Copies (of me): %d"), CopiesMade);
#endif

	if (Level.IsValid())
	{
		ABILITY_LOG_SCOPE(TEXT("LevelInfo"));
		Level->PrintAll();
	}
	else
	{
		ABILITY_LOG(Log, TEXT("No Level Data"));
	}

	{
		ABILITY_LOG_SCOPE(TEXT("BaseData"));
		BaseData.PrintAll();
	}

	{
		ABILITY_LOG_SCOPE(TEXT("CachedData"));
		CachedData.PrintAll();
	}
	
	for (int32 i=0; i < EGameplayModOp::Max; ++i)
	{
		if (Mods[i].Num() > 0)
		{
			ABILITY_LOG_SCOPE(TEXT("%s Mods"), *EGameplayModOpToString(i));
			for (const FAggregatorRef &Ref : Mods[i])
			{
				Ref.PrintAll();
			}
		}
	}
}

void FGameplayModifierData::PrintAll() const
{
	ABILITY_LOG(Log, TEXT("Magnitude: %s"), *Magnitude.ToSimpleString());
	ABILITY_LOG(Log, TEXT("Tags: %s"), *Tags.ToString());
}

void FGameplayModifierEvaluatedData::PrintAll() const
{
	ABILITY_LOG(Log, TEXT("IsValid: %d"), IsValid); 
	ABILITY_LOG(Log, TEXT("Magnitude: %.2f"), Magnitude);
	ABILITY_LOG(Log, TEXT("Tags: %s"), *Tags.ToString());
}

void FGameplayEffectLevelSpec::PrintAll() const
{
	ABILITY_LOG(Log, TEXT("ConstantLevel: %.2f"), ConstantLevel);
}

#undef LOCTEXT_NAMESPACE
