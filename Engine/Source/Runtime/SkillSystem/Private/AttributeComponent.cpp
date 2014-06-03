// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
// ActorComponent.cpp: Actor component implementation.

#include "SkillSystemModulePrivatePCH.h"
#include "AttributeComponent.h"
#include "GameplayCueInterface.h"
#include "Abilities/GameplayAbility.h"


#include "Net/UnrealNetwork.h"
#include "Engine/ActorChannel.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

DEFINE_LOG_CATEGORY(LogAttributeComponent);

#define LOCTEXT_NAMESPACE "AttributeComponent"


int32 DebugGameplayCues = 0;
static FAutoConsoleVariableRef CVarDebugGameplayCues(
	TEXT("SkillSystem.DebugGameplayCues"),
	DebugGameplayCues,
	TEXT("Enables Debugging for GameplayCue events"),
	ECVF_Default
	);

#pragma optimize( "", off )

/** Enable to log out all render state create, destroy and updatetransform events */
#define LOG_RENDER_STATE 0

UAttributeComponent::UAttributeComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bWantsInitializeComponent = true;

	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	PrimaryComponentTick.bStartWithTickEnabled = true; // FIXME! Just temp until timer manager figured out
	PrimaryComponentTick.bCanEverTick = true;
	
	ActiveGameplayEffects.Owner = this;

	SetIsReplicated(true);
	ReplicatedPredictionKey = 0;
	LocalPredictionKey = 0;
}

UAttributeComponent::~UAttributeComponent()
{
	ActiveGameplayEffects.PreDestroy();
}

UAttributeSet* UAttributeComponent::InitStats(TSubclassOf<class UAttributeSet> Attributes, const UDataTable* DataTable)
{
	UAttributeSet* AttributeObj = NULL;
	if (Attributes)
	{
		AttributeObj = GetOrCreateAttributeSubobject(Attributes);
		if (AttributeObj && DataTable)
		{
			AttributeObj->InitFromMetaDataTable(DataTable);
		}
	}
	return AttributeObj;
}

void UAttributeComponent::ModifyStats(TSubclassOf<class UAttributeSet> AttributeClass, FDataTableRowHandle ModifierHandle)
{
	UAttributeSet *Attributes = Cast<UAttributeSet>(GetAttributeSubobject(AttributeClass));
	if (Attributes)
	{
		FAttributeModifierTest *ModifierData = ModifierHandle.GetRow<FAttributeModifierTest>();
		if (ModifierData)
		{
			ModifierData->ApplyModifier(Attributes);
		}
	}
}

UAttributeSet*	UAttributeComponent::GetOrCreateAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass)
{
	AActor *OwningActor = GetOwner();
	UAttributeSet *MyAttributes  = NULL;
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

UAttributeSet* UAttributeComponent::GetAttributeSubobjectChecked(const TSubclassOf<UAttributeSet> AttributeClass) const
{
	UAttributeSet *Set = GetAttributeSubobject(AttributeClass);
	check(Set);
	return Set;
}

UAttributeSet* UAttributeComponent::GetAttributeSubobject(const TSubclassOf<UAttributeSet> AttributeClass) const
{
	for (UAttributeSet * Set : SpawnedAttributes)
	{
		if (Set && Set->IsA(AttributeClass))
		{
			return Set;
		}
	}
	return NULL;
}

void UAttributeComponent::OnRegister()
{
	Super::OnRegister();

	// Init starting data
	for (int32 i=0; i < DefaultStartingData.Num(); ++i)
	{
		if (DefaultStartingData[i].Attributes && DefaultStartingData[i].DefaultStartingTable)
		{
			UAttributeSet* Attributes = GetOrCreateAttributeSubobject(DefaultStartingData[i].Attributes);
			Attributes->InitFromMetaDataTable(DefaultStartingData[i].DefaultStartingTable);
		}
	}
}

// ---------------------------------------------------------

bool UAttributeComponent::AreGameplayEffectApplicationRequirementsSatisfied(const class UGameplayEffect* EffectToAdd, FGameplayEffectInstigatorContext& Instigator) const
{
	bool bReqsSatisfied = false;
	if (EffectToAdd)
	{
		// Collect gameplay tags from instigator and target to see if requirements are satisfied
		FGameplayTagContainer InstigatorTags;
		Instigator.GetOwnedGameplayTags(InstigatorTags);

		FGameplayTagContainer TargetTags;
		IGameplayTagAssetInterface* OwnerGTA = InterfaceCast<IGameplayTagAssetInterface>(GetOwner());
		if (OwnerGTA)
		{
			OwnerGTA->GetOwnedGameplayTags(TargetTags);
		}

		bReqsSatisfied = EffectToAdd->AreApplicationTagRequirementsSatisfied(InstigatorTags, TargetTags);
	}

	return bReqsSatisfied;
}

// ---------------------------------------------------------

bool UAttributeComponent::IsOwnerActorAuthoritative() const
{
	return !IsNetSimulating();
}

void UAttributeComponent::SetNumericAttribute(const FGameplayAttribute &Attribute, float NewFloatValue)
{
	UAttributeSet * AttributeSet = GetAttributeSubobjectChecked(Attribute.GetAttributeSetClass());
	Attribute.SetNumericValueChecked(NewFloatValue, AttributeSet);
}

float UAttributeComponent::GetNumericAttribute(const FGameplayAttribute &Attribute)
{
	UAttributeSet * AttributeSet = GetAttributeSubobjectChecked(Attribute.GetAttributeSetClass());
	return Attribute.GetNumericValueChecked(AttributeSet);
}

/** This is a helper function used in automated testing, I'm not sure how useful it will be to gamecode or blueprints */
FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectToTarget(UGameplayEffect *GameplayEffect, UAttributeComponent *Target, float Level, FModifierQualifier BaseQualifier)
{
	check(GameplayEffect);

	FGameplayEffectSpec	Spec(GameplayEffect, GetOwner(),  Level, GetCurveDataOverride());
	
	return ApplyGameplayEffectSpecToTarget(Spec, Target, BaseQualifier);
}

/** Helper function since we can't have default/optional values for FModifierQualifier in K2 function */
FActiveGameplayEffectHandle UAttributeComponent::K2_ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator)
{
	return ApplyGameplayEffectToSelf(GameplayEffect, Level, Instigator);
}

