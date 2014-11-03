// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectStackingExtension.h"
#include "GameplayTagsModule.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayModCalculation.h"

const float UGameplayEffect::INFINITE_DURATION = -1.f;
const float UGameplayEffect::INSTANT_APPLICATION = 0.f;
const float UGameplayEffect::NO_PERIOD = 0.f;
const float UGameplayEffect::INVALID_LEVEL = -1.f;

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	UGameplayEffect
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


UGameplayEffect::UGameplayEffect(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ChanceToApplyToTarget.SetValue(1.f);
	ChanceToExecuteOnGameplayEffect.SetValue(1.f);
	StackingPolicy = EGameplayEffectStackingPolicy::Unlimited;
	StackedAttribName = NAME_None;

#if WITH_EDITORONLY_DATA
	ShowAllProperties = true;
	Template = nullptr;
#endif

}

void UGameplayEffect::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	TagContainer.AppendTags(OwnedTagsContainer);
}

void UGameplayEffect::PostLoad()
{
	Super::PostLoad();

	ValidateGameplayEffect();
}

void UGameplayEffect::ValidateGameplayEffect()
{
	// todo: add a check here for instant effects that modify incoming/outgoing GEs
}

bool FGameplayEffectAttributeCaptureDefinition::operator==(const FGameplayEffectAttributeCaptureDefinition& Other) const
{
	return ((AttributeToCapture == Other.AttributeToCapture) && (AttributeSource == Other.AttributeSource) && (bSnapshot == Other.bSnapshot));
}

bool FGameplayEffectAttributeCaptureDefinition::operator!=(const FGameplayEffectAttributeCaptureDefinition& Other) const
{
	return ((AttributeToCapture != Other.AttributeToCapture) || (AttributeSource != Other.AttributeSource) || (bSnapshot != Other.bSnapshot));
}

FString FGameplayEffectAttributeCaptureDefinition::ToSimpleString() const
{
	return FString::Printf(TEXT("Attribute: %s, Capture Point: %s, Snapshot: %d"), *AttributeToCapture.GetName(), AttributeSource == EGameplayEffectAttributeCaptureSource::Source ? TEXT("Source") : TEXT("Target"), bSnapshot);
}

FGameplayEffectAttributeCaptureSpec::FGameplayEffectAttributeCaptureSpec()
{
}

FGameplayEffectAttributeCaptureSpec::FGameplayEffectAttributeCaptureSpec(const FGameplayEffectAttributeCaptureDefinition& InDefinition)
	: BackingDefinition(InDefinition)
{
}

const FGameplayEffectAttributeCaptureDefinition& FGameplayEffectAttributeCaptureSpec::GetBackingDefinition() const
{
	return BackingDefinition;
}

const FAggregator& FGameplayEffectAttributeCaptureSpec::GetAggregator() const
{
	FAggregator* Aggregator = AttributeAggregator.Get();
	if (ensure(Aggregator))
	{
		return *Aggregator;
	}

	ABILITY_LOG(Error, TEXT("FGameplayEffectAttributeCaptureSpec::GetAggregator called with no valid aggregator"));
	static FAggregator InvalidAggregator;
	return InvalidAggregator;
}

void FGameplayEffectAttributeCaptureSpecContainer::AddCaptureDefinition(const FGameplayEffectAttributeCaptureDefinition& InCaptureDefinition)
{
	const bool bSourceAttribute = (InCaptureDefinition.AttributeSource == EGameplayEffectAttributeCaptureSource::Source);
	TArray<FGameplayEffectAttributeCaptureSpec>& AttributeArray = (bSourceAttribute ? SourceAttributes : TargetAttributes);

	// Only add additional captures if this exact capture definition isn't already being handled
	if (!AttributeArray.ContainsByPredicate(
			[&](const FGameplayEffectAttributeCaptureSpec& Element)
			{
				return Element.GetBackingDefinition() == InCaptureDefinition;
			}))
	{
		AttributeArray.Add(FGameplayEffectAttributeCaptureSpec(InCaptureDefinition));

		if (!InCaptureDefinition.bSnapshot)
		{
			bHasNonSnapshottedAttributes = true;
		}
	}
}

void FGameplayEffectAttributeCaptureSpecContainer::CaptureAttributes(UAbilitySystemComponent* InAbilitySystemComponent, EGameplayEffectAttributeCaptureSource InCaptureSource)
{
	if (InAbilitySystemComponent)
	{
		const bool bSourceComponent = (InCaptureSource == EGameplayEffectAttributeCaptureSource::Source);
		TArray<FGameplayEffectAttributeCaptureSpec>& AttributeArray = (bSourceComponent ? SourceAttributes : TargetAttributes);

		// Capture every spec's requirements from the specified component
		for (FGameplayEffectAttributeCaptureSpec& CurCaptureSpec : AttributeArray)
		{
			InAbilitySystemComponent->CaptureAttributeForGameplayEffect(CurCaptureSpec);
		}
	}
}

const FAggregator& FGameplayEffectAttributeCaptureSpecContainer::GetAggregator(const FGameplayEffectAttributeCaptureDefinition& InDefinition) const
{
	const bool bSourceAttribute = (InDefinition.AttributeSource == EGameplayEffectAttributeCaptureSource::Source);
	const TArray<FGameplayEffectAttributeCaptureSpec>& AttributeArray = (bSourceAttribute ? SourceAttributes : TargetAttributes);

	const FGameplayEffectAttributeCaptureSpec* MatchingSpec = AttributeArray.FindByPredicate(
		[&](const FGameplayEffectAttributeCaptureSpec& Element)
		{
			return Element.GetBackingDefinition() == InDefinition;
		});

	if (MatchingSpec)
	{
		return MatchingSpec->GetAggregator();
	}

	ABILITY_LOG(Error, TEXT("FGameplayEffectAttributeCaptureSpecContainer::GetAggregator called for attribute definition %s when that attribute was not captured."), *InDefinition.ToSimpleString());
	static FAggregator InvalidAggregator;
	return InvalidAggregator;
}

