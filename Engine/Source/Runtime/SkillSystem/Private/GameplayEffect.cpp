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

FGameplayEffectSpec::FGameplayEffectSpec(const UGameplayEffect * InDef, TSharedPtr<FGameplayEffectLevelSpec> InLevel, const FGlobalCurveDataOverride *CurveData)
	: Def(InDef)
	, ModifierLevel(InLevel)
	, Duration(new FAggregator(InDef->Duration.MakeFinalizedCopy(CurveData), InLevel, SKILL_AGG_DEBUG(TEXT("%s Duration"), *InDef->GetName())))
	, Period(new FAggregator(InDef->Period.MakeFinalizedCopy(CurveData), InLevel, SKILL_AGG_DEBUG(TEXT("%s Period"), *InDef->GetName())))
	, StackingPolicy(EGameplayEffectStackingPolicy::Unlimited)
	, StackedAttribName(NAME_None)
{
	Duration.Get()->RegisterLevelDependancies();
	Period.Get()->RegisterLevelDependancies();

	// check the stacking policy for this gameplay effect and warn if it's set up badly
	if (Def->StackingPolicy != EGameplayEffectStackingPolicy::Unlimited)
	{
		StackingPolicy = Def->StackingPolicy;

		if (Def->Modifiers.Num() < 1)
		{
			SKILL_LOG(Warning, TEXT("%s has a stacking rule but does not have any modifiers to apply it to."), *Def->GetPathName());
			StackingPolicy = EGameplayEffectStackingPolicy::Unlimited;
		}
		else
		{
			UDataTable* DataTable = ISkillSystemModule::Get().GetSkillSystemGlobals().GetGlobalAttributeDataTable();

			if (Def->Modifiers[0].ModifierType != EGameplayMod::Attribute)
			{
				SKILL_LOG(Warning, TEXT("%s has a stacking rule but its first modifier does not modify an attribute."), *Def->GetPathName());
				StackingPolicy = EGameplayEffectStackingPolicy::Unlimited;
			}
			else
			{
				FName AttributeName(*Def->Modifiers[0].Attribute.GetName());
				
				if (DataTable && DataTable->RowMap.Contains(AttributeName))
				{
					FAttributeMetaData* Row = (FAttributeMetaData*)DataTable->RowMap[AttributeName];
					if (Row && Row->bCanStack == false)
					{
						SKILL_LOG(Warning, TEXT("%s has a stacking rule but modifies attribute %s. %s is not allowed to stack."), *Def->GetPathName(), *Def->Modifiers[0].Attribute.GetName(), *Def->Modifiers[0].Attribute.GetName());
						StackingPolicy = EGameplayEffectStackingPolicy::Unlimited;
					}
				}
				else
				{
					SKILL_LOG(Warning, TEXT("%s has a stacking rule but modifies attribute %s. %s was not found in the global attribute data table."), *Def->GetPathName(), *Def->Modifiers[0].Attribute.GetName(), *Def->Modifiers[0].Attribute.GetName());
					StackingPolicy = EGameplayEffectStackingPolicy::Unlimited;
				}
			}

			// look for other stacking attributes and warn that they will be ignored
			for (int32 Idx = 1; Idx < Def->Modifiers.Num(); ++Idx)
			{
				if (Def->Modifiers[Idx].ModifierType == EGameplayMod::Attribute)
				{
					FName AttributeName(*Def->Modifiers[0].Attribute.GetName());

					if (DataTable && DataTable->RowMap.Contains(AttributeName))
					{
						FAttributeMetaData* Row = (FAttributeMetaData*)DataTable->RowMap[AttributeName];
						if (Row && Row->bCanStack)
						{
							SKILL_LOG(Warning, TEXT("%s has a stacking rule and modifies attribute %s but %s is not the modified by the first modifier. Stacking is based on the first modifier."), *Def->GetPathName(), *Def->Modifiers[0].Attribute.GetName(), *Def->Modifiers[0].Attribute.GetName());
						}
					}
				}
			}
		}
		
		UDataTable* DataTable = ISkillSystemModule::Get().GetSkillSystemGlobals().GetGlobalAttributeDataTable();

		// check for other attributes that may allow stacking and warn that they will be ignored
		for (int32 Idx = 1; Idx < Def->Modifiers.Num(); ++Idx)
		{
			if (Def->Modifiers[Idx].ModifierType == EGameplayMod::Attribute)
			{
				FName AttributeName(*Def->Modifiers[Idx].Attribute.GetName());
				if (DataTable && DataTable->RowMap.Contains(AttributeName))
				{
					FAttributeMetaData* Row = (FAttributeMetaData*)DataTable->RowMap[AttributeName];
					if (Row && Row->bCanStack == true)
					{
						SKILL_LOG(Warning, TEXT("%s has a stacking rule and modifies more than one stackable attribute. Stacking will  not apply to attribute %s"), *Def->GetPathName(), *Def->Modifiers[Idx].Attribute.GetName());
					}
				}
			}
		}
	}

	InitModifiers(CurveData);
}