/** This is a helper function - it seems like this will be useful as a blueprint interface at the least, but Level parameter may need to be expanded */
FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator, FModifierQualifier BaseQualifier)
{
	check(GameplayEffect);

	FGameplayEffectSpec	Spec(GameplayEffect, Instigator, Level, GetCurveDataOverride());

	return ApplyGameplayEffectSpecToSelf(Spec, BaseQualifier);
}

float UAttributeComponent::GetGameplayEffectMagnitudeByTag(FActiveGameplayEffectHandle InHandle, const FGameplayTag& InTag) const
{
	return ActiveGameplayEffects.GetGameplayEffectMagnitudeByTag(InHandle, InTag);
}

int32 UAttributeComponent::GetNumActiveGameplayEffect() const
{
	return ActiveGameplayEffects.GetNumGameplayEffects();
}

bool UAttributeComponent::IsGameplayEffectActive(FActiveGameplayEffectHandle InHandle) const
{
	return ActiveGameplayEffects.IsGameplayEffectActive(InHandle);
}

bool UAttributeComponent::HasAnyTags(FGameplayTagContainer &Tags)
{
	// Fixme: we could aggregate our current tags into a map to avoid this type of iteration
	return ActiveGameplayEffects.HasAnyTags(Tags);
}

bool UAttributeComponent::HasAllTags(FGameplayTagContainer &Tags)
{
	return ActiveGameplayEffects.HasAllTags(Tags);
}

void UAttributeComponent::TEMP_ApplyActiveGameplayEffects()
{
	for (int32 idx=0; idx < ActiveGameplayEffects.GameplayEffects.Num(); ++idx)
	{
		FActiveGameplayEffect& ActiveEffect = ActiveGameplayEffects.GameplayEffects[idx];

		ExecuteGameplayEffect(ActiveEffect.Spec, FModifierQualifier().IgnoreHandle(ActiveEffect.Handle));

		SKILL_LOG(Log, TEXT("ActiveEffect[%d] %s - Duration: %.2f]"), idx, *ActiveEffect.Spec.ToSimpleString(), ActiveEffect.Spec.GetDuration());
	}
}

FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec &Spec, UAttributeComponent *Target, FModifierQualifier BaseQualifier)
{
	// Apply outgoing Effects to the Spec.
	// Outgoing immunity may stop the outgoing effect from being applied to the target
	if (ActiveGameplayEffects.ApplyActiveEffectsTo(Spec, FModifierQualifier(BaseQualifier).Type(EGameplayMod::OutgoingGE)))
	{
		return Target->ApplyGameplayEffectSpecToSelf(Spec, BaseQualifier);
	}

	return FActiveGameplayEffectHandle();
}

FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &Spec, FModifierQualifier BaseQualifier)
{
	// Temp, only non instant, non periodic GEs can be predictive
	// Effects with other effects may be a mix so go with non-predictive
	check((BaseQualifier.PredictionKey() == 0) || (Spec.GetDuration() != UGameplayEffect::INSTANT_APPLICATION && Spec.GetPeriod() == UGameplayEffect::NO_PERIOD));

	// check if the effect being applied actually succeeds
	float ChanceToApply = Spec.GetChanceToApplyToTarget();
	if ((ChanceToApply < 1.f - SMALL_NUMBER) && (FMath::FRand() > ChanceToApply))
	{
		return FActiveGameplayEffectHandle();
	}

	// Make sure we create our copy of the spec in the right place first...
	FActiveGameplayEffectHandle	MyHandle;
	bool bInvokeGameplayCueApplied = false;

	FGameplayEffectSpec* OurCopyOfSpec = NULL;
	TSharedPtr<FGameplayEffectSpec> StackSpec;
	{
	
		float Duration = Spec.GetDuration();

		if (Duration != UGameplayEffect::INSTANT_APPLICATION)
		{
			// recalculating stacking needs to come before creating the new effect
			if (Spec.StackingPolicy != EGameplayEffectStackingPolicy::Unlimited)
			{
				ActiveGameplayEffects.StacksNeedToRecalculate();
			}
			FActiveGameplayEffect &NewActiveEffect = ActiveGameplayEffects.CreateNewActiveGameplayEffect(Spec, BaseQualifier.PredictionKey());
			MyHandle = NewActiveEffect.Handle;
			OurCopyOfSpec = &NewActiveEffect.Spec;

			bInvokeGameplayCueApplied = true;
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
	}

	// Now that we have our own copy, apply our GEs that modify IncomingGEs
	if (!ActiveGameplayEffects.ApplyActiveEffectsTo(*OurCopyOfSpec, FModifierQualifier(BaseQualifier).Type(EGameplayMod::IncomingGE).IgnoreHandle(MyHandle)))
	{
		// We're immune to this effect
		return FActiveGameplayEffectHandle();
	}	
	
	// Now that we have the final version of this effect, actually apply it if its going to be hanging around
	if (Spec.GetDuration() != UGameplayEffect::INSTANT_APPLICATION )
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
		InvokeGameplayCueAdded(*OurCopyOfSpec);
		InvokeGameplayCueActivated(*OurCopyOfSpec);
	}
	
	// Execute the GE at least once (if instant, this will execute once and be done. If persistent, it was added to ActiveGameplayEffects above)
	
	// Execute if this is an instant application effect
	if (Spec.GetDuration() == UGameplayEffect::INSTANT_APPLICATION)
	{
		ExecuteGameplayEffect(*OurCopyOfSpec, FModifierQualifier(BaseQualifier).IgnoreHandle(MyHandle));
	}

	if (Spec.GetPeriod() != UGameplayEffect::NO_PERIOD && Spec.TargetEffectSpecs.Num() > 0)
	{
		SKILL_LOG(Warning, TEXT("%s is periodic but also applies GameplayEffects to its target. GameplayEffects will only be applied once, not every period."), *Spec.Def->GetPathName());
	}
	// todo: this is ignoring the returned handles, should we put them into a TArray and return all of the handles?
	for (const TSharedRef<FGameplayEffectSpec> TargetSpec : Spec.TargetEffectSpecs)
	{
		ApplyGameplayEffectSpecToSelf(TargetSpec.Get(), BaseQualifier);
	}

	return MyHandle;
}

void UAttributeComponent::ExecutePeriodicEffect(FActiveGameplayEffectHandle	Handle)
{
	ActiveGameplayEffects.ExecutePeriodicGameplayEffect(Handle);
}

void UAttributeComponent::ExecuteGameplayEffect(const FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext)
{
	// Should only ever execute effects that are instant application or periodic application
	// Effects with no period and that aren't instant application should never be executed
	check( (Spec.GetDuration() == UGameplayEffect::INSTANT_APPLICATION || Spec.GetPeriod() != UGameplayEffect::NO_PERIOD) );
	
	ActiveGameplayEffects.ExecuteActiveEffectsFrom(Spec, QualifierContext);
}

void UAttributeComponent::CheckDurationExpired(FActiveGameplayEffectHandle Handle)
{
	ActiveGameplayEffects.CheckDuration(Handle);
}