bool FGameplayEffectAttributeCaptureSpecContainer::HasNonSnapshottedAttributes() const
{
	return bHasNonSnapshottedAttributes;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FGameplayEffectSpec
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

FGameplayEffectSpec::FGameplayEffectSpec(const UGameplayEffect* InDef, const FGameplayEffectContextHandle& InEffectContext, float Level)
: Def(InDef)
{
	check(Def);	
	SetLevel(Level);
	SetContext(InEffectContext);

	// Init our ModifierSpecs
	Modifiers.Reserve(Def->Modifiers.Num());
	for (const FGameplayModifierInfo &ModInfo : Def->Modifiers)
	{
		// This creates a new FModifierSpec that we own.
		new (Modifiers)FModifierSpec(ModInfo);
	}

	// Gather all capture definitions from modifiers
	for (const FModifierSpec& CurModSpec : Modifiers)
	{
		for (const FGameplayEffectAttributeCaptureDefinition& CurCaptureDef : CurModSpec.Info.MagnitudeV2.CustomMagnitudeCalculationAttributes)
		{
			CapturedRelevantAttributes.AddCaptureDefinition(CurCaptureDef);
		}
	}

	// Gather all capture definitions from executions
	for (const FGameplayEffectExecutionDefinition& Exec : Def->Executions)
	{
		if (Exec.CalculationClass == nullptr)
			continue;

		TArray<FGameplayEffectAttributeCaptureDefinition> Reqs;
		const UGameplayEffectCalculation* CDO = Cast<UGameplayEffectCalculation>(Exec.CalculationClass->ClassDefaultObject);
		
		CDO->GetCaptureDefinitions(Reqs);

		// Maybe it's better to pass in CapturedRelevantAttributes.AddCaptureDefinition(CurCaptureDef); to GetCaptureDefinitions?
		for (const FGameplayEffectAttributeCaptureDefinition& Req : Reqs)
		{
			CapturedRelevantAttributes.AddCaptureDefinition(Req);
		}
	}

	// Add the GameplayEffect asset tags to the source Spec tags
	CapturedSourceTags.GetSpecTags().AppendTags(InDef->GameplayEffectTags);

	// Make TargetEffectSpecs too
	for (UGameplayEffect* TargetDef : InDef->TargetEffects)
	{
		TargetEffectSpecs.Add(TSharedRef<FGameplayEffectSpec>(new FGameplayEffectSpec(TargetDef, EffectContext, Level)));
	}

	// Everything is setup now, capture data from our source
	CaptureDataFromSource();
}

void FGameplayEffectSpec::CaptureDataFromSource()
{
	// Capture source actor tags
	CapturedSourceTags.GetActorTags().RemoveAllTags();
	EffectContext.GetOwnedGameplayTags(CapturedSourceTags.GetActorTags());

	// Capture source Attributes
	// Is this the right place to do it? Do we ever need to create spec and capture attributes at a later time? If so, this will need to move.
	CapturedRelevantAttributes.CaptureAttributes(EffectContext.GetInstigatorAbilitySystemComponent(), EGameplayEffectAttributeCaptureSource::Source);
}

void FGameplayEffectSpec::SetLevel(float InLevel)
{
	Level = InLevel;
	if (Def)
	{
		SetDuration(Def->Duration.GetValueAtLevel(InLevel));

		Period = Def->Period.GetValueAtLevel(InLevel);
		ChanceToApplyToTarget = Def->ChanceToApplyToTarget.GetValueAtLevel(InLevel);
		ChanceToExecuteOnGameplayEffect = Def->ChanceToExecuteOnGameplayEffect.GetValueAtLevel(InLevel);
	}
}

float FGameplayEffectSpec::GetLevel() const
{
	return Level;
}

float FGameplayEffectSpec::GetDuration() const
{
	return Duration;
}

void FGameplayEffectSpec::SetDuration(float NewDuration)
{
	Duration = NewDuration;
	if (Duration > 0.f)
	{
		// We may  have potential problems one day if a game is applying duration based gameplay effects from instantaneous effects
		// (E.g., everytime fire damage is applied, a DOT is also applied). We may need to for Duration to always be capturd.
		CapturedRelevantAttributes.AddCaptureDefinition(UAbilitySystemComponent::GetOutgoingDurationCapture());
	}
}

float FGameplayEffectSpec::GetPeriod() const
{
	return Period;
}

float FGameplayEffectSpec::GetChanceToApplyToTarget() const
{
	return ChanceToApplyToTarget;
}

float FGameplayEffectSpec::GetChanceToExecuteOnGameplayEffect() const
{
	return ChanceToExecuteOnGameplayEffect;
}

float FGameplayEffectSpec::GetModifierMagnitude(const FModifierSpec& ModSpec) const
{
	return ModSpec.EvaluatedMagnitude;

	//return ModSpec.Info.Magnitude.GetValueAtLevel(Level);
}

void FGameplayEffectSpec::CalculateModifierMagnitudes()
{
	for (FModifierSpec& Mod : Modifiers)
	{
		Mod.CalculateMagnitude(*this);
	}
}

float FGameplayEffectSpec::GetMagnitude(const FGameplayAttribute &Attribute) const
{
	float CurrentMagnitude = 0.f;
	for (const FModifierSpec &Mod : Modifiers)
	{		
		if (Mod.Info.Attribute != Attribute)
		{
			continue;
		}

		CurrentMagnitude = GetModifierMagnitude(Mod);
		break;
	}

	return CurrentMagnitude;
}

const FGameplayEffectModifiedAttribute* FGameplayEffectSpec::GetModifiedAttribute(const FGameplayAttribute& Attribute) const
{
	for (const FGameplayEffectModifiedAttribute& ModifiedAttribute : ModifiedAttributes)
	{
		if (ModifiedAttribute.Attribute == Attribute)
		{
			return &ModifiedAttribute;
		}
	}
	return NULL;
}

FGameplayEffectModifiedAttribute* FGameplayEffectSpec::GetModifiedAttribute(const FGameplayAttribute& Attribute)
{
	for (FGameplayEffectModifiedAttribute& ModifiedAttribute : ModifiedAttributes)
	{
		if (ModifiedAttribute.Attribute == Attribute)
		{
			return &ModifiedAttribute;
		}
	}
	return NULL;
}

FGameplayEffectModifiedAttribute* FGameplayEffectSpec::AddModifiedAttribute(const FGameplayAttribute& Attribute)
{
	FGameplayEffectModifiedAttribute NewAttribute;
	NewAttribute.Attribute = Attribute;
	return &ModifiedAttributes[ModifiedAttributes.Add(NewAttribute)];
}

void FGameplayEffectSpec::PruneModifiedAttributes()
{
	TSet<FGameplayAttribute> ImportantAttributes;

	for (FGameplayEffectCue CueInfo : Def->GameplayCues)
	{
		if (CueInfo.MagnitudeAttribute.IsValid())
		{
			ImportantAttributes.Add(CueInfo.MagnitudeAttribute);
		}
	}

	// Remove all unimportant attributes
	for (int32 i = ModifiedAttributes.Num() - 1; i >= 0; i--)
	{
		if (!ImportantAttributes.Contains(ModifiedAttributes[i].Attribute))
		{
			ModifiedAttributes.RemoveAtSwap(i);
		}
	}
}

EGameplayEffectStackingPolicy::Type FGameplayEffectSpec::GetStackingType() const
{
	return Def->StackingPolicy;
}

void FGameplayEffectSpec::SetContext(FGameplayEffectContextHandle NewEffectContext)
{
	bool bWasAlreadyInit = EffectContext.IsValid();
	EffectContext = NewEffectContext;	
	if (bWasAlreadyInit)
	{
		CaptureDataFromSource();
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FModifierSpec
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

FModifierSpec::FModifierSpec(const FGameplayModifierInfo& InInfo)
: Info(InInfo)
, EvaluatedMagnitude(0.f)
{

}

float FModifierSpec::GetEvaluatedMagnitude() const
{
	return EvaluatedMagnitude;
}

void FModifierSpec::CalculateMagnitude(OUT FGameplayEffectSpec& OwnerSpec)
{	
	EvaluatedMagnitude = Info.Magnitude.GetValueAtLevel(OwnerSpec.GetLevel());

	// @todo: this should have an enumeration over which to use
	/*
	if (Info.MagnitudeV2.CalculationClassMagnitude)
	{
		const UGameplayModCalculation* ModCalcCDO = Info.MagnitudeV2.CalculationClassMagnitude->GetDefaultObject<UGameplayModCalculation>();
		check(ModCalcCDO);

		EvaluatedMagnitude = ModCalcCDO->CalculateMagnitude(OwnerSpec);
	}
	else
	{
		EvaluatedMagnitude = Info.MagnitudeV2.ScalableFloatMagnitude.GetValueAtLevel(OwnerSpec.GetLevel());
	}
	*/
}


// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FActiveGameplayEffect
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------


void FActiveGameplayEffect::CheckOngoingTagRequirements(const FGameplayTagContainer& OwnerTags, FActiveGameplayEffectsContainer& OwningContainer)
{
	bool ShouldBeInhibited = !Spec.Def->OngoingTagRequirements.RequirementsMet(OwnerTags);

	if (IsInhibited != ShouldBeInhibited)
	{
		if (ShouldBeInhibited)
		{
			// Register this ActiveGameplayEffects modifiers with our Attribute Aggregators
			if (Spec.GetPeriod() == UGameplayEffect::NO_PERIOD)
			{
				for (int32 ModIdx = 0; ModIdx < Spec.Modifiers.Num(); ++ModIdx)
				{
					const FModifierSpec &Mod = Spec.Modifiers[ModIdx];
					
					ABILITY_LOG_SCOPE(TEXT("Applying Attribute Mod %s to property"), *Mod.ToSimpleString());

					FAggregator* Aggregator = OwningContainer.FindOrCreateAttributeAggregator(Mod.Info.Attribute).Get();

					Aggregator->RemoveMod(Handle);
				}
			}
		}
		else
		{
			// Register this ActiveGameplayEffects modifiers with our Attribute Aggregators
			if (Spec.GetPeriod() == UGameplayEffect::NO_PERIOD)
			{
				for (int32 ModIdx = 0; ModIdx < Spec.Modifiers.Num(); ++ModIdx)
				{
					const FModifierSpec &Mod = Spec.Modifiers[ModIdx];
					
					ABILITY_LOG_SCOPE(TEXT("Applying Attribute Mod %s to property"), *Mod.ToSimpleString());

					FAggregator* Aggregator = OwningContainer.FindOrCreateAttributeAggregator(Mod.Info.Attribute).Get();

					// GE_FIXME: Figure this out - when does EvaluatedData get calculated (here? Before and stored off?)
					float EvaluatedData = Spec.GetModifierMagnitude(Mod);

					Aggregator->AddMod(EvaluatedData, Mod.Info.ModifierOp, &Mod.Info.SourceTags, &Mod.Info.TargetTags, Handle);
				}
			}
		}

		IsInhibited = ShouldBeInhibited;
	}
}

bool FActiveGameplayEffect::CanBeStacked(const FActiveGameplayEffect& Other) const
{
	return (Handle != Other.Handle) && Spec.GetStackingType() == Other.Spec.GetStackingType() && Spec.Def->Modifiers[0].Attribute == Other.Spec.Def->Modifiers[0].Attribute;
}

void FActiveGameplayEffect::PreReplicatedRemove(const struct FActiveGameplayEffectsContainer &InArray)
{
	if (Spec.Def == nullptr)
	{
		ABILITY_LOG(Error, TEXT("Received PreReplicatedRemove with no UGameplayEffect def."));
		return;
	}

	const_cast<FActiveGameplayEffectsContainer&>(InArray).InternalOnActiveGameplayEffectRemoved(*this);	// Const cast is ok. It is there to prevent mutation of the GameplayEffects array, which this wont do.
	
	InArray.Owner->InvokeGameplayCueEvent(Spec, EGameplayCueEvent::Removed);
}

void FActiveGameplayEffect::PostReplicatedAdd(const struct FActiveGameplayEffectsContainer &InArray)
{
	if (Spec.Def == nullptr)
	{
		ABILITY_LOG(Error, TEXT("Received ReplicatedGameplayEffect with no UGameplayEffect def."));
		return;
	}

	bool ShouldInvokeGameplayCueEvents = true;
	if (PredictionKey.IsValidKey())
	{
		// PredictionKey will only be valid on the client that predicted it. So if this has a valid PredictionKey, we can assume we already predicted it and shouldn't invoke GameplayCues.
		// We may need to do more bookkeeping here in the future. Possibly give the predicted gameplayeffect a chance to pass something off to the new replicated gameplay effect.
		
		if (InArray.HasPredictedEffectWithPredictedKey(PredictionKey))
		{
			ShouldInvokeGameplayCueEvents = false;
		}
	}

	const_cast<FActiveGameplayEffectsContainer&>(InArray).InternalOnActiveGameplayEffectAdded(*this);	// Const cast is ok. It is there to prevent mutation of the GameplayEffects array, which this wont do.

	static const int32 MAX_DELTA_TIME = 3;

	if (ShouldInvokeGameplayCueEvents)
	{
		InArray.Owner->InvokeGameplayCueEvent(Spec, EGameplayCueEvent::WhileActive);
	}

	// Was this actually just activated, or are we just finding out about it due to relevancy/join in progress?
	float WorldTimeSeconds = InArray.GetWorldTime();
	int32 GameStateTime = InArray.GetGameStateTime();

	int32 DeltaGameStateTime = GameStateTime - StartGameStateTime;	// How long we think the effect has been playing (only 1 second precision!)

	if (ShouldInvokeGameplayCueEvents && GameStateTime > 0 && FMath::Abs(DeltaGameStateTime) < MAX_DELTA_TIME)
	{
		InArray.Owner->InvokeGameplayCueEvent(Spec, EGameplayCueEvent::OnActive);
	}

	// Set our local start time accordingly
	StartWorldTime = WorldTimeSeconds - static_cast<float>(DeltaGameStateTime);
}

void FActiveGameplayEffect::PostReplicatedChange(const struct FActiveGameplayEffectsContainer &InArray)
{
	if (Spec.Def == nullptr)
	{
		ABILITY_LOG(Error, TEXT("Received ReplicatedGameplayEffect with no UGameplayEffect def."));
	}
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	FActiveGameplayEffectsContainer
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

void FActiveGameplayEffectsContainer::RegisterWithOwner(UAbilitySystemComponent* InOwner)
{
	if (Owner != InOwner)
	{
		Owner = InOwner;

		// Binding raw is ok here, since the owner is literally the UObject that owns us. If we are destroyed, its because that uobject is destroyed,
		// and if that is destroyed, the delegate wont be able to fire.
		Owner->RegisterGenericGameplayTagEvent().AddRaw(this, &FActiveGameplayEffectsContainer::OnOwnerTagChange);

		// Add Aggregators for system attributes
		//FindOrCreateAttributeAggregator(
	}
}

/** This is the main function that executes a GameplayEffect on Attributes and ActiveGameplayEffects */
void FActiveGameplayEffectsContainer::ExecuteActiveEffectsFrom(FGameplayEffectSpec &Spec, FPredictionKey PredictionKey)
{
	// If there are no modifiers, we always want to apply the GameplayCue. If there are modifiers, we only want to invoke the GameplayCue if one of them went through (coudl be blocked by immunity or % chance roll)
	bool InvokeGameplayCueExecute = (Spec.Modifiers.Num() == 0);

	// check if this is a stacking effect and if it is the active stacking effect.
	if (Spec.GetStackingType() != EGameplayEffectStackingPolicy::Unlimited)
	{
		// we're a stacking attribute but not active
		if (Spec.bTopOfStack == false)
		{
			return;
		}
	}

	float ChanceToExecute = Spec.GetChanceToExecuteOnGameplayEffect();		// Not implemented? Should we just remove ChanceToExecute?

	// Capture our own tags.
	// TODO: We should only capture them if we need to. We may have snapshotted target tags (?) (in the case of dots with exotic setups?)

	Spec.CapturedTargetTags.GetActorTags().RemoveAllTags();
	Owner->GetOwnedGameplayTags(Spec.CapturedTargetTags.GetActorTags());

	Spec.CalculateModifierMagnitudes();

	// ------------------------------------------------------
	//	Modifiers
	//		These will modify the base value of attributes
	// ------------------------------------------------------

	for (const FModifierSpec &Mod : Spec.Modifiers)
	{
		UAttributeSet* AttributeSet = const_cast<UAttributeSet*>(Owner->GetAttributeSubobject(Mod.Info.Attribute.GetAttributeSetClass()));
		if (AttributeSet == NULL)
		{
			// Our owner doesn't have this attribute, so we can't do anything
			ABILITY_LOG(Log, TEXT("%s does not have attribute %s. Skipping modifier"), *Owner->GetPathName(), *Mod.Info.Attribute.GetName());
			continue;
		}

		ABILITY_LOG_SCOPE(TEXT("Executing Attribute Mod %s"), *Mod.ToSimpleString());

		// First, evaluate all of our data
			
		// GE_Remove: still fixing this up:

		float EvaluatedMagnitude = Mod.Info.Magnitude.GetValueAtLevel(Spec.GetLevel());

		FGameplayModifierEvaluatedData EvaluatedData(EvaluatedMagnitude);

		FGameplayEffectModCallbackData ExecuteData(Spec, Mod, EvaluatedData, *Owner);

		/** 
		 *  This should apply 'gamewide' rules. Such as clamping Health to MaxHealth or granting +3 health for every point of strength, etc 
		 *	PreAttributeModify can return false to 'throw out' this modification.
		 */
		if (AttributeSet->PreGameplayEffectExecute(ExecuteData) == false)
		{
			continue;
		}

		// Todo: Tags/application checks here - make sure we can still apply (GE_Remove ??? is this still valid?)
		InvokeGameplayCueExecute = true;

		// Do we have active GE's that are already modifying this?
		FAggregatorRef* RefPtr = AttributeAggregatorMap.Find(Mod.Info.Attribute);
		if (RefPtr)
		{
			ABILITY_LOG(Log, TEXT("Property %s has active mods. Adding to Aggregator."), *Mod.Info.Attribute.GetName());
			RefPtr->Get()->ExecModOnBaseValue(Mod.Info.ModifierOp, Mod.GetEvaluatedMagnitude() );
		}
		else
		{
			// Modify the property inplace, without putting it in the AttributeAggregatorMap map
			float		CurrentValueOfProperty = Owner->GetNumericAttribute(Mod.Info.Attribute);
			const float NewPropertyValue = FAggregator::StaticExecModOnBaseValue(CurrentValueOfProperty, Mod.Info.ModifierOp, Mod.GetEvaluatedMagnitude());

			InternalUpdateNumericalAttribute(Mod.Info.Attribute, NewPropertyValue, &ExecuteData);
		}

		FGameplayEffectModifiedAttribute* ModifiedAttribute = Spec.GetModifiedAttribute(Mod.Info.Attribute);
		if (!ModifiedAttribute)
		{
			// If we haven't already created a modified attribute holder, create it
			ModifiedAttribute = Spec.AddModifiedAttribute(Mod.Info.Attribute);
		}
		ModifiedAttribute->TotalMagnitude += EvaluatedData.Magnitude;

		/** This should apply 'gamewide' rules. Such as clamping Health to MaxHealth or granting +3 health for every point of strength, etc */
		AttributeSet->PostGameplayEffectExecute(ExecuteData);
	}

	// ------------------------------------------------------
	//	Executions
	//		This will run custom code to 'do stuff'
	// ------------------------------------------------------
	 
	for (int32 ExecIdx=0; ExecIdx < Spec.Def->Executions.Num(); ++ExecIdx)
	{
		const FGameplayEffectExecutionDefinition& Exec = Spec.Def->Executions[ExecIdx];
		if (Exec.CalculationClass == nullptr)
			continue;

		const UGameplayEffectCalculation* CDO = Cast<UGameplayEffectCalculation>(Exec.CalculationClass->ClassDefaultObject);
		check(CDO);


		// Apply custom inline mods
		FActiveGameplayEffectHandle ModifierHandle = FActiveGameplayEffectHandle::GenerateNewHandle(Owner);
		TSet<FAggregator*>	ModifiedAggregators;
		for (const FExtensionAttributeModifierInfo& Mod : Exec.CalculationModifiers)
		{
			// Evaluate this mods current value. Right now this is always a scalable float, but we'd like to support
			// pulling arbitrary attribute values here too - these would be pulled from the Spec's captured attribute only?
			// But bottom line - we evaluate them right here and that is their value for this scope. Right?
			
			// We do a const cast here becase we are being really careful! We will add Mods below but we will also remove them once the 
			// custom calculation is finished. Custom Exec functions will only get const references to the aggregator.
			
			// If the custom exec functions want to add mods inplace, they can do it either like:
			//	A) like we are doing it here (careful const cast + remove)
			//  B) We give them a way to clone the aggregators on the stack and allow them to do whatever they want to those copies
			//  C) We provide another optional parameter into ::Evaluate that takes additional mods.
			 
			
			// @todo: Just always assuming snapshot for now, which is wrong. This should use a different struct.
			FAggregator& Aggregator = const_cast<FAggregator&>(Spec.CapturedRelevantAttributes.GetAggregator(FGameplayEffectAttributeCaptureDefinition(Mod.Attribute, Mod.Source, true)));


			float EvaluatedMagnitude = Mod.Magnitude.GetValueAtLevel(Spec.GetLevel());

			Aggregator.DisableCallbacks();
			Aggregator.AddMod(EvaluatedMagnitude, Mod.ModifierOp, &Mod.SourceTags, &Mod.TargetTags, ModifierHandle);

			ModifiedAggregators.Add(&Aggregator);
		}

		CDO->Execute(Spec, ExecIdx, Owner);

		// Remove those custom inline mods
		for (FAggregator* Aggregator : ModifiedAggregators)
		{
			Aggregator->RemoveMod(ModifierHandle);
			Aggregator->EnabledCallbacks();
		}

	}


	// ------------------------------------------------------
	//	Invoke GameplayCue events
	// ------------------------------------------------------

	
	// Prune the modified attributes before we replicate
	Spec.PruneModifiedAttributes();

	if (InvokeGameplayCueExecute && Spec.Def->GameplayCues.Num())
	{
		// TODO: check replication policy. Right now we will replicate every execute via a multicast RPC

		ABILITY_LOG(Log, TEXT("Invoking Execute GameplayCue for %s"), *Spec.ToSimpleString() );
		Owner->NetMulticast_InvokeGameplayCueExecuted_FromSpec(Spec, PredictionKey);
	}
}

void FActiveGameplayEffectsContainer::ExecutePeriodicGameplayEffect(FActiveGameplayEffectHandle Handle)
{
	FActiveGameplayEffect* ActiveEffect = GetActiveGameplayEffect(Handle);
	if (ActiveEffect)
	{
		// Execute
		ExecuteActiveEffectsFrom(ActiveEffect->Spec);
	}
}

FActiveGameplayEffect* FActiveGameplayEffectsContainer::GetActiveGameplayEffect(const FActiveGameplayEffectHandle Handle)
{
	// Could make this a map for quicker lookup
	for (FActiveGameplayEffect& Effect : GameplayEffects)
	{
		if (Effect.Handle == Handle)
		{
			return &Effect;
		}
	}
	return nullptr;
}

FAggregatorRef& FActiveGameplayEffectsContainer::FindOrCreateAttributeAggregator(FGameplayAttribute Attribute)
{
	FAggregatorRef *RefPtr = AttributeAggregatorMap.Find(Attribute);
	if (RefPtr)
	{
		return *RefPtr;
	}

	// Create a new aggregator for this attribute.
	float CurrentValueOfProperty = Owner->GetNumericAttribute(Attribute);
	ABILITY_LOG(Log, TEXT("Creating new entry in AttributeAggregatorMap for %s. CurrentValue: %.2f"), *Attribute.GetName(), CurrentValueOfProperty);

	FAggregator* NewAttributeAggregator = new FAggregator(CurrentValueOfProperty);
	
	if (Attribute.IsSystemAttribute() == false)
	{
		NewAttributeAggregator->OnDirty.AddUObject(Owner, &UAbilitySystemComponent::OnAttributeAggregatorDirty, Attribute);
	}

	return AttributeAggregatorMap.Add(Attribute, FAggregatorRef(NewAttributeAggregator));
}

// Move this to UAttributeComponent?
void FActiveGameplayEffectsContainer::OnAttributeAggregatorDirty(FAggregator* Aggregator, FGameplayAttribute Attribute)
{
	ABILITY_LOG_SCOPE(TEXT("FActiveGameplayEffectsContainer::OnAttributeAggregatorDirty"));
	check(AttributeAggregatorMap.FindChecked(Attribute).Get() == Aggregator);

	// Our Aggregator has changed, we need to reevaluate this aggregator and update the current value of the attribute.
	// Note that this is not an execution, so there are no 'source' and 'target' tags to fill out in the FAggregatorEvaluateParameters.
	// ActiveGameplayEffects that have required owned tags will be turned on/off via delegates, and will add/remove themselves from attribute
	// aggregators when that happens.
	
	FAggregatorEvaluateParameters EvaluationParameters;
	
	float NewValue = Aggregator->Evaluate(EvaluationParameters);
	InternalUpdateNumericalAttribute(Attribute, NewValue, nullptr);
}

void FActiveGameplayEffectsContainer::SetBaseAttributeValueFromReplication(FGameplayAttribute Attribute, float BaseValue)
{
/*
	FIXME: the approach for client side attribute prediction will need to be slightly rethought. Rather than touching base values we should be able
	to just apply Mods... this function may just go away?

	FAggregatorRef* RefPtr = AttributeAggregatorMap.Find(Attribute);
	if (RefPtr && RefPtr->Get())
	{
		RefPtr->Get()->SetBaseValue(Attribute);
		RefPtr->Get()->MarkDirty();
	}
*/
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
	
	ABILITY_LOG(Warning, TEXT("GetGameplayEffectDuration called with invalid Handle: %s"), *Handle.ToString() );
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
					return Effect.Spec.GetModifierMagnitude(Mod);
				}
			}
		}
	}

	ABILITY_LOG(Warning, TEXT("GetGameplayEffectMagnitude called with invalid Handle: %s"), *Handle.ToString());
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
				Effect.Spec.bTopOfStack == false)
			{
				return false;
			}
			return true;
		}
	}

	return false;
}

