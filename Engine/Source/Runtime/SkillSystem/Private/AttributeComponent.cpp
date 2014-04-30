// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.
// ActorComponent.cpp: Actor component implementation.

#include "SkillSystemModulePrivatePCH.h"

#include "Net/UnrealNetwork.h"
#include "MessageLog.h"
#include "UObjectToken.h"
#include "MapErrors.h"

DEFINE_LOG_CATEGORY(LogAttributeComponent);

#define LOCTEXT_NAMESPACE "AttributeComponent"

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

UAttributeSet*	UAttributeComponent::GetOrCreateAttributeSubobject(UClass *AttributeClass)
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
			//LastSpawnedSet = MyAttributes;
			TestFloat= 999.f;
		}
	}

	return MyAttributes;
}

UAttributeSet* UAttributeComponent::GetAttributeSubobjectChecked(UClass *AttributeClass) const
{
	UAttributeSet *Set = GetAttributeSubobject(AttributeClass);
	check(Set);
	return Set;
}

UAttributeSet* UAttributeComponent::GetAttributeSubobject(UClass *AttributeClass) const
{
	// FIXME: Lookup may would be helpful here

	AActor *ActorOwner = GetOwner();
	if (ActorOwner)
	{
		TArray<UObject*> ChildObjects;
		GetObjectsWithOuter(ActorOwner, ChildObjects);

		for (int32 ChildIndex=0; ChildIndex < ChildObjects.Num(); ++ChildIndex)
		{
			UObject *Obj = ChildObjects[ChildIndex];
			if (Obj->GetClass()->IsChildOf(AttributeClass))
			{
				return Cast<UAttributeSet>(Obj);
			}
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
		TSet<FName> InstigatorTags;
		Instigator.GetOwnedGameplayTags(InstigatorTags);

		TSet<FName> TargetTags;
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
FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectSpecToTarget(UGameplayEffect *GameplayEffect, UAttributeComponent *Target, float Level, FModifierQualifier BaseQualifier) const
{
	check(GameplayEffect);

	FGameplayEffectSpec	Spec(GameplayEffect, TSharedPtr<FGameplayEffectLevelSpec>(new FGameplayEffectLevelSpec(Level, GameplayEffect->LevelInfo, GetOwner())), GetCurveDataOverride());
	Spec.Def = GameplayEffect;
	Spec.InstigatorStack.AddInstigator(GetOwner());
	
	return ApplyGameplayEffectSpecToTarget(Spec, Target);
}

/** Helper function since we can't have default/optional values for FModifierQualifier in K2 function */
FActiveGameplayEffectHandle UAttributeComponent::K2_ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator)
{
	return ApplyGameplayEffectToSelf(GameplayEffect, Level, Instigator);
}

/** This is a helper function - it seems like this will be useful as a blueprint interface at the least, but Level parameter may need to be expanded */
FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectToSelf(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator, FModifierQualifier BaseQualifier)
{
	FGameplayEffectSpec	Spec(GameplayEffect, TSharedPtr<FGameplayEffectLevelSpec>(new FGameplayEffectLevelSpec(Level, GameplayEffect->LevelInfo, GetOwner())), GetCurveDataOverride());
	Spec.Def = GameplayEffect;
	Spec.InstigatorStack.AddInstigator(Instigator);

	return ApplyGameplayEffectSpecToSelf(Spec, BaseQualifier);
}

float UAttributeComponent::GetGameplayEffectMagnitudeByTag(FActiveGameplayEffectHandle InHandle, FName InTagName) const
{
	return ActiveGameplayEffects.GetGameplayEffectMagnitudeByTag(InHandle, InTagName);
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

void UAttributeComponent::TEMP_ApplyActiveGameplayEffects()
{
	for (int32 idx=0; idx < ActiveGameplayEffects.GameplayEffects.Num(); ++idx)
	{
		FActiveGameplayEffect& ActiveEffect = ActiveGameplayEffects.GameplayEffects[idx];

		ExecuteGameplayEffect(ActiveEffect.Spec, FModifierQualifier().IgnoreHandle(ActiveEffect.Handle));

		SKILL_LOG(Log, TEXT("ActiveEffect[%d] %s - Duration: %.2f]"), idx, *ActiveEffect.Spec.ToSimpleString(), ActiveEffect.Spec.GetDuration());
	}
}

FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectSpecToTarget(OUT FGameplayEffectSpec &Spec, UAttributeComponent *Target, FModifierQualifier BaseQualifier) const
{
	// Apply outgoing Effects to the Spec.
	ActiveGameplayEffects.ApplyActiveEffectsTo(Spec, FModifierQualifier(BaseQualifier).Type(EGameplayMod::OutgoingGE));

	return Target->ApplyGameplayEffectSpecToSelf(Spec, BaseQualifier);
}

FActiveGameplayEffectHandle UAttributeComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &Spec, FModifierQualifier BaseQualifier)
{
	// Make sure we create our copy of the spec in the right place first...
	FActiveGameplayEffectHandle	MyHandle;
	bool bInvokeGameplayCueApplied = false;

	FGameplayEffectSpec* OurCopyOfSpec = NULL;
	TSharedPtr<FGameplayEffectSpec> StackSpec;
	{
	
		float Duration = Spec.GetDuration();
		if (Duration != UGameplayEffect::INSTANT_APPLICATION)
		{
			FActiveGameplayEffect &NewActiveEffect = ActiveGameplayEffects.CreateNewActiveGameplayEffect(Spec);
			MyHandle = NewActiveEffect.Handle;
			OurCopyOfSpec = &NewActiveEffect.Spec;

			/*
			FIXME: Timer. Currently using polling in ::Tick until timer issues sorted out.
			float Period = NewActiveEffect.GetPeriod();
			if (Period > 0.f)
			{
				GetWorld()->GetTimerManager().SetTimer(FTimerDelegate::CreateUObject(this, &UAttributeComponent::ExecutePeriodicEffect, MyHandle), Period, true);
			}
			*/

			
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
	ActiveGameplayEffects.ApplyActiveEffectsTo(*OurCopyOfSpec, FModifierQualifier(BaseQualifier).Type(EGameplayMod::IncomingGE).IgnoreHandle(MyHandle));

	// todo: apply some better logic to this so that we don't recalculate stacking effects as often
	ActiveGameplayEffects.bNeedToRecalculateStacks = true;
	
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
	
	// Execute if this is an instant application effect or if it is a periodic effect (this would be the first execution of a repeating effect)
	if (Spec.GetDuration() == UGameplayEffect::INSTANT_APPLICATION)
	{
		ExecuteGameplayEffect(*OurCopyOfSpec, FModifierQualifier(BaseQualifier).IgnoreHandle(MyHandle));
	}

	return MyHandle;
}

void UAttributeComponent::ExecutePeriodicEffect(FActiveGameplayEffectHandle	Handle)
{
	// Fixme: this will be the callback to execute periodic effects, but since the timer manager can't 
	// can't have multiple timers registered to a single function on a single object, we are just doing polling in Tick
	/*
	if (!ActiveGameplayEffects.ExecuteGameplayEffect(Handle))
	{
	}
	*/
}

void UAttributeComponent::ExecuteGameplayEffect(const FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext)
{
	// Should only ever execute effects that are instant application or periodic application
	// Effects with no period and that aren't instant application should never be executed
	check( (Spec.GetDuration() == UGameplayEffect::INSTANT_APPLICATION || Spec.GetPeriod() != UGameplayEffect::NO_PERIOD) );
	
	ActiveGameplayEffects.ExecuteActiveEffectsFrom(Spec, QualifierContext);
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
		GameplayCueInterface->GameplayCueExecuted(CueInfo.GameplayCueTags, NormalizedMagnitude);
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
		GameplayCueInterface->GameplayCueActivated(CueInfo.GameplayCueTags, NormalizedMagnitude);
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
		GameplayCueInterface->GameplayCueAdded(CueInfo.GameplayCueTags, NormalizedMagnitude);
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
		GameplayCueInterface->GameplayCueRemoved(CueInfo.GameplayCueTags, NormalizedMagnitude);
	}
}

void UAttributeComponent::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UAttributeComponent, ActiveGameplayEffects);
}

void UAttributeComponent::OnRep_GameplayEffects()
{

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
	SKILL_LOG(Log, TEXT("NextExecuteTime: %.2f"), NextExecuteTime);
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

void UAttributeComponent::TEMP_TimerTest()
{
	GetWorld()->GetTimerManager().SetTimer(FTimerDelegate::CreateUObject(this, &UAttributeComponent::TEMP_TimerTestCallback, 1), 1.0, false);
	GetWorld()->GetTimerManager().SetTimer(FTimerDelegate::CreateUObject(this, &UAttributeComponent::TEMP_TimerTestCallback, 2), 1.0, false);
}

void UAttributeComponent::TEMP_TimerTestCallback(int32 x)
{
	SKILL_LOG(Log, TEXT("TEMP_TimerTestCallback: %d"), x);
}

void UAttributeComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsTick);
	SKILL_LOG_SCOPE(TEXT("Ticking %s"), *GetName());

	// This should be tmep until timer manager stuff is figured out
	ActiveGameplayEffects.TEMP_TickActiveEffects(DeltaTime);
}

#undef LOCTEXT_NAMESPACE