bool UAttributeComponent::RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle)
{
	return ActiveGameplayEffects.RemoveActiveGameplayEffect(Handle);
}

float UAttributeComponent::GetGameplayEffectDuration(FActiveGameplayEffectHandle Handle) const
{
	return ActiveGameplayEffects.GetGameplayEffectDuration(Handle);
}

float UAttributeComponent::GetGameplayEffectMagnitude(FActiveGameplayEffectHandle Handle, FGameplayAttribute Attribute) const
{
	return ActiveGameplayEffects.GetGameplayEffectMagnitude(Handle, Attribute);
}

void UAttributeComponent::InvokeGameplayCueExecute(const FGameplayEffectSpec &Spec)
{
	if (DebugGameplayCues)
	{
		SKILL_LOG(Warning, TEXT("InvokeGameplayCueExecute: %s"), *Spec.ToSimpleString());
	}


	AActor *ActorOwner = GetOwner();
	IGameplayCueInterface * GameplayCueInterface = InterfaceCast<IGameplayCueInterface>(ActorOwner);
	if (!GameplayCueInterface)
	{
		return;
	}
	
	// FIXME: Replication of level not finished
	float ExecuteLevel = Spec.ModifierLevel.Get()->IsValid() ? Spec.ModifierLevel.Get()->GetLevel() : 1.f;
	for (FGameplayEffectCue CueInfo : Spec.Def->GameplayCues)
	{
		float NormalizedMagnitude = CueInfo.NormalizeLevel(ExecuteLevel);
		GameplayCueInterface->GameplayCueExecuted(CueInfo.GameplayCueTags, NormalizedMagnitude, Spec.InstigatorContext);

		if (DebugGameplayCues)
		{
			DrawDebugSphere(GetWorld(), Spec.InstigatorContext.HitResult->Location, 30.f, 32, FColor(255, 0, 0), true, 30.f);
			SKILL_LOG(Warning, TEXT("   %s"), *CueInfo.GameplayCueTags.ToString());
		}
	}
}

void UAttributeComponent::InvokeGameplayCueActivated(const FGameplayEffectSpec &Spec)
{
	AActor *ActorOwner = GetOwner();
	IGameplayCueInterface * GameplayCueInterface = InterfaceCast<IGameplayCueInterface>(ActorOwner);
	if (!GameplayCueInterface)
	{
		return;
	}

	// FIXME: Replication of level not finished
	float ExecuteLevel = Spec.ModifierLevel.Get()->IsValid() ? Spec.ModifierLevel.Get()->GetLevel() : 1.f; 
	for (FGameplayEffectCue CueInfo : Spec.Def->GameplayCues)
	{
		float NormalizedMagnitude = CueInfo.NormalizeLevel(ExecuteLevel);
		GameplayCueInterface->GameplayCueActivated(CueInfo.GameplayCueTags, NormalizedMagnitude, Spec.InstigatorContext);
	}
}

void UAttributeComponent::NetMulticast_InvokeGameplayCueExecuted_Implementation(const FGameplayEffectSpec Spec)
{
	InvokeGameplayCueExecute(Spec);
}

void UAttributeComponent::InvokeGameplayCueAdded(const FGameplayEffectSpec &Spec)
{
	AActor *ActorOwner = GetOwner();
	IGameplayCueInterface * GameplayCueInterface = InterfaceCast<IGameplayCueInterface>(ActorOwner);
	if (!GameplayCueInterface)
	{
		return;
	}

	// FIXME: Replication of level not finished
	float ExecuteLevel = Spec.ModifierLevel.Get()->IsValid() ? Spec.ModifierLevel.Get()->GetLevel() : 1.f;
	for (FGameplayEffectCue CueInfo : Spec.Def->GameplayCues)
	{
		float NormalizedMagnitude = CueInfo.NormalizeLevel(ExecuteLevel);
		GameplayCueInterface->GameplayCueAdded(CueInfo.GameplayCueTags, NormalizedMagnitude, Spec.InstigatorContext);
	}
}