void FActiveGameplayEffectsContainer::CaptureAttributeForGameplayEffect(OUT FGameplayEffectAttributeCaptureSpec& OutCaptureSpec)
{
	FAggregatorRef& AttributeAggregator = FindOrCreateAttributeAggregator(OutCaptureSpec.BackingDefinition.AttributeToCapture);
	
	if (OutCaptureSpec.BackingDefinition.bSnapshot)
	{
		OutCaptureSpec.AttributeAggregator.TakeSnapshotOf(AttributeAggregator);
	}
	else
	{
		OutCaptureSpec.AttributeAggregator = AttributeAggregator;
	}
}

void FActiveGameplayEffectsContainer::InternalUpdateNumericalAttribute(FGameplayAttribute Attribute, float NewValue, const FGameplayEffectModCallbackData* ModData)
{
	ABILITY_LOG(Log, TEXT("Property %s new value is: %.2f"), *Attribute.GetName(), NewValue);
	Owner->SetNumericAttribute(Attribute, NewValue);

	FOnGameplayAttributeChange* Delegate = AttributeChangeDelegates.Find(Attribute);
	if (Delegate)
	{
		Delegate->Broadcast(NewValue, ModData);
	}
}

void FActiveGameplayEffectsContainer::StacksNeedToRecalculate()
{
	if (!bNeedToRecalculateStacks)
	{
		bNeedToRecalculateStacks = true;
		FTimerManager& TimerManager = Owner->GetWorld()->GetTimerManager();
		FTimerDelegate Delegate = FTimerDelegate::CreateUObject(Owner, &UAbilitySystemComponent::OnRestackGameplayEffects);

		TimerManager.SetTimerForNextTick(Delegate);
	}
}

