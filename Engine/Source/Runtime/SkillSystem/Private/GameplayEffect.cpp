// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"

const float UGameplayEffect::INFINITE_DURATION = -1.f;
const float UGameplayEffect::INSTANT_APPLICATION = 0.f;
const float UGameplayEffect::NO_PERIOD = 0.f;
const float FGameplayEffectLevelSpec::INVALID_LEVEL = -1.f;

#if SKILL_SYSTEM_AGGREGATOR_DEBUG
FAggregator::FAllocationStats FAggregator::AllocationStats;
#endif

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayEffect
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


UGameplayEffect::UGameplayEffect(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

}

void UGameplayEffect::GetOwnedGameplayTags(TSet<FName>& OutTags) const
{
	return OwnedTagsContainer.GetTags(OutTags);
}

bool UGameplayEffect::HasOwnedGameplayTag(FName TagToCheck) const
{
	return OwnedTagsContainer.HasTag(TagToCheck);
}

void UGameplayEffect::GetClearGameplayTags(TSet<FName>& OutTags) const
{
	return ClearTagsContainer.GetTags(OutTags);
}

bool UGameplayEffect::AreApplicationTagRequirementsSatisfied(const TSet<FName>& InstigatorTags, const TSet<FName>& TargetTags) const
{
	TSet<FName> InstigatorReqs;
	ApplicationRequiredInstigatorTags.GetTags(InstigatorReqs);

	TSet<FName> TargetReqs;
	ApplicationRequiredTargetTags.GetTags(TargetReqs);

	return (InstigatorTags.Includes(InstigatorReqs) && TargetTags.Includes(TargetReqs));
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayEffectSpec
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FGameplayEffectSpec::InitModifiers(const FGlobalCurveDataOverride *CurveData)
{
	check(Def);

	Modifiers.Reserve(Def->Modifiers.Num());
	
	for (const FGameplayModifierInfo &ModInfo : Def->Modifiers)
	{
		// This creates a new FModifierSpec that we own.
		// The modifier level instance is shared through a shared pointer though
		Modifiers.Emplace( FModifierSpec(ModInfo, ModifierLevel, CurveData));	
	}	
}

void FGameplayEffectSpec::MakeUnique()
{
	for (FModifierSpec &ModSpec : Modifiers)
	{
		ModSpec.Aggregator.MakeUnique();
	}
}

int32 FGameplayEffectSpec::ApplyModifiersFrom(const FGameplayEffectSpec &InSpec, const FModifierQualifier &QualifierContext)
{
	int32 NumApplied = 0;
	bool ShouldSnapshot = InSpec.ShouldApplyAsSnapshot(QualifierContext);

	if (InSpec.Def && !InSpec.Def->AreGameplayEffectTagRequirementsSatisfied(Def))
	{
		return 0;
	}

	// Fixme: Use acceleration structures to speed up these types of lookups
	for (const FModifierSpec &InMod : InSpec.Modifiers)
	{
		if (!InMod.CanModifyInContext(QualifierContext))
		{
			continue;
		}

		switch (InMod.Info.EffectType)
		{
			case EGameplayModEffect::Magnitude:
			{
				for (FModifierSpec &MyMod : Modifiers)
				{
					if (InMod.CanModifyModifier(MyMod, QualifierContext))
					{
						InMod.ApplyModTo(MyMod, ShouldSnapshot);
						Def->OwnedTagsContainer.AppendTags(InMod.Info.PassedTags);
						NumApplied++;
					}
					else
					{
						NumApplied += ApplyModifiersToAggregators(InMod, MyMod.Aggregator, QualifierContext, ShouldSnapshot);
					}
				}
				break;
			}

			// Fixme: Duration mods aren't checking that attributes match - do we care?
			// eg - "I mod duration of all GEs that modify Health" would need to check to see if this mod modifies health before doing the stuff below.
			// Or can we get by with just tags?

			case EGameplayModEffect::Duration:
			{
				Duration.Get()->ApplyMod(InMod.Info.ModifierOp, InMod.Aggregator, ShouldSnapshot);
				Def->OwnedTagsContainer.AppendTags(InMod.Info.PassedTags);
				NumApplied++;
				break;
			}
		}
	}

	return NumApplied;
}

int32 FGameplayEffectSpec::ApplyModifiersToAggregators(const FModifierSpec &InMod, FAggregatorRef &MyMod, const FModifierQualifier &QualifierContext, bool ShouldSnapshot)
{
	int32 NumApplied = 0;
	for (int32 Op = 0; Op < EGameplayModOp::Max; ++Op)
	{
		for (FAggregatorRef SubMod : MyMod.Get()->Mods[Op])
		{
			if (InMod.CanModifyAggregator(SubMod, QualifierContext))
			{
				SubMod.Get()->ApplyMod(InMod.Info.ModifierOp, InMod.Aggregator, ShouldSnapshot);
				SubMod.Get()->BaseData.Tags.AppendTags(InMod.Info.PassedTags);
				NumApplied++;
			}
			else
			{
				NumApplied += ApplyModifiersToAggregators(InMod, SubMod, QualifierContext, ShouldSnapshot);
			}
		}
	}

	return NumApplied;
}

int32 FGameplayEffectSpec::ExecuteModifiersFrom(const FGameplayEffectSpec &InSpec, const FModifierQualifier &QualifierContext)
{
	int32 NumExecuted = 0;

	// Fixme: Use acceleration structures to speed up these types of lookups
	for (FModifierSpec &MyMod : Modifiers)
	{
		for (const FModifierSpec &InMod : InSpec.Modifiers)
		{
			if (InMod.CanModifyModifier(MyMod, QualifierContext))
			{
				InMod.ExecuteModOn(MyMod);
				NumExecuted++;
			}
		}
	}

	return NumExecuted;
}

float FGameplayEffectSpec::GetDuration() const
{
	return Duration.Get()->Evaluate().Magnitude;
}

float FGameplayEffectSpec::GetPeriod() const
{
	return Period.Get()->Evaluate().Magnitude;
}

bool FGameplayEffectSpec::ShouldApplyAsSnapshot(const FModifierQualifier &QualifierContext) const
{
	bool ShouldSnapshot;
	switch(Def->CopyPolicy)
	{
		case EGameplayEffectCopyPolicy::AlwaysSnapshot:
			ShouldSnapshot = true;
			break;
			
		case EGameplayEffectCopyPolicy::AlwaysLink:
			ShouldSnapshot = false;
			break;
			
		default:
			ShouldSnapshot = (QualifierContext.Type() == EGameplayMod::OutgoingGE);
			break;
	}
	
	return ShouldSnapshot;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FModifierSpec
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

bool FModifierSpec::CanModifyInContext(const FModifierQualifier &QualifierContext) const
{
	// Can only modify if are valid within this Qualifier Context
	// (E.g, if I am an OutgoingGE mod, I cannot modify during a IncomingGE context)
	if (Info.ModifierType != QualifierContext.Type())
	{
		return false;
	}	

	return true;
}

bool FModifierSpec::CanModifyModifier(FModifierSpec &Other, const FModifierQualifier &QualifierContext) const
{
	// Attribute is essentially a key. This isn't 100% necessary - we could just rely on tags
	if (Info.Attribute != Other.Info.Attribute)
	{
		return false;
	}

	if (!AreTagRequirementsSatisfied(Other))
	{
		return false;
	}

	return true;
}

bool FModifierSpec::CanModifyAggregator(FAggregatorRef Ref, const FModifierQualifier &QualifierContext) const
{
	if (!AreTagRequirementsSatisfied(Ref))
	{
		return false;
	}

	return true;
}

void FModifierSpec::ApplyModTo(FModifierSpec &Other, bool TakeSnapshot) const
{
	Other.Aggregator.Get()->ApplyMod(this->Info.ModifierOp, this->Aggregator, TakeSnapshot);
}

void FModifierSpec::ExecuteModOn(FModifierSpec &Other) const
{
	SKILL_LOG_SCOPE(TEXT("Executing %s on %s"), *ToSimpleString(), *Other.ToSimpleString() );
	Other.Aggregator.Get()->ExecuteModAggr(this->Info.ModifierOp, this->Aggregator);
}

bool FModifierSpec::AreTagRequirementsSatisfied(const FModifierSpec &ModifierToBeModified) const
{
	bool HasRequired = ModifierToBeModified.Info.OwnedTags.HasAllTags(Info.RequiredTags);
	bool HasIgnored = ModifierToBeModified.Info.OwnedTags.Num() > 0 && ModifierToBeModified.Info.OwnedTags.HasAnyTag(Info.IgnoreTags);

	return HasRequired && !HasIgnored;
}

bool FModifierSpec::AreTagRequirementsSatisfied(const FAggregatorRef &AggregatorToBeModified) const
{
	const FAggregator* Aggregator = AggregatorToBeModified.Get();
	if (Aggregator)
	{
		bool HasRequired = Aggregator->BaseData.Tags.HasAllTags(Info.RequiredTags);
		bool HasIgnored = Aggregator->BaseData.Tags.Num() > 0 && Aggregator->BaseData.Tags.HasAnyTag(Info.IgnoreTags);

		return HasRequired && !HasIgnored;
	}

	return false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayEffectInstigatorContext
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FGameplayEffectInstigatorContext::AddInstigator(class AActor *InInstigator)
{
	Instigator = InInstigator;

	// Cache off his attribute component.
	if (Instigator)
	{
		TArray<UObject*> ChildObjects;
		GetObjectsWithOuter(Instigator, ChildObjects);

		for (UObject * Obj : ChildObjects)
		{
			if (Obj && Obj->GetClass()->IsChildOf(UAttributeComponent::StaticClass()))
			{
				InstigatorAttributeComponent = Cast<UAttributeComponent>(Obj);
				break;
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FAggregatorRef
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FAggregatorRef::MakeUnique()
{
	SKILL_LOG(Log, TEXT("MakeUnique %s"), *ToString());

	// Make a hard ref copy of our FAggregator
	MakeHardRef();
	SharedPtr = TSharedPtr<FAggregator>(new FAggregator(*SharedPtr.Get()));
	WeakPtr = SharedPtr;

	// Update dependancy chain so that the copy we just made is updated if any of the applied modifiers change
	SharedPtr->RefreshDependencies();
}

void FAggregatorRef::MakeUniqueDeep()
{
	SKILL_LOG_SCOPE(TEXT("MakeUniqueDeep %s"), *ToString());

	// Make a hard ref copy of our FAggregator
	MakeHardRef();
	SharedPtr = TSharedPtr<FAggregator>(new FAggregator(*SharedPtr.Get()));
	WeakPtr = SharedPtr;

	// Make all of our mods UniqueDeep
	Get()->MakeUniqueDeep();

	// Update dependancy chain so that the copy we just made is updated if any of the applied modifiers change
	SharedPtr->RefreshDependencies();
}

FString FAggregatorRef::ToString() const
{
	if (SharedPtr.IsValid())
	{
		return FString::Printf(TEXT("[HardRef to: %s]"), *SharedPtr->ToSimpleString());
	}
	if (WeakPtr.IsValid())
	{
		return FString::Printf(TEXT("[SoftRef to: %s]"), *WeakPtr.Pin()->ToSimpleString());
	}
	return FString(TEXT("Invalid"));
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FAggregator
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

FAggregator::FAggregator()
{
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	CopiesMade = 0;
	FAggregator::AllocationStats.DefaultCStor++;
#endif
}

FAggregator::FAggregator(const FGameplayModifierData &InBaseData, TSharedPtr<FGameplayEffectLevelSpec> InLevel, const TCHAR *InDebugStr)
: Level(InLevel)
, BaseData(InBaseData)
{
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	if (InDebugStr)
	{
		DebugString = FString(InDebugStr);
	}
	CopiesMade = 0;
	FAggregator::AllocationStats.ModifierCStor++;
#endif
}

FAggregator::FAggregator(const FScalableFloat &InBaseMagnitude, TSharedPtr<FGameplayEffectLevelSpec> LevelInfo, const TCHAR *InDebugStr)
: Level(LevelInfo)
, BaseData(InBaseMagnitude)
{
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	if (InDebugStr)
	{
		DebugString = FString(InDebugStr);
	}
	CopiesMade = 0;
	FAggregator::AllocationStats.ScalableFloatCstor++;
#endif
}

FAggregator::FAggregator(const FGameplayModifierEvaluatedData &InEvalData, const TCHAR *InDebugStr)
: Level(TSharedPtr<FGameplayEffectLevelSpec>(new FGameplayEffectLevelSpec()))
, BaseData(InEvalData.Magnitude, InEvalData.Callbacks, &InEvalData.Tags)
{
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	if (InDebugStr)
	{
		DebugString = FString(InDebugStr);
	}
	CopiesMade = 0;
	FAggregator::AllocationStats.FloatCstor++;
#endif
}

FAggregator::FAggregator(const FAggregator &In)
{
	*this = In;
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
	FAggregator::AllocationStats.CopyCstor++;
	CopiesMade = 0;
	DebugString = FString::Printf(TEXT("Copy %d of [%s]"), ++In.CopiesMade, *In.DebugString);
#endif
} 

FAggregator::~FAggregator()
{
	for (TWeakPtr<FAggregator> WeakPtr : Dependants)
	{
		if (WeakPtr.IsValid())
		{
			SKILL_LOG(Log, TEXT("%s Marking Dependant %s Dirty on Destroy"), *ToSimpleString(), *WeakPtr.Pin()->ToSimpleString());

			WeakPtr.Pin()->MarkDirty();
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
			FAggregator::AllocationStats.DependantsUpdated++;
#endif
		}
	}
}

void FAggregator::RefreshDependencies()
{
	TWeakPtr<FAggregator> LocalWeakPtr(SharedThis(this));
	for (int32 i = 0; i < EGameplayModOp::Max; ++i)
	{
		for (FAggregatorRef &Ref : Mods[i])
		{
			if (Ref.IsValid())
			{
				Ref.Get()->AddDependantAggregator(LocalWeakPtr);
			}
		}
	}
}

void FAggregator::ApplyMod(EGameplayModOp::Type ModType, FAggregatorRef Ref, bool TakeSnapshot)
{
	if (TakeSnapshot)
	{
		Ref.MakeUniqueDeep();
	}
	else
	{
		Ref.MakeSoftRef();
	}
	Mods[ModType].Push(Ref);

	// Make the ref tell us if it changes
	if (HasBeenAlreadyMadeSharable())
	{
		Ref.Get()->AddDependantAggregator(TWeakPtr< FAggregator >(SharedThis(this)));
	}
	MarkDirty();
}

// Execute is intended to directly modify the base value of an aggregator.
// However if the aggregator's base value is not static (scales with some outside influece)
// we will treat this execute as an apply, but make our own copy of the passed in aggregator.
// (so that it will essentially be permanent).
void FAggregator::ExecuteModAggr(EGameplayModOp::Type ModType, FAggregatorRef Ref)
{
	const FGameplayModifierEvaluatedData& EvaluatedData = Ref.Get()->Evaluate();
	ExecuteMod(ModType, EvaluatedData);	
}

void FAggregator::ExecuteMod(EGameplayModOp::Type ModType, const FGameplayModifierEvaluatedData& EvaluatedData)
{
	// Am I allowed to even be executed on? If my base data scales then all I can do is apply this to myself
	if (!BaseData.Magnitude.IsStatic())
	{
		SKILL_LOG(Log, TEXT("Treating Execute as an Apply since our Level is not static!"));
		FAggregatorRef NewAggregator(new FAggregator(EvaluatedData, SKILL_AGG_DEBUG(TEXT("ExecutedMod"))));
		ApplyMod(ModType, NewAggregator, false);
		return;
	}

	switch (ModType)
	{
		case EGameplayModOp::Override:
		{
			BaseData.Magnitude.SetValue(EvaluatedData.Magnitude);
			BaseData.Tags = EvaluatedData.Tags;
			break;
		}
		case EGameplayModOp::Additive:
		{
			BaseData.Magnitude.Value += EvaluatedData.Magnitude;
			BaseData.Tags.AppendTags(EvaluatedData.Tags);
			break;
		}
		case EGameplayModOp::Multiplicitive:
		{
			BaseData.Magnitude.Value *= EvaluatedData.Magnitude;
			BaseData.Tags.AppendTags(EvaluatedData.Tags);
			break;
		}
		case EGameplayModOp::Division:
		{
			BaseData.Magnitude.Value /= EvaluatedData.Magnitude;
			BaseData.Tags.AppendTags(EvaluatedData.Tags);
			break;
		}
		case EGameplayModOp::Callback:
		{
			// Executing a callback mod on another aggregator, what is expected here?
			check(false);
			break;
		}
	}

	SKILL_LOG(Log, TEXT("ExecuteMod: %s new BaseData.Magnitude: %s"), *ToSimpleString(), *BaseData.Magnitude.ToSimpleString());
	MarkDirty();
}

void FAggregator::AddDependantAggregator(TWeakPtr<FAggregator> InDependant)
{
	check(InDependant.IsValid());
	check(!Dependants.Contains(InDependant));

	SKILL_LOG(Log, TEXT("AddDependantAggregator: %s is a dependant of %s"), *InDependant.Pin()->ToSimpleString(), *ToSimpleString());
	Dependants.Add(InDependant);
}

void FAggregator::TakeSnapshotOfLevel()
{
	check(Level.IsValid());
	Level = TSharedPtr<FGameplayEffectLevelSpec>( new FGameplayEffectLevelSpec( *Level.Get()) );
}

// Please try really hard to never add a "force full re-evaluate" flag to this function!
// We want to strive to make this system dirty the cached data when its actually dirtied, 
// and never do full catch all rebuilds.

const FGameplayModifierEvaluatedData& FAggregator::Evaluate() const
{
	SKILL_LOG_SCOPE(TEXT("Aggregator Evaluate %s"), *ToSimpleString());

	if (!CachedData.IsValid)
	{
		CachedData.IsValid = true;

		// If there are any overrides, then just take the first valid one.
		for (const FAggregatorRef &Agg : Mods[EGameplayModOp::Override])
		{
			SKILL_LOG_SCOPE(TEXT("EGameplayModOp::Override"));
			if (Agg.IsValid())
			{
				CachedData = Agg.Get()->Evaluate();
				return CachedData;
			}
		}

		// If we are going to do math, we need to lock our base value in.
		// Calculate our magnitude at our level
		// (We may not have a level, in that case, ensure we also don't have a leveling table)
		
		float EvaluatedMagnitude = 0.f;
		if (Level->IsValid())
		{
			EvaluatedMagnitude = BaseData.Magnitude.GetValueAtLevel(Level->GetLevel());
		}
		else
		{
			EvaluatedMagnitude = BaseData.Magnitude.GetValueChecked();
		}

		CachedData = FGameplayModifierEvaluatedData(EvaluatedMagnitude, BaseData.Callbacks, ActiveHandle, &BaseData.Tags);
		SKILL_LOG(Log, TEXT("BaseMagnitude: %.2f"), CachedData.Magnitude);

		// Additive: Just sum them up
		for (const FAggregatorRef &Agg : Mods[EGameplayModOp::Additive])
		{
			SKILL_LOG_SCOPE(TEXT("EGameplayModOp::Additive"));
			if (Agg.IsValid())
			{
				Agg.Get()->Evaluate().Aggregate( CachedData.Tags, CachedData.Magnitude, 0.f );
			}
		}		

		// Multiple - sum the multiplication with a bias then multiply it at the end
		float Factor = 1.f;
		for (const FAggregatorRef &Agg : Mods[EGameplayModOp::Multiplicitive])
		{
			SKILL_LOG_SCOPE(TEXT("EGameplayModOp::Multiplicitive"));
			if (Agg.IsValid())
			{
				Agg.Get()->Evaluate().Aggregate( CachedData.Tags, Factor, 1.f );
			}
		}
		CachedData.Magnitude *= Factor;

		// Divisor - sum the division with a bias then divide it at the end
		Factor = 1.f;
		for (const FAggregatorRef &Agg : Mods[EGameplayModOp::Division])
		{
			SKILL_LOG_SCOPE(TEXT("EGameplayModOp::Division"));
			if (Agg.IsValid())
			{
				Agg.Get()->Evaluate().Aggregate(CachedData.Tags, Factor, 1.f);
			}
		}
		CachedData.Magnitude /= Factor;

		SKILL_LOG(Log, TEXT("Final Magnitude: %.2f"), CachedData.Magnitude);
		
	}
	else
	{
		SKILL_LOG(Log, TEXT("CachedData was valid. Magnitude: %.2f"), CachedData.Magnitude);
	}
	return CachedData;
}

void FAggregator::PreEvaluate(FGameplayEffectModCallbackData &Data) const
{
	check(Data.ModifierSpec.Aggregator.Get() == this);
	for (const FAggregatorRef &Agg : Mods[EGameplayModOp::Callback])
	{
		if (Agg.IsValid())
		{
			Agg.Get()->Evaluate().InvokePreExecute(Data);
		}
	}
}

void FAggregator::PostEvaluate(const struct FGameplayEffectModCallbackData &Data) const
{
	check(Data.ModifierSpec.Aggregator.Get() == this);
	for (const FAggregatorRef &Agg : Mods[EGameplayModOp::Callback])
	{
		if (Agg.IsValid())
		{
			Agg.Get()->Evaluate().InvokePostExecute(Data);
		}
	}
}

FAggregator & FAggregator::MarkDirty()
{
	CachedData.IsValid = false;
	for (int32 i=0; i < Dependants.Num(); ++i)
	{
		TWeakPtr<FAggregator> WeakPtr = Dependants[i];
		if (WeakPtr.IsValid())
		{
			SKILL_LOG(Log, TEXT("%s Marking Dependant %s Dirty (from ::MarkDirty())"), *ToSimpleString(), *WeakPtr.Pin()->ToSimpleString());
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
			FAggregator::AllocationStats.DependantsUpdated++;
#endif
			WeakPtr.Pin()->MarkDirty();
		}
		else
		{
			Dependants.RemoveAtSwap(i);
			--i;
		}
	}
	OnDirty.ExecuteIfBound(this);

	return *this;
}

void FAggregator::MakeUniqueDeep()
{
	for (int32 i = 0; i < EGameplayModOp::Max; ++i)
	{
		for (FAggregatorRef &Ref : Mods[i])
		{
			if (Ref.IsValid())
			{
				Ref.MakeUniqueDeep();
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayModifierEvaluatedData
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FGameplayModifierEvaluatedData::Aggregate(FGameplayTagContainer &OutTags, float &OutMagnitude, const float Bias) const
{
	OutMagnitude += (Magnitude - Bias);
	OutTags.AppendTags(Tags);
}

void FGameplayModifierEvaluatedData::InvokePreExecute(FGameplayEffectModCallbackData &Data) const
{
	if (Callbacks)
	{
		for (TSubclassOf<class UGameplayEffectExtension> ExtClass : Callbacks->ExtensionClasses)
		{
			if (ExtClass)
			{
				UGameplayEffectExtension * Ext = ExtClass->GetDefaultObject<UGameplayEffectExtension>();
				Ext->PreGameplayEffectExecute(*this, Data);
			}
		}
	}
}

void FGameplayModifierEvaluatedData::InvokePostExecute(const FGameplayEffectModCallbackData &Data) const
{
	if (Callbacks)
	{
		for (TSubclassOf<class UGameplayEffectExtension> ExtClass : Callbacks->ExtensionClasses)
		{
			if (ExtClass)
			{
				UGameplayEffectExtension * Ext = ExtClass->GetDefaultObject<UGameplayEffectExtension>();
				Ext->PostGameplayEffectExecute(*this, Data);
			}
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayModifierEvaluatedData
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FActiveGameplayEffect::PreReplicatedRemove(const struct FActiveGameplayEffectsContainer &InArray)
{
	InArray.Owner->InvokeGameplayCueRemoved(Spec);
}

void FActiveGameplayEffect::PostReplicatedAdd(const struct FActiveGameplayEffectsContainer &InArray)
{
	static const float MAX_DELTA_TIME = 3.f;

	InArray.Owner->InvokeGameplayCueAdded(Spec);
	// Was this actually just activated, or are we just finding out about it due to relevancy/join in progress?
	float WorldTimeSeconds = InArray.Owner->GetWorld()->GetTimeSeconds();
	if (InArray.Owner->GetWorld()->GetGameState<AGameState>())
	{
		WorldTimeSeconds = InArray.Owner->GetWorld()->GetGameState<AGameState>()->ElapsedTime;
	}
	float DeltaTime = WorldTimeSeconds - StartTime;

	if (WorldTimeSeconds > 0.f && FMath::Abs(DeltaTime) < MAX_DELTA_TIME)
	{
		InArray.Owner->InvokeGameplayCueActivated(Spec);
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FActiveGameplayEffectsContainer
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FActiveGameplayEffectsContainer::ApplyActiveEffectsTo(OUT FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext) const
{
	FActiveGameplayEffectHandle().IsValid();

	SKILL_LOG_SCOPE(TEXT("ApplyActiveEffectsTo: %s %s"), *Spec.ToSimpleString(), *QualifierContext.ToString());
	for (const FActiveGameplayEffect & ActiveEffect : GameplayEffects)
	{
		// We dont want to use FModifierQualifier::TestTarget here, since we aren't the 'target'. We are applying stuff to Spec which will be applied to a target.
		if (QualifierContext.IgnoreHandle().IsValid() && QualifierContext.IgnoreHandle() == ActiveEffect.Handle)
		{
			continue;
		}
		Spec.ApplyModifiersFrom( ActiveEffect.Spec, QualifierContext );
	}
}

/** This is the main function that applies/attaches a GameplayEffect on Attributes and ActiveGameplayEffects */
void FActiveGameplayEffectsContainer::ApplySpecToActiveEffectsAndAttributes(const FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext)
{
	for (const FModifierSpec &Mod : Spec.Modifiers)
	{
		if (Mod.Info.ModifierType == EGameplayMod::Attribute)
		{
			// Todo: Tag/application checks here

			SKILL_LOG_SCOPE(TEXT("Applying Attribute Mod %s to property"), *Mod.ToSimpleString());

			FAggregatorRef *RefPtr = OngoingPropertyEffects.Find(Mod.Info.Attribute);
			if (!RefPtr)
			{
				// Create a new aggregator for this attribute.

				float CurrentValueOfProperty = Owner->GetNumericAttribute(Mod.Info.Attribute);
				SKILL_LOG(Log, TEXT("Creating new entry in OngoingPropertyEffect map. CurrentValue: %.2f"), CurrentValueOfProperty);

				FAggregator *NewPropertyAggregator = new FAggregator(FGameplayModifierEvaluatedData(CurrentValueOfProperty), SKILL_AGG_DEBUG(TEXT("Attribute %s Aggregator"), *Mod.Info.Attribute.GetName()));

				NewPropertyAggregator->OnDirty = FAggregator::FOnDirty::CreateRaw(this, &FActiveGameplayEffectsContainer::OnPropertyAggregatorDirty, Mod.Info.Attribute);

				RefPtr = &OngoingPropertyEffects.Add(Mod.Info.Attribute, FAggregatorRef(NewPropertyAggregator));
			}

			// Add the modifier to the property value aggregator
			// Note that this will immediately invoke the callback to update the attribute's current value, so we don't have to explicitly do it here.
			RefPtr->Get()->ApplyMod(Mod.Info.ModifierOp, Mod.Aggregator, Spec.ShouldApplyAsSnapshot(QualifierContext));
		}

		if (Mod.Info.ModifierType == EGameplayMod::ActiveGE)
		{
			SKILL_LOG_SCOPE(TEXT("Applying AttributeMod %s to ActiveEffects"),  *Mod.ToSimpleString(), *Mod.Info.Attribute.GetName());

			// TODO: Tag checks here

			// This modies GEs that are currently active, so apply this to them...
			for (FActiveGameplayEffect & ActiveEffect : GameplayEffects)
			{
				if (!QualifierContext.TestTarget(ActiveEffect.Handle))
				{
					continue;
				}

				ActiveEffect.Spec.ApplyModifiersFrom( Spec, FModifierQualifier().Type(EGameplayMod::ActiveGE) );
			}
		}
	}
}

/** This is the main function that executes a GameplayEffect on Attributes and ActiveGameplayEffects */
void FActiveGameplayEffectsContainer::ExecuteActiveEffectsFrom(const FGameplayEffectSpec &Spec, const FModifierQualifier &QualifierContext)
{
	bool InvokeGameplayCueExecute = false;

	for (const FModifierSpec &Mod : Spec.Modifiers)
	{
		if (Mod.Info.ModifierType == EGameplayMod::Attribute)
		{
			UAttributeSet * AttributeSet = Owner->GetAttributeSubobject(Mod.Info.Attribute.GetAttributeSetClass());
			if (AttributeSet == NULL)
			{
				// Our owner doesn't have this attribute, so we can't do anything
				SKILL_LOG(Log, TEXT("%s does not have attribute %s. Skipping modifier"), *Owner->GetPathName(), *Mod.Info.Attribute.GetName());
				continue;
			}

			// Todo: Tags/application checks here - make sure we can still apply
			InvokeGameplayCueExecute = true;

			SKILL_LOG_SCOPE(TEXT("Executing Attribute Mod %s"), *Mod.ToSimpleString());

			// First, evaluate all of our data 

			FGameplayModifierEvaluatedData EvaluatedData = Mod.Aggregator.Get()->Evaluate();

			FGameplayEffectModCallbackData ExecuteData(Spec, Mod, EvaluatedData, *Owner);
						
			/** This should apply 'gameplay effect specific' rules, such as life steal, shields, etc */
			Mod.Aggregator.Get()->PreEvaluate(ExecuteData);

			/** This should apply 'gamewide' rules. Such as clamping Health to MaxHealth or granting +3 health for every point of strength, etc */
			AttributeSet->PreAttributeModify(ExecuteData);

			// Do we have active GE's that are already modifying this?
			FAggregatorRef *RefPtr = OngoingPropertyEffects.Find(Mod.Info.Attribute);
			if (RefPtr)
			{
				SKILL_LOG(Log, TEXT("Property %s has active mods. Adding to Aggregator."), *Mod.Info.Attribute.GetName());
				RefPtr->Get()->ExecuteMod(Mod.Info.ModifierOp, EvaluatedData);
			}
			else
			{
				// Modify the property inplace, without putting it in the OngoingPropertyEffects map
				float		CurrentValueOfProperty = Owner->GetNumericAttribute(Mod.Info.Attribute);
				FAggregator	Aggregator(CurrentValueOfProperty, SKILL_AGG_DEBUG(TEXT("Inplace Attribute %s"), *Mod.Info.Attribute.GetName()));

				Aggregator.ExecuteMod(Mod.Info.ModifierOp, EvaluatedData);
				
				const float NewPropertyValue = Aggregator.Evaluate().Magnitude;

				SKILL_LOG(Log, TEXT("Property %s new value is: %.2f [was: %.2f]"), *Mod.Info.Attribute.GetName(), NewPropertyValue, CurrentValueOfProperty);
				Owner->SetNumericAttribute(Mod.Info.Attribute, NewPropertyValue);
			}

			/** This should apply 'gameplay effect specific' rules, such as life steal, shields, etc */
			Mod.Aggregator.Get()->PostEvaluate(ExecuteData);

			/** This should apply 'gamewide' rules. Such as clamping Health to MaxHealth or granting +3 health for every point of strength, etc */
			AttributeSet->PostAttributeModify(ExecuteData);
			
		}
		else if(Mod.Info.ModifierType == EGameplayMod::ActiveGE)
		{
			SKILL_LOG_SCOPE(TEXT("Executing Attribute Mod %s on ActiveEffects"), *Mod.ToSimpleString());

			// Todo: Tag checks here

			// This modies GEs that are currently active, so apply this to them...
			for (FActiveGameplayEffect & ActiveEffect : GameplayEffects)
			{
				// Don't apply spec to itself
				if (!QualifierContext.TestTarget(ActiveEffect.Handle))
				{
					continue;
				}

				if (ActiveEffect.Spec.ExecuteModifiersFrom( Spec, FModifierQualifier().Type(EGameplayMod::ActiveGE) ) > 0)
				{
					InvokeGameplayCueExecute = true;
				}
			}
		}
	}

	if (InvokeGameplayCueExecute)
	{
		// TODO: check replication policy. Right now we will replciate every execute via a multicast RPC

		SKILL_LOG(Log, TEXT("Invoking Execute GameplayCue for %s"), *Spec.ToSimpleString() );
		Owner->NetMulticast_InvokeGameplayCueExecuted(Spec);
	}

}

bool FActiveGameplayEffectsContainer::ExecuteGameplayEffect(FActiveGameplayEffectHandle Handle)
{
	// Could make this a map for quicker lookup
	for (int32 idx = 0; idx < GameplayEffects.Num(); ++idx)
	{
		if (GameplayEffects[idx].Handle == Handle)
		{
			//GameplayEffects.
			return true;
		}
	}

	return false;
}

float FActiveGameplayEffectsContainer::GetGameplayEffectDuration(FActiveGameplayEffectHandle Handle) const
{
	// Could make this a map for quicker lookup
	for (int32 idx = 0; idx < GameplayEffects.Num(); ++idx)
	{
		if (GameplayEffects[idx].Handle == Handle)
		{
			return GameplayEffects[idx].GetDuration();
		}
	}
	
	SKILL_LOG(Warning, TEXT("GetGameplayEffectDuration called with invalid Handle: %s"), *Handle.ToString() );
	return UGameplayEffect::INFINITE_DURATION;
}

float FActiveGameplayEffectsContainer::GetGameplayEffectMagnitude(FActiveGameplayEffectHandle Handle, FGameplayAttribute Attribute) const
{
	// Could make this a map for quicker lookup
	for (FActiveGameplayEffect Effect : GameplayEffects)
	{
		if (Effect.Handle == Handle)
		{
			for (const FModifierSpec &Mod : Effect.Spec.Modifiers)
			{
				if (Mod.Info.Attribute == Attribute)
				{
					return Mod.Aggregator.Get()->Evaluate().Magnitude;
				}
			}
		}
	}

	SKILL_LOG(Warning, TEXT("GetGameplayEffectMagnitude called with invalid Handle: %s"), *Handle.ToString());
	return -1.f;
}

bool FActiveGameplayEffectsContainer::IsGameplayEffectActive(FActiveGameplayEffectHandle Handle) const
{
	// Could make this a map for quicker lookup
	for (FActiveGameplayEffect Effect : GameplayEffects)
	{
		if (Effect.Handle == Handle)
		{
			return true;
		}
	}
	return false;
}

float FActiveGameplayEffectsContainer::GetGameplayEffectMagnitudeByTag(FActiveGameplayEffectHandle Handle, FName InTagName) const
{
	// Could make this a map for quicker lookup
	for (FActiveGameplayEffect Effect : GameplayEffects)
	{
		if (Effect.Handle == Handle)
		{
			for (const FModifierSpec &Mod : Effect.Spec.Modifiers)
			{
				if (Mod.Info.OwnedTags.HasTag(InTagName))
				{
					return Mod.Aggregator.Get()->Evaluate().Magnitude;
				}
			}
		}
	}

	SKILL_LOG(Warning, TEXT("GetGameplayEffectMagnitude called with invalid Handle: %s"), *Handle.ToString());
	return -1.f;
}

void FActiveGameplayEffectsContainer::OnPropertyAggregatorDirty(FAggregator* Aggregator, FGameplayAttribute Attribute)
{
	check(OngoingPropertyEffects.FindChecked(Attribute).Get() == Aggregator);

	SKILL_LOG_SCOPE(TEXT("FActiveGameplayEffectsContainer::OnPropertyAggregatorDirty"));

	// Immediately calculate the newest value of the property			
	float NewPropertyValue = Aggregator->Evaluate().Magnitude;

	SKILL_LOG(Log, TEXT("Property %s new value is: %.2f"), *Attribute.GetName(), NewPropertyValue);
	
	Owner->SetNumericAttribute(Attribute, NewPropertyValue);
}

FActiveGameplayEffect & FActiveGameplayEffectsContainer::CreateNewActiveGameplayEffect(const FGameplayEffectSpec &Spec, float GameTimeSeconds)
{
	LastAssignedHandle = LastAssignedHandle.GetNextHandle();
	FActiveGameplayEffect & NewEffect = *new (GameplayEffects)FActiveGameplayEffect(LastAssignedHandle, Spec, GameTimeSeconds);
	
	MarkItemDirty(NewEffect);
	return NewEffect;
}

bool FActiveGameplayEffectsContainer::RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle)
{
	// Could make this a map for quicker lookup
	for (int32 idx = 0; idx < GameplayEffects.Num(); ++idx)
	{
		if (GameplayEffects[idx].Handle == Handle)
		{
			Owner->InvokeGameplayCueRemoved(GameplayEffects[idx].Spec);

			GameplayEffects.RemoveAtSwap(idx);
			MarkArrayDirty();
			return true;
		}
	}

	SKILL_LOG(Warning, TEXT("RemoveActiveGameplayEffect called with invalid Handle: %s"), *Handle.ToString());
	return false;
}

void FActiveGameplayEffectsContainer::RegisterAttributeModifyCallback(FGameplayAttribute Attribute, FOnGameplayAttributeEffectExecuted Delegate)
{
	PropertyExecutionCallbacks.FindOrAdd(Attribute) = Delegate;
}

bool FActiveGameplayEffectsContainer::IsNetAuthority() const
{
	check(Owner);
	return Owner->IsOwnerActorAuthoritative();
}

void FActiveGameplayEffectsContainer::TEMP_TickActiveEffects(float DeltaSeconds)
{
	if (!IsNetAuthority())
	{
		// For now - clients don't tick their GameplayEffects at all. 
		// We eventually want them to tick to predicate gameplay cue events.
		return;
	}


	float CurrentTime = Owner->GetWorld()->GetTimeSeconds();
	// if we have a game state use the ElapsedTime so client server time replication is more accurate else use the time seconds of the game world.
	if (Owner->GetWorld()->GetGameState<AGameState>())
	{
		CurrentTime = Owner->GetWorld()->GetGameState<AGameState>()->ElapsedTime;
	}

	for (int32 idx = 0; idx < GameplayEffects.Num(); ++idx)
	{
		FActiveGameplayEffect &Effect = GameplayEffects[idx];

		if (Effect.NextExecuteTime > 0)
		{
			float TimeOver = CurrentTime - Effect.NextExecuteTime;
			if (TimeOver >= -KINDA_SMALL_NUMBER)
			{
				// Advance
				Effect.AdvanceNextExecuteTime(CurrentTime);

				// Execute
				ExecuteActiveEffectsFrom( Effect.Spec, FModifierQualifier().IgnoreHandle(Effect.Handle) );
			}
		}

		float Duration = Effect.GetDuration();
		if (Duration > 0.f && Effect.StartTime + Effect.GetDuration() < CurrentTime)
		{
			Owner->InvokeGameplayCueRemoved(Effect.Spec);

			MarkArrayDirty();
			GameplayEffects.RemoveAtSwap(idx);
			idx--;
		}
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayEffectSystemManager
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	Misc
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

FString EGameplayModOpToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayModOp"));
	return e->GetEnum(Type).ToString();
}

FString EGameplayModToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayMod"));
	return e->GetEnum(Type).ToString();
}

FString EGameplayModEffectToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayModEffect"));
	return e->GetEnum(Type).ToString();
}

FString EGameplayEffectCopyPolicyToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayEffectCopyPolicy"));
	return e->GetEnum(Type).ToString();
}