void FGameplayEffectSpec::InitModifiers(const FGlobalCurveDataOverride *CurveData)
{
	check(Def);

	Modifiers.Reserve(Def->Modifiers.Num());
	
	for (const FGameplayModifierInfo &ModInfo : Def->Modifiers)
	{
		// Pass down the LevelInfo into this Modifier. 
		// ModifierSpec may want to override how leveling will work (for this modifier only).
		// Or it may use the GameplayEffectSpec's level. 
		// ApplyNewDef will update NewLevelSpec appropriately.

		TSharedPtr<FGameplayEffectLevelSpec> NewLevelSpec = ModifierLevel;
		ModifierLevel->ApplyNewDef(ModInfo.LevelInfo, NewLevelSpec);

		// This creates a new FModifierSpec that we own.
		Modifiers.Emplace(FModifierSpec(ModInfo, NewLevelSpec, CurveData));
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
	SKILL_LOG_SCOPE(TEXT("FGameplayEffectSpec::ApplyModifiersFrom %s. InSpec: %s"), *this->ToSimpleString(), *InSpec.ToSimpleString());

	int32 NumApplied = 0;
	bool ShouldSnapshot = InSpec.ShouldApplyAsSnapshot(QualifierContext);

	if (InSpec.Def && !InSpec.Def->AreGameplayEffectTagRequirementsSatisfied(Def))
	{
		return 0;
	}

	// Fixme: Use acceleration structures to speed up these types of lookups
	// The called functions below are reliant on the InSpecs evaluated data. We should ideally only call evaluate once per mod.

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
						NumApplied++;
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
				NumApplied++;
				break;
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

float FGameplayEffectSpec::GetMagnitude(const FGameplayAttribute &Attribute) const
{
	float CurrentMagnitude = 0.f;

	for (const FModifierSpec &Mod : Modifiers)
	{
		if (Mod.Info.ModifierType == EGameplayMod::Attribute)
		{
			if (Mod.Info.Attribute != Attribute)
			{
				continue;
			}

			// Todo: Tags/application checks here - make sure we can still apply

			// First, evaluate all of our data 

			FGameplayModifierEvaluatedData EvaluatedData = Mod.Aggregator.Get()->Evaluate();

			FAggregator	Aggregator(CurrentMagnitude, SKILL_AGG_DEBUG(TEXT("Magnitude of Attribute %s"), *Mod.Info.Attribute.GetName()));

			Aggregator.ExecuteMod(Mod.Info.ModifierOp, EvaluatedData);

			CurrentMagnitude = Aggregator.Evaluate().Magnitude;
		}
	}

	return CurrentMagnitude;
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

EGameplayEffectStackingPolicy::Type FGameplayEffectSpec::GetStackingType() const
{
	return StackingPolicy;
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

	// Tag checking is done at the FAggregator level. So all we do here is the attribute check.

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
	const FGameplayModifierEvaluatedData & ToBeModifiedData = ModifierToBeModified.Aggregator.Get()->Evaluate();

	bool HasRequired = ToBeModifiedData.Tags.HasAllTags(this->Info.RequiredTags);
	bool HasIgnored = ToBeModifiedData.Tags.Num() > 0 && ToBeModifiedData.Tags.HasAnyTag(this->Info.IgnoreTags);

	return HasRequired && !HasIgnored;
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

bool FAggregatorRef::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	float EvaluatedMagnitude=0.f;

	if (Ar.IsSaving())
	{
		if (IsValid())
		{
			EvaluatedMagnitude = Get()->Evaluate().Magnitude;
		}
	}

	Ar << EvaluatedMagnitude;

	if (Ar.IsLoading())
	{
		if (!IsValid())
		{
			SharedPtr = TSharedPtr<struct FAggregator>(new FAggregator);
			WeakPtr = SharedPtr;
		}

		Get()->SetFromNetSerialize(EvaluatedMagnitude);
	}
	return true;
}

void FAggregator::SetFromNetSerialize(float SerializedValue)
{
	BaseData = FGameplayModifierData(SerializedValue, NULL);
	CachedData = FGameplayModifierEvaluatedData(SerializedValue);
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
, BaseData(InEvalData.Magnitude, InEvalData.Callbacks)
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
	
	RegisterLevelDependancies();
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

void FAggregator::RegisterLevelDependancies()
{
	if (!BaseData.Magnitude.IsStatic())
	{
		TWeakPtr<FAggregator> LocalWeakPtr(SharedThis(this));
		Level->RegisterLevelDependancy(LocalWeakPtr);
	}
}

// Please try really hard to never add a "force full re-evaluate" flag to this function!
// We want to strive to make this system dirty the cached data when its actually dirtied, 
// and never do full catch all rebuilds.

const FGameplayModifierEvaluatedData& FAggregator::Evaluate() const
{
	SKILL_LOG_SCOPE(TEXT("Aggregator Evaluate %s"), *ToSimpleString());

	if (!CachedData.IsValid)
	{
		// ------------------------------------------------------------------------
		// If there are any overrides, then just take the first valid one.
		// ------------------------------------------------------------------------
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

		int32 TotalModCount = Mods[EGameplayModOp::Additive].Num() + Mods[EGameplayModOp::Multiplicitive].Num() + Mods[EGameplayModOp::Division].Num();
		
		// Early out if no mods.
		// We need to calculate num of mods anyways to do ModLIst.Reserve.
		if (TotalModCount <= 0)
		{
			SKILL_LOG(Log, TEXT("Final Magnitude: %.2f"), CachedData.Magnitude);
			return CachedData;
		}

		// ------------------------------------------------------------------------
		//	Apply Numeric Modifiers
		//		This is convoluted due to tagging. We have modifiers that require we have or don't have certain tags.
		//		These mods can also give us new tags. Its possible the 1st mod will require a tag that the 2nd mod gives us.
		//
		//	The basic approach here is create an ordered, linear list of all modifiers:
		//		[Additive][Multipliticitive][Division] mods
		//
		//	We make a pass through the list, aggregating as we go. During a pass we keep track of what what tags we've added
		//	and if there were any mods that we rejected due to not having tags. When the pass is over, we check if we added
		//	any that we needed. If so, we make another pass. (Once a modifier aggregated, we remove it from the list).
		//
		//	Paradoxes are still possible. ModX gives tag A, requires we don't have tag B. ModY gives tag B, requires we don't have tag A.
		//	We detect this in a single pass. In the above example we would aggregate ModX, but warn loudly when we found ModY.
		//	(we expect content to solve this via stacking rules, or just not making these type of requirements).
		//
		//
		//
		//	
		// ------------------------------------------------------------------------
		
		// We have to do tag aggregation to figure out what we can and can't apply.
		FGameplayTagContainer CommittedIgnoreTags;

		TArray<const FAggregator*> ModList;
		ModList.Reserve(TotalModCount + (EGameplayModOp::Override - EGameplayModOp::Additive));

		// Build linear list of what we will aggregate
		for (int32 OpIdx = (EGameplayModOp::Additive); OpIdx < EGameplayModOp::Override; ++OpIdx)
		{
			for (const FAggregatorRef &Agg : Mods[OpIdx])
			{	
				ModList.Add(Agg.Get());
			}
			ModList.Add(this);	// Sentinel value to signify "NextOp"
		}

		static const float OpBias[EGameplayModOp::Override] = {
			0.f,	// EGameplayModOp::Additive
			1.f,	// EGameplayModOp::Multiplicitive
			1.f,	// EGameplayModOp::Division
		};

		float OpAggregation[EGameplayModOp::Override] = {
			0.f,	// EGameplayModOp::Additive
			1.f,	// EGameplayModOp::Multiplicitive
			1.f,	// EGameplayModOp::Division
		};

		// Make multiple passes to do tagging
		while (true)
		{
			FGameplayTagContainer	NewGiveTags;
			FGameplayTagContainer	NewIgnoreTags;
			FGameplayTagContainer	MissingTags;

			EGameplayModOp::Type	ModOp = (EGameplayModOp::Additive);
			
			for (int32 ModIdx = 0; ModIdx < ModList.Num()-1; ++ModIdx)
			{
				const FAggregator * Agg = ModList[ModIdx];
				if (Agg == NULL)
				{
					continue;
				}
				if (Agg == this)
				{
					ModOp = static_cast<EGameplayModOp::Type>(static_cast<int32>(ModOp)+1);
					check(ModOp < EGameplayModOp::Override);
					continue;
				}
				
				const FGameplayModifierData &ModBaseData = ModList[ModIdx]->BaseData;
				const FGameplayTagContainer &ModIgnoreTags = ModBaseData.IgnoreTags;
				const FGameplayTagContainer &ModRequireTags = ModBaseData.RequireTags;

				// This mod requires we have certain tags
				if (ModRequireTags.Num() > 0 && !CachedData.Tags.HasAllTags(ModRequireTags) && !NewGiveTags.HasAllTags(ModRequireTags))
				{
					// But something else could give us this tag! So keep track of it and don't remove this Mod from the ModList.
					MissingTags.AppendTags(ModRequireTags);
					continue;
				}

				// This mod is now either accepted or rejected. It will not be needed for subsequent passes in this Evaluate, so NULL It out now.
				ModList[ModIdx] = NULL;
				
				// This mod requires we don't have certain tags
				if (ModIgnoreTags.Num() > 0 && CachedData.Tags.HasAnyTag(ModIgnoreTags))
				{
					continue;
				}

				// Check for conflicts within this pass
				if (ModIgnoreTags.Num() > 0 && NewGiveTags.Num() > 0 && ModIgnoreTags.HasAnyTag(NewGiveTags))
				{
					// Pass problem!
					SKILL_LOG(Warning, TEXT("Tagging conflicts during Aggregate! Use Stacking rules to avoid this"));
					SKILL_LOG(Warning, TEXT("   While Evaluating: %s"), *ToSimpleString());
					SKILL_LOG(Warning, TEXT("   Applying Mod: %s"), *Agg->ToSimpleString());
					SKILL_LOG(Warning, TEXT("   ModIgnoreTags: %s"), *ModIgnoreTags.ToString() );
					SKILL_LOG(Warning, TEXT("   NewGiveTags: %s"), *NewGiveTags.ToString());
					continue;
				}

				// Our requirements on the mod
				const FGameplayModifierEvaluatedData& ModEvaluatedData = Agg->Evaluate();

				// We have already commited during this aggregation to not have certain tags
				if (CommittedIgnoreTags.Num() > 0 && CommittedIgnoreTags.HasAnyTag(ModEvaluatedData.Tags))
				{
					continue;
				}

				if (NewIgnoreTags.Num() > 0 && ModEvaluatedData.Tags.Num() > 0 && NewIgnoreTags.HasAnyTag(ModEvaluatedData.Tags))
				{
					// Pass problem!
					SKILL_LOG(Warning, TEXT("Tagging conflicts during Aggregate! Use Stacking rules to avoid this"));
					SKILL_LOG(Warning, TEXT("   While Evaluating: %s"), *ToSimpleString());
					SKILL_LOG(Warning, TEXT("   Applying Mod: %s"), *Agg->ToSimpleString());
					SKILL_LOG(Warning, TEXT("   Mod Tags: %s"), *ModEvaluatedData.Tags.ToString());
					SKILL_LOG(Warning, TEXT("   NewIgnoreTags: %s"), *NewIgnoreTags.ToString());
					continue;
				}

				// Commit this Mod
				NewIgnoreTags.AppendTags(ModIgnoreTags);
				ModEvaluatedData.Aggregate(NewGiveTags, OpAggregation[ModOp], OpBias[ModOp]);
			}

			// Commit this pass's tags
			CachedData.Tags.AppendTags(NewGiveTags);

			// Keep doing passes until we don't add any new tags or don't have any missing tags
			if (MissingTags.Num() <= 0 || !MissingTags.HasAnyTag( NewGiveTags ))
			{
				break;
			}
		}

		float Division = OpAggregation[EGameplayModOp::Division] > 0.f ? OpAggregation[EGameplayModOp::Division] : 1.f;

		CachedData.Magnitude = ((CachedData.Magnitude + OpAggregation[EGameplayModOp::Additive]) * OpAggregation[EGameplayModOp::Multiplicitive]) / Division;

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

	// Execute OnDirty callbacks first. This may do things like update the actual uproperty value of an attribute.
	OnDirty.ExecuteIfBound(this);

	// Now tell people who depend on my value that I have changed. Important to do this after the OnDirty callback has been called.
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

void FAggregator::CleaerAllDependancies()
{
	Dependants.SetNum(0);
	OnDirty.Unbind();
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
//	FActiveGameplayEffect
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FActiveGameplayEffect::PreReplicatedRemove(const struct FActiveGameplayEffectsContainer &InArray)
{
	InArray.Owner->InvokeGameplayCueRemoved(Spec);
}

void FActiveGameplayEffect::PostReplicatedAdd(const struct FActiveGameplayEffectsContainer &InArray)
{
	static const int32 MAX_DELTA_TIME = 3;

	InArray.Owner->InvokeGameplayCueAdded(Spec);
	// Was this actually just activated, or are we just finding out about it due to relevancy/join in progress?
	float WorldTimeSeconds = InArray.GetWorldTime();
	int32 GameStateTime = InArray.GetGameStateTime();

	int32 DeltaGameStateTime = GameStateTime - StartGameStateTime;	// How long we think the effect has been playing (only 1 second precision!)

	if (GameStateTime > 0 && FMath::Abs(DeltaGameStateTime) < MAX_DELTA_TIME)
	{
		InArray.Owner->InvokeGameplayCueActivated(Spec);
	}

	// Set our local start time accordingly

	StartWorldTime = WorldTimeSeconds - static_cast<float>(DeltaGameStateTime);
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

			FAggregator * Aggregator = FindOrCreateAttributeAggregator(Mod.Info.Attribute).Get();

			// Add the modifier to the property value aggregator
			// Note that this will immediately invoke the callback to update the attribute's current value, so we don't have to explicitly do it here.
			Aggregator->ApplyMod(Mod.Info.ModifierOp, Mod.Aggregator, Spec.ShouldApplyAsSnapshot(QualifierContext));
		}

		if (Mod.Info.ModifierType == EGameplayMod::ActiveGE)
		{
			SKILL_LOG_SCOPE(TEXT("Applying AttributeMod %s to ActiveEffects"),  *Mod.ToSimpleString(), *Mod.Info.Attribute.GetName());

			// TODO: Tag checks here

			// This modifies GEs that are currently active, so apply this to them...
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

	// check if this is a stacking effect and if it is the active stacking effect.
	// todo: move this into the loop if we end up applying stacking to different modifiers on the same effect
	if (Spec.GetStackingType() != EGameplayEffectStackingPolicy::Unlimited)
	{
		// we're a stacking attribute but not active
		if (Spec.StackedAttribName == NAME_None)
		{
			return;
		}
	}

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

			// This modifies GEs that are currently active, so apply this to them...
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
		// TODO: check replication policy. Right now we will replicate every execute via a multicast RPC

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
			return true;
		}
	}

	return false;
}

void FActiveGameplayEffectsContainer::AddDependancyToAttribute(FGameplayAttribute Attribute, const TWeakPtr<FAggregator> InDependant)
{
	FAggregator * Aggregator = FindOrCreateAttributeAggregator(Attribute).Get();
	Aggregator->AddDependantAggregator(InDependant);
}

FAggregatorRef & FActiveGameplayEffectsContainer::FindOrCreateAttributeAggregator(FGameplayAttribute Attribute)
{
	FAggregatorRef *RefPtr = OngoingPropertyEffects.Find(Attribute);
	if (RefPtr)
	{
		return *RefPtr;
	}

	// Create a new aggregator for this attribute.
	float CurrentValueOfProperty = Owner->GetNumericAttribute(Attribute);
	SKILL_LOG(Log, TEXT("Creating new entry in OngoingPropertyEffect map for AddDependancyToAttribute. CurrentValue: %.2f"), CurrentValueOfProperty);

	FAggregator *NewPropertyAggregator = new FAggregator(FGameplayModifierEvaluatedData(CurrentValueOfProperty), SKILL_AGG_DEBUG(TEXT("Attribute %s Aggregator"), *Attribute.GetName()));

	NewPropertyAggregator->OnDirty = FAggregator::FOnDirty::CreateRaw(this, &FActiveGameplayEffectsContainer::OnPropertyAggregatorDirty, Attribute);

	return OngoingPropertyEffects.Add(Attribute, FAggregatorRef(NewPropertyAggregator));
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
	for (const FActiveGameplayEffect& Effect : GameplayEffects)
	{
		if (Effect.Handle == Handle)
		{
			// stacking effects may return false
			if (Effect.Spec.GetStackingType() != EGameplayEffectStackingPolicy::Unlimited &&
				Effect.Spec.StackedAttribName == NAME_None)
			{
				return false;
			}
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

FActiveGameplayEffect & FActiveGameplayEffectsContainer::CreateNewActiveGameplayEffect(const FGameplayEffectSpec &Spec)
{
	LastAssignedHandle = LastAssignedHandle.GetNextHandle();
	FActiveGameplayEffect & NewEffect = *new (GameplayEffects)FActiveGameplayEffect(LastAssignedHandle, Spec, GetWorldTime(), GetGameStateTime());
	
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

bool FActiveGameplayEffectsContainer::IsNetAuthority() const
{
	check(Owner);
	return Owner->IsOwnerActorAuthoritative();
}

void FActiveGameplayEffectsContainer::PreDestroy()
{
	// Prior to destruction we need to clear all dependancies
	for (auto It : OngoingPropertyEffects)
	{
		if (It.Value.IsValid())
		{
			It.Value.Get()->CleaerAllDependancies();
		}
	}
}

int32 FActiveGameplayEffectsContainer::GetGameStateTime() const
{
	UWorld *World = Owner->GetWorld();
	AGameState * GameState = World->GetGameState<AGameState>();
	if (GameState)
	{
		return GameState->ElapsedTime;
	}

	return static_cast<int32>(World->GetTimeSeconds());
}

float FActiveGameplayEffectsContainer::GetWorldTime() const
{
	UWorld *World = Owner->GetWorld();
	return World->GetTimeSeconds();
}

void FActiveGameplayEffectsContainer::TEMP_TickActiveEffects(float DeltaSeconds)
{
	if (!IsNetAuthority())
	{
		// For now - clients don't tick their GameplayEffects at all. 
		// We eventually want them to tick to predicate gameplay cue events.
		return;
	}

	float CurrentTime = GetWorldTime();

	// effects may have been added outside of the tick during the last frame
	if (bNeedToRecalculateStacks)
	{
		RecalculateStacking();
	}

	for (int32 Idx = 0; Idx < GameplayEffects.Num(); ++Idx)
	{
		FActiveGameplayEffect &Effect = GameplayEffects[Idx];

		if (Effect.NextExecuteTime > 0)
		{
			float TimeOver = CurrentTime - Effect.NextExecuteTime;
			while (TimeOver >= -KINDA_SMALL_NUMBER)
			{
				// Advance
				Effect.AdvanceNextExecuteTime(CurrentTime);

				// Execute
				ExecuteActiveEffectsFrom( Effect.Spec, FModifierQualifier().IgnoreHandle(Effect.Handle) );

				// Update TimeOver
				TimeOver = CurrentTime - Effect.NextExecuteTime;
			}
		}

		float Duration = Effect.GetDuration();
		if (Duration > 0.f && Effect.StartWorldTime + Effect.GetDuration() < CurrentTime)
		{
			Owner->InvokeGameplayCueRemoved(Effect.Spec);

			MarkArrayDirty();
			GameplayEffects.RemoveAtSwap(Idx);
			Idx--;

			// todo: check if the removed effect deals with an attribute that stacks, if not don't set the flag
			bNeedToRecalculateStacks = true;
		}
	}

	// this needs to be updated here instead of waiting for the start of the next tick
	// an instantaneous effect could come in before the start of the next tick
	if (bNeedToRecalculateStacks)
	{
		RecalculateStacking();
	}
}

void FActiveGameplayEffectsContainer::RecalculateStacking()
{
	bNeedToRecalculateStacks = false;

	// one or more gameplay effects has been removed or added so we need to go through all of them to see what the best elements are in each stack
	TArray<FActiveGameplayEffect*> StackedEffects;
	TArray<FActiveGameplayEffect*> CustomStackedEffects;

	for (FActiveGameplayEffect& Effect : GameplayEffects)
	{
		// ignore effects that don't stack and effects that replace when they stack
		// effects that replace when they stack only need to be handled when they are added
		if (Effect.Spec.GetStackingType() != EGameplayEffectStackingPolicy::Unlimited &&
			Effect.Spec.GetStackingType() != EGameplayEffectStackingPolicy::Replaces)
		{
			if (Effect.Spec.Def->Modifiers.Num() > 0)
			{
				Effect.Spec.StackedAttribName = NAME_None;
				bool bFoundStack = false;

				// group all of the custom stacking effects and deal with them after
				if (Effect.Spec.GetStackingType() == EGameplayEffectStackingPolicy::Callback)
				{
					CustomStackedEffects.Add(&Effect);
					continue;
				}

				for (FActiveGameplayEffect*& StackedEffect : StackedEffects)
				{
					if (StackedEffect->Spec.GetStackingType() == Effect.Spec.GetStackingType() && StackedEffect->Spec.Def->Modifiers[0].Attribute == Effect.Spec.Def->Modifiers[0].Attribute)
					{
						switch (Effect.Spec.GetStackingType())
						{
						case EGameplayEffectStackingPolicy::Highest:
						{
							float BestSpecMagnitude = StackedEffect->Spec.GetMagnitude(StackedEffect->Spec.Modifiers[0].Info.Attribute);
							float CurrSpecMagnitude = Effect.Spec.GetMagnitude(Effect.Spec.Modifiers[0].Info.Attribute);
							if (BestSpecMagnitude < CurrSpecMagnitude)
							{
								StackedEffect = &Effect;
							}
							break;
						}
						case EGameplayEffectStackingPolicy::Lowest:
						{
							float BestSpecMagnitude = StackedEffect->Spec.GetMagnitude(StackedEffect->Spec.Modifiers[0].Info.Attribute);
							float CurrSpecMagnitude = Effect.Spec.GetMagnitude(Effect.Spec.Modifiers[0].Info.Attribute);
							if (BestSpecMagnitude > CurrSpecMagnitude)
							{
								StackedEffect = &Effect;
							}
							break;
						}
						default:
							// we shouldn't ever be here
							SKILL_LOG(Warning, TEXT("%s uses unhandled stacking rule %s."), *Effect.Spec.Def->GetPathName(), *EGameplayEffectStackingPolicyToString(Effect.Spec.GetStackingType()));
							break;
						};

						bFoundStack = true;
						break;
					}
				}

				// if we didn't find a stack matching this effect we need to add it to our list
				if (!bFoundStack)
				{
					StackedEffects.Add(&Effect);
				}
			}
		}
	}

	// now deal with the custom effects
	while (CustomStackedEffects.Num() > 0)
	{
		int32 StackedIdx = StackedEffects.Add(CustomStackedEffects[0]);
		CustomStackedEffects.RemoveAtSwap(0);

		UGameplayEffectStackingExtension * StackedExt = StackedEffects[StackedIdx]->Spec.Def->StackingExtension->GetDefaultObject<UGameplayEffectStackingExtension>();

		TArray<FActiveGameplayEffect*> Effects;
		
		for (int32 Idx = 0; Idx < CustomStackedEffects.Num(); Idx++)
		{
			UGameplayEffectStackingExtension * CurrentExt = CustomStackedEffects[Idx]->Spec.Def->StackingExtension->GetDefaultObject<UGameplayEffectStackingExtension>();
			if (CurrentExt && StackedExt && CurrentExt->Handle == StackedExt->Handle && 
				StackedEffects[StackedIdx]->Spec.Def->Modifiers[0].Attribute == CustomStackedEffects[Idx]->Spec.Def->Modifiers[0].Attribute)
			{
				Effects.Add(CustomStackedEffects[Idx]);
				CustomStackedEffects.RemoveAtSwap(Idx);
				--Idx;
			}
		}

		UGameplayEffectStackingExtension * Ext = StackedEffects[StackedIdx]->Spec.Def->StackingExtension->GetDefaultObject<UGameplayEffectStackingExtension>();
		Ext->CalculateStack(Effects, *this, *StackedEffects[StackedIdx]);
	}

	// we've found all of the best stacking effects, mark them so that they updated properly
	for (FActiveGameplayEffect* const StackedEffect : StackedEffects)
	{
		StackedEffect->Spec.StackedAttribName = FName(*StackedEffect->Spec.Def->Modifiers[0].Attribute.GetName());
	}
}

bool FActiveGameplayEffectsContainer::HasAnyTags(FGameplayTagContainer &Tags)
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsHasAnyTag);

	for (FActiveGameplayEffect &ActiveEffect : GameplayEffects)
	{
		// Fixme: check stacking rules!
		if (ActiveEffect.Spec.Def->OwnedTagsContainer.HasAnyTag(Tags))
		{
			return true;
		}
	}

	return false;
}

bool FActiveGameplayEffectsContainer::CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, AActor *Instigator)
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsCanApplyAttributeModifiers);

	FGameplayEffectSpec	Spec(GameplayEffect, TSharedPtr<FGameplayEffectLevelSpec>(new FGameplayEffectLevelSpec(Level, GameplayEffect->LevelInfo, Instigator)), Owner->GetCurveDataOverride());
	Spec.Def = GameplayEffect;
	Spec.InstigatorStack.AddInstigator(Instigator);

	ApplyActiveEffectsTo(Spec, FModifierQualifier().Type(EGameplayMod::IncomingGE));
	
	for (const FModifierSpec & Mod : Spec.Modifiers)
	{
		// It only makes sense to check additive operators
		if (Mod.Info.ModifierOp == EGameplayModOp::Additive)
		{
			UAttributeSet * Set = Owner->GetAttributeSubobject(Mod.Info.Attribute.GetAttributeSetClass());
			float CurrentValue = Mod.Info.Attribute.GetNumericValueChecked(Set);
			float CostValue = Mod.Aggregator.Get()->Evaluate().Magnitude;

			if (CurrentValue + CostValue < 0.f)
			{
				return false;
			}
		}
	}
	return true;
}

TArray<float> FActiveGameplayEffectsContainer::GetActiveEffectsTimeRemaining(const FActiveGameplayEffectQuery Query) const
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsGetActiveEffectsData);

	float CurrentTime = GetWorldTime();

	TArray<float>	ReturnList;

	for (const FActiveGameplayEffect &Effect : GameplayEffects)
	{
		if (Query.TagContainer)
		{
			if (!Effect.Spec.Def->OwnedTagsContainer.HasAnyTag(*Query.TagContainer))
			{
				continue;
			}
		}

		float Elapsed = CurrentTime - Effect.StartWorldTime;
		float Duration = Effect.GetDuration();

		ReturnList.Add(Duration - Elapsed);
	}

	// Note: keep one return location to avoid copy operation.
	return ReturnList;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayEffectLevelSpec
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

float FGameplayEffectLevelSpec::GetLevel() const
{
	if (ConstantLevel != INVALID_LEVEL)
	{
		return ConstantLevel;
	}

	if (Source.IsValid() && Attribute.GetUProperty() != NULL)
	{
		UClass * SetClass = Attribute.GetAttributeSetClass();
		check(SetClass);

		TArray<UObject*> ChildObjects;
		GetObjectsWithOuter(Source.Get(), ChildObjects);

		for (int32 ChildIndex = 0; ChildIndex < ChildObjects.Num(); ++ChildIndex)
		{
			UObject *Obj = ChildObjects[ChildIndex];
			if (Obj->GetClass()->IsChildOf(SetClass))
			{
				CachedLevel = Attribute.GetNumericValueChecked(CastChecked<UAttributeSet>(Obj));
				return CachedLevel;
			}
		}
	}

	// Our source is invalid. 
	ConstantLevel = CachedLevel;
	return CachedLevel;
}

void FGameplayEffectLevelSpec::RegisterLevelDependancy(TWeakPtr<FAggregator> OwningAggregator)
{
	if (Source.IsValid() && Attribute.GetUProperty() != NULL)
	{
		UAttributeComponent * SourceComponent = Source->FindComponentByClass<UAttributeComponent>();
		if (SourceComponent)
		{
			SourceComponent->AddDependancyToAttribute(Attribute, OwningAggregator);
		}
	}
}

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

FString EGameplayEffectStackingPolicyToString(int32 Type)
{
	static UEnum *e = FindObject<UEnum>(ANY_PACKAGE, TEXT("EGameplayEffectStackingPolicy"));
	return e->GetEnum(Type).ToString();
}