FActiveGameplayEffect& FActiveGameplayEffectsContainer::CreateNewActiveGameplayEffect(const FGameplayEffectSpec &Spec, FPredictionKey InPredictionKey)
{
	SCOPE_CYCLE_COUNTER(STAT_CreateNewActiveGameplayEffect);

	FActiveGameplayEffectHandle NewHandle = FActiveGameplayEffectHandle::GenerateNewHandle(Owner);
	FActiveGameplayEffect& NewEffect = *new (GameplayEffects)FActiveGameplayEffect(NewHandle, Spec, GetWorldTime(), GetGameStateTime(), InPredictionKey);

	// Calculate Duration mods if we have a real duration
	float DurationBaseValue = NewEffect.Spec.GetDuration();
	if (DurationBaseValue > 0.f)
	{
		const FAggregator& OutgoingAgg = NewEffect.Spec.CapturedRelevantAttributes.GetAggregator(UAbilitySystemComponent::GetOutgoingDurationCapture());
		const FAggregator& IncomingAgg = *FindOrCreateAttributeAggregator(UAbilitySystemComponent::GetIncomingDurationProperty()).Get();
		
		FAggregator DurationAgg;

		DurationAgg.AddModsFrom(OutgoingAgg);
		DurationAgg.AddModsFrom(IncomingAgg);

		FAggregatorEvaluateParameters Params;
		Params.SourceTags = NewEffect.Spec.CapturedSourceTags.GetAggregatedTags();
		Params.TargetTags = NewEffect.Spec.CapturedTargetTags.GetAggregatedTags();

		float FinalDuration = DurationAgg.EvaluateWithBase(DurationBaseValue, Params);

		// We cannot mod ourselves into an instant or infinite duration effect
		if (FinalDuration <= 0.f)
		{
			ABILITY_LOG(Error, TEXT("GameplayEffect %s Duration was modified to %.2f. Clamping to 0.1s duration."), *NewEffect.Spec.Def->GetName(), FinalDuration);
			FinalDuration = 0.1f;
		}

		NewEffect.Spec.SetDuration(FinalDuration);

		// ABILITY_LOG(Warning, TEXT("SetDuration for %s. Base: %.2f, Final: %.2f"), *NewEffect.Spec.Def->GetName(), DurationBaseValue, FinalDuration);
	}
	
	// register callbacks with the timer manager
	if (Owner)
	{
		if (NewEffect.Spec.GetDuration() > 0.f)
		{
			FTimerManager& TimerManager = Owner->GetWorld()->GetTimerManager();
			FTimerDelegate Delegate = FTimerDelegate::CreateUObject(Owner, &UAbilitySystemComponent::CheckDurationExpired, NewHandle);
			TimerManager.SetTimer(NewEffect.DurationHandle, Delegate, NewEffect.Spec.GetDuration(), false, NewEffect.Spec.GetDuration());
		}
		// The timer manager moves things from the pending list to the active list after checking the active list on the first tick so we need to execute here
		if (NewEffect.Spec.GetPeriod() != UGameplayEffect::NO_PERIOD)
		{
			FTimerManager& TimerManager = Owner->GetWorld()->GetTimerManager();
			FTimerDelegate Delegate = FTimerDelegate::CreateUObject(Owner, &UAbilitySystemComponent::ExecutePeriodicEffect, NewHandle);

			// If this is a periodic stacking effect, make sure that it's in sync with the others.
			float FirstDelay = -1.f;
			bool bApplyToNextTick = true;
			if (NewEffect.Spec.GetStackingType() != EGameplayEffectStackingPolicy::Unlimited)
			{
				for (FActiveGameplayEffect& Effect : GameplayEffects)
				{
					if (Effect.CanBeStacked(NewEffect))
					{
						FirstDelay = TimerManager.GetTimerRemaining(Effect.PeriodHandle);
						bApplyToNextTick = TimerManager.IsTimerPending(Effect.PeriodHandle);
						break;
					}
				}
			}
			
			if (bApplyToNextTick)
			{
				TimerManager.SetTimerForNextTick(Delegate); // not part of a stack or the first item on the stack, execute once right away
			}

			TimerManager.SetTimer(NewEffect.PeriodHandle, Delegate, NewEffect.Spec.GetPeriod(), true, FirstDelay); // this is going to be off by a frame for stacking because of the pending list
		}
	}
	
	if (InPredictionKey.IsValidKey() == false || IsNetAuthority())	// Clients predicting a GameplayEffect must not call MarkItemDirty
	{
		MarkItemDirty(NewEffect);
	}
	else
	{
		// Clients predicting should call MarkArrayDirty to force the internal replication map to be rebuilt.
		MarkArrayDirty();

		// Once replicated state has caught up to this prediction key, we must remove this gameplay effect.
		InPredictionKey.NewRejectOrCaughtUpDelegate(FPredictionKeyEvent::CreateUObject(Owner, &UAbilitySystemComponent::RemoveActiveGameplayEffect_NoReturn, NewEffect.Handle));
		
	}

	InternalOnActiveGameplayEffectAdded(NewEffect);

	return NewEffect;
}

void FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectAdded(FActiveGameplayEffect& Effect)
{
	// Add our ongoing tag requirements to the dependency map. We will actually check for these tags below.
	for (const FGameplayTag& Tag : Effect.Spec.Def->OngoingTagRequirements.IgnoreTags)
	{
		ActiveEffectTagDependencies.FindOrAdd(Tag).Add(Effect.Handle);
	}

	for (const FGameplayTag& Tag : Effect.Spec.Def->OngoingTagRequirements.RequireTags)
	{
		ActiveEffectTagDependencies.FindOrAdd(Tag).Add(Effect.Handle);
	}

	// Update our owner with the tags this GameplayEffect grants them
	Owner->UpdateTagMap(Effect.Spec.Def->OwnedTagsContainer, 1);
	for (const FGameplayEffectCue& Cue : Effect.Spec.Def->GameplayCues)
	{
		Owner->UpdateTagMap(Cue.GameplayCueTags, 1);
	}

	// Check if we should actually be turned on or not (this will turn us on for the first time)
	FGameplayTagContainer OwnerTags;
	Owner->GetOwnedGameplayTags(OwnerTags);

	// Calc all of our modifier magnitudes now. Some may need to update later based on attributes changing, etc, but those should
	// be done through delegate callbacks.
	Effect.Spec.CalculateModifierMagnitudes();

	// @todo: Probably do the initial mod spec magnitude calculations right here, but only on the server
	// @todo: Shouldn't the rest of the stuff about dependencies only run on the server?
	Effect.CheckOngoingTagRequirements(OwnerTags, *this);
}