void UAttributeComponent::InvokeGameplayCueRemoved(const FGameplayEffectSpec &Spec)
{
	AActor *ActorOwner = GetOwner();
	IGameplayCueInterface * GameplayCueInterface = InterfaceCast<IGameplayCueInterface>(ActorOwner);
	if (!GameplayCueInterface)
	{
		return;
	}

	// FIXME: Replication of level not finished
	float ExecuteLevel = Spec.ModifierLevel.Get()->IsValid() ? Spec.ModifierLevel.Get()->GetLevel() : 1.f;
	for (FGameplayEffectCue CueInfo : Spec.Def->GameplayCues)
	{
		float NormalizedMagnitude = CueInfo.NormalizeLevel(ExecuteLevel);
		GameplayCueInterface->GameplayCueRemoved(CueInfo.GameplayCueTags, NormalizedMagnitude, Spec.InstigatorContext);
	}
}

void UAttributeComponent::AddDependancyToAttribute(FGameplayAttribute Attribute, const TWeakPtr<FAggregator> InDependant)
{
	ActiveGameplayEffects.AddDependancyToAttribute(Attribute, InDependant);
}

bool UAttributeComponent::CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator)
{
	return ActiveGameplayEffects.CanApplyAttributeModifiers(GameplayEffect, Level, Instigator);
}

TArray<float> UAttributeComponent::GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const
{
	return ActiveGameplayEffects.GetActiveEffectsTimeRemaining(Query);
}

void UAttributeComponent::OnRestackGameplayEffects()
{
	ActiveGameplayEffects.RecalculateStacking();
}

// ---------------------------------------------------------------------------------------

void UAttributeComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UAttributeComponent, SpawnedAttributes);
	DOREPLIFETIME(UAttributeComponent, ActiveGameplayEffects);

	DOREPLIFETIME(UAttributeComponent, ReplicatedInstancedAbilities);
	DOREPLIFETIME(UAttributeComponent, ActivatableAbilities);

	DOREPLIFETIME(UAttributeComponent, ReplicatedPredictionKey);
}

bool UAttributeComponent::ReplicateSubobjects(class UActorChannel *Channel, class FOutBunch *Bunch, FReplicationFlags *RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);

	for (UAttributeSet* Set : SpawnedAttributes)
	{
		if (Set)
		{
			WroteSomething |= Channel->ReplicateSubobject(Set, *Bunch, *RepFlags);
		}
	}

	for (UGameplayAbility* Ability : ReplicatedInstancedAbilities)
	{
		if (Ability)
		{
			WroteSomething |= Channel->ReplicateSubobject(Ability, *Bunch, *RepFlags);
		}
	}

	return WroteSomething;
}

void UAttributeComponent::GetSubobjectsWithStableNamesForNetworking(TArray<UObject*>& Objs)
{
	for (UAttributeSet* Set : SpawnedAttributes)
	{
		if (Set && Set->IsNameStableForNetworking())
		{
			Objs.Add(Set);
		}
	}
}

void UAttributeComponent::PostNetReceive()
{
	Super::PostNetReceive();
}

void UAttributeComponent::OnRep_GameplayEffects()
{

}

