// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayEffectTypes.h"

// #include "GameplayEffectAggregator.generated.h"

struct GAMEPLAYABILITIES_API FAggregatorEvaluateParameters
{
	FAggregatorEvaluateParameters() : SourceTags(nullptr), TargetTags(nullptr) { }

	const FGameplayTagContainer*	SourceTags;
	const FGameplayTagContainer*	TargetTags;

	/** If any tags are specified in the filter, a mod's owning active gameplay effect's source tags must match ALL of them in order for the mod to count during evaluation */
	FGameplayTagContainer	AppliedSourceTagFilter;

	/** If any tags are specified in the filter, a mod's owning active gameplay effect's target tags must match ALL of them in order for the mod to count during evaluation */
	FGameplayTagContainer	AppliedTargetTagFilter;
};

struct GAMEPLAYABILITIES_API FAggregatorMod
{
	const FGameplayTagRequirements*	SourceTagReqs;
	const FGameplayTagRequirements*	TargetTagReqs;

	float EvaluatedMagnitude;		// Magnitude this mod was last evaluated at

	FActiveGameplayEffectHandle ActiveHandle;	// Handle of the active GameplayEffect we are tied to (if any)

	bool Qualifies(const FAggregatorEvaluateParameters& Parameters) const;
};


struct GAMEPLAYABILITIES_API FAggregator : public TSharedFromThis<FAggregator>
{
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnAggregatorDirty, FAggregator*);

	FAggregator(float InBaseValue=0.f) : BaseValue(InBaseValue) { }

	/** Simple accessor to base value */
	float GetBaseValue() const;
	void SetBaseValue(float NewBaseValue);
	
	void ExecModOnBaseValue(TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude);
	static float StaticExecModOnBaseValue(float BaseValue, TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude);

	void AddMod(float EvaluatedData, TEnumAsByte<EGameplayModOp::Type> ModifierOp, const FGameplayTagRequirements*	SourceTagReqs, const FGameplayTagRequirements* TargetTagReqs, FActiveGameplayEffectHandle ActiveHandle = FActiveGameplayEffectHandle());
	void RemoveMod(FActiveGameplayEffectHandle ActiveHandle);

	/** Evaluates the Aggregator with the internal base value and given parameters */
	float Evaluate(const FAggregatorEvaluateParameters& Parameters) const;

	/** Evaluates the Aggregator with an arbitrary base value */
	float EvaluateWithBase(float InlineBaseValue, const FAggregatorEvaluateParameters& Parameters) const;

	/** Evaluates the Aggregator to compute its "bonus" (final - base) value */
	float EvaluateBonus(const FAggregatorEvaluateParameters& Parameters) const;

	void TakeSnapshotOf(const FAggregator& AggToSnapshot);

	FOnAggregatorDirty OnDirty;

	void AddModsFrom(const FAggregator& SourceAggregator);

private:

	void BroadcastOnDirty();

	float	BaseValue;
	TArray<FAggregatorMod>	Mods[EGameplayModOp::Max];

	float SumMods(const TArray<FAggregatorMod> &Mods, float Bias, const FAggregatorEvaluateParameters& Parameters) const;
	void RemoveModsWithActiveHandle(TArray<FAggregatorMod>& Mods, FActiveGameplayEffectHandle ActiveHandle);

	friend struct FAggregator;
};

struct GAMEPLAYABILITIES_API FAggregatorRef
{
	FAggregatorRef() { }
	FAggregatorRef(FAggregator* InData) : Data(InData) { }

	FAggregator* Get() const { return Data.Get(); }

	TSharedPtr<FAggregator>	Data;

	void TakeSnapshotOf(const FAggregatorRef& RefToSnapshot);
};