bool FActiveGameplayEffectsContainer::RemoveActiveGameplayEffect(FActiveGameplayEffectHandle Handle)
{
	// Could make this a map for quicker lookup
	for (int32 Idx = 0; Idx < GameplayEffects.Num(); ++Idx)
	{
		if (GameplayEffects[Idx].Handle == Handle)
		{
			return InternalRemoveActiveGameplayEffect(Idx);
		}
	}

	ABILITY_LOG(Warning, TEXT("RemoveActiveGameplayEffect called with invalid Handle: %s"), *Handle.ToString());
	return false;
}

bool FActiveGameplayEffectsContainer::InternalRemoveActiveGameplayEffect(int32 Idx)
{
	SCOPE_CYCLE_COUNTER(STAT_RemoveActiveGameplayEffect);

	if (ensure(Idx < GameplayEffects.Num()))
	{
		FActiveGameplayEffect& Effect = GameplayEffects[Idx];

		bool ShouldInvokeGameplayCueEvent = true;
		if (!IsNetAuthority() && Effect.PredictionKey.IsValidKey() && Effect.PredictionKey.WasReceived() == false)
		{
			// This was an effect that we predicted. Don't invoke GameplayCue event if we have another GameplayEffect that shares the same predictionkey and was received from the server
			if (HasReceivedEffectWithPredictedKey(Effect.PredictionKey))
			{
				ShouldInvokeGameplayCueEvent = false;
			}
		}

		if (ShouldInvokeGameplayCueEvent)
		{
			Owner->InvokeGameplayCueEvent(GameplayEffects[Idx].Spec, EGameplayCueEvent::Removed);
		}

		InternalOnActiveGameplayEffectRemoved(Effect);

		if (GameplayEffects[Idx].DurationHandle.IsValid())
		{
			Owner->GetWorld()->GetTimerManager().ClearTimer(GameplayEffects[Idx].DurationHandle);
		}
		if (GameplayEffects[Idx].PeriodHandle.IsValid())
		{
			Owner->GetWorld()->GetTimerManager().ClearTimer(GameplayEffects[Idx].PeriodHandle);
		}

		// Remove our tag requirements from the dependancy map
		for (const FGameplayTag& Tag : Effect.Spec.Def->OngoingTagRequirements.IgnoreTags)
		{
			auto Ptr = ActiveEffectTagDependencies.Find(Tag);
			if (Ptr)
			{
				Ptr->Remove(Effect.Handle);
				if (Ptr->Num() <= 0)
				{
					ActiveEffectTagDependencies.Remove(Tag);
				}
			}
		}

		for (const FGameplayTag& Tag : Effect.Spec.Def->OngoingTagRequirements.RequireTags)
		{
			auto Ptr = ActiveEffectTagDependencies.Find(Tag);
			if (Ptr)
			{
				Ptr->Remove(Effect.Handle);
				if (Ptr->Num() <= 0)
				{
					ActiveEffectTagDependencies.Remove(Tag);
				}
			}
		}


		// Fixme: Yuck?
		for (const FGameplayModifierInfo& Mod : Effect.Spec.Def->Modifiers)
		{
			if (Mod.Attribute.IsValid())
			{
				FAggregatorRef* RefPtr = AttributeAggregatorMap.Find(Mod.Attribute);
				if (RefPtr)
				{
					RefPtr->Get()->RemoveMod(Effect.Handle);
				}
			}
		}



		GameplayEffects.RemoveAtSwap(Idx);
		MarkArrayDirty();

		// Hack: force netupdate on owner. This isn't really necessary in real gameplay but is nice
		// during debugging where breakpoints or pausing can messup network update times. Open issue
		// with network team.
		Owner->GetOwner()->ForceNetUpdate();

		StacksNeedToRecalculate();

		return true;
	}

	ABILITY_LOG(Warning, TEXT("InternalRemoveActiveGameplayEffect called with invalid index: %d"), Idx);
	return false;
}