void UAttributeComponent::OnRep_PredictionKey()
{
	// Every predictive action we've done up to and including the current value of ReplicatedPredictionKey needs to be wiped
	int32 idx = 0;
	for ( ; idx < PredictionDelegates.Num(); ++idx)
	{
		if (PredictionDelegates[idx].Key <= ReplicatedPredictionKey)
		{
			PredictionDelegates[idx].Value.Broadcast();
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

FAttributeComponentPredictionKeyClear& UAttributeComponent::GetPredictionKeyDelegate(int32 PredictionKey)
{
	// See if we already have one for this key
	for (TPair<int32, FAttributeComponentPredictionKeyClear> & Item : PredictionDelegates)
	{
		if (Item.Key == PredictionKey)
		{
			return Item.Value;
		}
	}

	// Create a new one
	TPair<int32, FAttributeComponentPredictionKeyClear> NewPair;
	NewPair.Key = PredictionKey;
	
	PredictionDelegates.Add(NewPair);
	return PredictionDelegates.Last().Value;
}

int32 UAttributeComponent::GetNextPredictionKey()
{
	return ++LocalPredictionKey;
}

// ---------------------------------------------------------------------------------------

void UAttributeComponent::PrintAllGameplayEffects() const
{
	SKILL_LOG_SCOPE(TEXT("PrintAllGameplayEffects %s"), *GetName());
	SKILL_LOG(Log, TEXT("Owner: %s"), *GetOwner()->GetName());
	ActiveGameplayEffects.PrintAllGameplayEffects();
}

void FActiveGameplayEffectsContainer::PrintAllGameplayEffects() const
{
	SKILL_LOG_SCOPE(TEXT("ActiveGameplayEffects. Num: %d"), GameplayEffects.Num());
	for (const FActiveGameplayEffect& Effect : GameplayEffects)
	{
		Effect.PrintAll();
	}
}

void FActiveGameplayEffect::PrintAll() const
{
	SKILL_LOG(Log, TEXT("Handle: %s"), *Handle.ToString());
	SKILL_LOG(Log, TEXT("StartWorldTime: %.2f"), StartWorldTime);
	Spec.PrintAll();
}

void FGameplayEffectSpec::PrintAll() const
{
	SKILL_LOG_SCOPE(TEXT("GameplayEffectSpec"));
	SKILL_LOG(Log, TEXT("Def: %s"), *Def->GetName());
	
	SKILL_LOG(Log, TEXT("Duration: "));
	Duration.PrintAll();

	SKILL_LOG(Log, TEXT("Period:"));
	Period.PrintAll();

	SKILL_LOG(Log, TEXT("Modifiers:"));
	for (const FModifierSpec &Mod : Modifiers)
	{
		Mod.PrintAll();
	}
}

void FModifierSpec::PrintAll() const
{
	SKILL_LOG_SCOPE(TEXT("ModifierSpec"));
	SKILL_LOG(Log, TEXT("Attribute: %s"), *Info.Attribute.GetName());
	SKILL_LOG(Log, TEXT("ModifierType: %s"), *EGameplayModToString(Info.ModifierType));
	SKILL_LOG(Log, TEXT("ModifierOp: %s"), *EGameplayModOpToString(Info.ModifierOp));
	SKILL_LOG(Log, TEXT("EffectType: %s"), *EGameplayModEffectToString(Info.EffectType));
	SKILL_LOG(Log, TEXT("RequiredTags: %s"), *Info.RequiredTags.ToString());
	SKILL_LOG(Log, TEXT("OwnedTags: %s"), *Info.OwnedTags.ToString());
	SKILL_LOG(Log, TEXT("(Base) Magnitude: %s"), *Info.Magnitude.ToSimpleString());

	Aggregator.PrintAll();
}

void FAggregatorRef::PrintAll() const
{
	if (!WeakPtr.IsValid())
	{
		SKILL_LOG(Log, TEXT("Invalid AggregatorRef"));
		return;
	}

	if (SharedPtr.IsValid())
	{
		SKILL_LOG(Log, TEXT("HardRef AggregatorRef"));
	}
	else
	{
		SKILL_LOG(Log, TEXT("SoftRef AggregatorRef"));

	}
	
	Get()->PrintAll();
}

void FAggregator::PrintAll() const
{
	SKILL_LOG_SCOPE(TEXT("FAggregator 0x%X"), this);

#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	SKILL_LOG(Log, TEXT("DebugStr: %s"), *DebugString);
	SKILL_LOG(Log, TEXT("Copies (of me): %d"), CopiesMade);
#endif

	if (Level.IsValid())
	{
		SKILL_LOG_SCOPE(TEXT("LevelInfo"));
		Level->PrintAll();
	}
	else
	{
		SKILL_LOG(Log, TEXT("No Level Data"));
	}

	{
		SKILL_LOG_SCOPE(TEXT("BaseData"));
		BaseData.PrintAll();
	}

	{
		SKILL_LOG_SCOPE(TEXT("CachedData"));
		CachedData.PrintAll();
	}
	
	for (int32 i=0; i < EGameplayModOp::Max; ++i)
	{
		if (Mods[i].Num() > 0)
		{
			SKILL_LOG_SCOPE(TEXT("%s Mods"), *EGameplayModOpToString(i));
			for (const FAggregatorRef &Ref : Mods[i])
			{
				Ref.PrintAll();
			}
		}
	}
}

void FGameplayModifierData::PrintAll() const
{
	SKILL_LOG(Log, TEXT("Magnitude: %s"), *Magnitude.ToSimpleString());
	SKILL_LOG(Log, TEXT("Tags: %s"), *Tags.ToString());
}

void FGameplayModifierEvaluatedData::PrintAll() const
{
	SKILL_LOG(Log, TEXT("IsValid: %d"), IsValid); 
	SKILL_LOG(Log, TEXT("Magnitude: %.2f"), Magnitude);
	SKILL_LOG(Log, TEXT("Tags: %s"), *Tags.ToString());
}

void FGameplayEffectLevelSpec::PrintAll() const
{
	SKILL_LOG(Log, TEXT("ConstantLevel: %.2f"), ConstantLevel);
}

#undef LOCTEXT_NAMESPACE