/** This does cleanup that has to happen whether the effect is being removed locally or due to replication */
void FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectRemoved(const FActiveGameplayEffect& Effect)
{
	Effect.OnRemovedDelegate.Broadcast();

	if (Effect.Spec.Def)
	{
		// Update gameplaytag count and broadcast delegate if we are at 0
		IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
		Owner->UpdateTagMap(Effect.Spec.Def->OwnedTagsContainer, -1);

		for (const FGameplayEffectCue& Cue : Effect.Spec.Def->GameplayCues)
		{
			Owner->UpdateTagMap(Cue.GameplayCueTags, -1);
		}
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("InternalOnActiveGameplayEffectRemoved called with no GameplayEffect: %s"), *Effect.Handle.ToString());
	}
}

void FActiveGameplayEffectsContainer::OnOwnerTagChange(FGameplayTag TagChange, int32 NewCount)
{
	// It may be beneficial to do a scoped lock on attribute reeevalluation during this dunction

	auto Ptr = ActiveEffectTagDependencies.Find(TagChange);
	if (Ptr)
	{
		FGameplayTagContainer OwnerTags;
		Owner->GetOwnedGameplayTags(OwnerTags);

		TSet<FActiveGameplayEffectHandle>& Handles = *Ptr;
		for (const FActiveGameplayEffectHandle& Handle : Handles)
		{
			FActiveGameplayEffect* ActiveEffect = GetActiveGameplayEffect(Handle);
			if (ActiveEffect)
			{
				ActiveEffect->CheckOngoingTagRequirements(OwnerTags, *this);
			}
		}
	}
}

bool FActiveGameplayEffectsContainer::IsNetAuthority() const
{
	check(Owner);
	return Owner->IsOwnerActorAuthoritative();
}

void FActiveGameplayEffectsContainer::PreDestroy()
{
	
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

void FActiveGameplayEffectsContainer::CheckDuration(FActiveGameplayEffectHandle Handle)
{
	for (int32 Idx = 0; Idx < GameplayEffects.Num(); ++Idx)
	{
		FActiveGameplayEffect& Effect = GameplayEffects[Idx];
		if (Effect.Handle == Handle)
		{
			FTimerManager& TimerManager = Owner->GetWorld()->GetTimerManager();

			// The duration may have changed since we registered this callback with the timer manager.
			// Make sure that this effect should really be destroyed now
			float Duration = Effect.GetDuration();
			float CurrentTime = GetWorldTime();
			if (Duration > 0.f && (Effect.StartWorldTime + Duration) < CurrentTime)
			{
				// This gameplay effect has hit its duration. Check if it needs to execute one last time before removing it.
				if (Effect.PeriodHandle.IsValid())
				{
					float PeriodTimeRemaining = TimerManager.GetTimerRemaining(Effect.PeriodHandle);
					if (FMath::IsNearlyEqual(PeriodTimeRemaining, 0.f))
					{
						ExecuteActiveEffectsFrom(Effect.Spec);
					}
				}

				InternalRemoveActiveGameplayEffect(Idx);
			}
			else
			{
				// check the time remaining for the current gameplay effect duration timer
				// if it is less than zero create a new callback with the correct time remaining
				float TimeRemaining = TimerManager.GetTimerRemaining(Effect.DurationHandle);
				if (TimeRemaining <= 0.f)
				{
					FTimerDelegate Delegate = FTimerDelegate::CreateUObject(Owner, &UAbilitySystemComponent::CheckDurationExpired, Effect.Handle);
					TimerManager.SetTimer(Effect.DurationHandle, Delegate, (Effect.StartWorldTime + Duration) - CurrentTime, false);
				}
			}

			break;
		}
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
				Effect.Spec.bTopOfStack = false;
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
							ABILITY_LOG(Warning, TEXT("%s uses unhandled stacking rule %s."), *Effect.Spec.Def->GetPathName(), *EGameplayEffectStackingPolicyToString(Effect.Spec.GetStackingType()));
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
		StackedEffect->Spec.bTopOfStack = true;
	}
}

bool FActiveGameplayEffectsContainer::CanApplyAttributeModifiers(const UGameplayEffect *GameplayEffect, float Level, const FGameplayEffectContextHandle& EffectContext)
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsCanApplyAttributeModifiers);

	FGameplayEffectSpec	Spec(GameplayEffect, EffectContext, Level);
	
	for (const FModifierSpec& Mod : Spec.Modifiers)
	{
		// It only makes sense to check additive operators
		if (Mod.Info.ModifierOp == EGameplayModOp::Additive)
		{
			const UAttributeSet* Set = Owner->GetAttributeSubobject(Mod.Info.Attribute.GetAttributeSetClass());
			float CurrentValue = Mod.Info.Attribute.GetNumericValueChecked(Set);
			float CostValue = Spec.GetModifierMagnitude(Mod);

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
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsGetActiveEffectsTimeRemaining);

	float CurrentTime = GetWorldTime();

	TArray<float>	ReturnList;

	for (const FActiveGameplayEffect& Effect : GameplayEffects)
	{
		if (!Query.Matches(Effect))
		{
			continue;
		}

		float Elapsed = CurrentTime - Effect.StartWorldTime;
		float Duration = Effect.GetDuration();

		ReturnList.Add(Duration - Elapsed);
	}

	// Note: keep one return location to avoid copy operation.
	return ReturnList;
}

TArray<float> FActiveGameplayEffectsContainer::GetActiveEffectsDuration(const FActiveGameplayEffectQuery Query) const
{
	SCOPE_CYCLE_COUNTER(STAT_GameplayEffectsGetActiveEffectsDuration);

	TArray<float>	ReturnList;

	for (const FActiveGameplayEffect &Effect : GameplayEffects)
	{
		if (!Query.Matches(Effect))
		{
			continue;
		}

		ReturnList.Add(Effect.GetDuration());
	}

	// Note: keep one return location to avoid copy operation.
	return ReturnList;
}

void FActiveGameplayEffectsContainer::RemoveActiveEffects(const FActiveGameplayEffectQuery Query)
{
	for (int32 idx=0; idx < GameplayEffects.Num(); ++idx)
	{
		const FActiveGameplayEffect& Effect = GameplayEffects[idx];
		if (Query.Matches(Effect))
		{
			InternalRemoveActiveGameplayEffect(idx);
			idx--;
		}
	}
}

FOnGameplayAttributeChange& FActiveGameplayEffectsContainer::RegisterGameplayAttributeEvent(FGameplayAttribute Attribute)
{
	return AttributeChangeDelegates.FindOrAdd(Attribute);
}

bool FActiveGameplayEffectsContainer::HasReceivedEffectWithPredictedKey(FPredictionKey PredictionKey) const
{
	for (const FActiveGameplayEffect& Effect : GameplayEffects)
	{
		if (Effect.PredictionKey == PredictionKey && Effect.PredictionKey.WasReceived() == true)
		{
			return true;
		}
	}

	return false;
}

bool FActiveGameplayEffectsContainer::HasPredictedEffectWithPredictedKey(FPredictionKey PredictionKey) const
{
	for (const FActiveGameplayEffect& Effect : GameplayEffects)
	{
		if (Effect.PredictionKey == PredictionKey && Effect.PredictionKey.WasReceived() == false)
		{
			return true;
		}
	}

	return false;
}

// --------------------------------------------------------------------------------------------------------------------------------------------------------
//
//	Misc
//
// --------------------------------------------------------------------------------------------------------------------------------------------------------

namespace GlobalActiveGameplayEffectHandles
{
	static TMap<FActiveGameplayEffectHandle, TWeakObjectPtr<UAbilitySystemComponent>>	Map;
}

FActiveGameplayEffectHandle FActiveGameplayEffectHandle::GenerateNewHandle(UAbilitySystemComponent* OwningComponent)
{
	static int32 GHandleID=0;
	FActiveGameplayEffectHandle NewHandle(GHandleID++);

	TWeakObjectPtr<UAbilitySystemComponent> WeakPtr(OwningComponent);

	GlobalActiveGameplayEffectHandles::Map.Add(NewHandle, WeakPtr);

	return NewHandle;
}

UAbilitySystemComponent* FActiveGameplayEffectHandle::GetOwningAbilitySystemComponent()
{
	TWeakObjectPtr<UAbilitySystemComponent>* Ptr = GlobalActiveGameplayEffectHandles::Map.Find(*this);
	if (Ptr)
	{
		return Ptr->Get();
	}

	return nullptr;	
}
// -----------------------------------------------------------------

bool FActiveGameplayEffectQuery::Matches(const FActiveGameplayEffect& Effect) const
{
	if (TagContainer)
	{
		if (!Effect.Spec.Def->OwnedTagsContainer.MatchesAny(*TagContainer, false))
		{
			return false;
		}
	}	
	
	if (ModifyingAttribute.IsValid())
	{
		bool FailedModifyingAttributeCheck = true;
		for (const FModifierSpec& Modifier : Effect.Spec.Modifiers)
		{
			if (Modifier.Info.Attribute == ModifyingAttribute)
			{
				FailedModifyingAttributeCheck = false;
				break;
			}
		}
		if (FailedModifyingAttributeCheck)
		{
			return false;
		}
	}

	return true;
}

