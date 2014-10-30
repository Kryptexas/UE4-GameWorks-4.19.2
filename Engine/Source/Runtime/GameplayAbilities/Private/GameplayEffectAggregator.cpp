// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "GameplayEffectAggregator.h"

bool FAggregatorMod::Qualifies(const FAggregatorEvaluateParameters& Parameters) const
{
	bool SourceMet = (!SourceTagReqs || SourceTagReqs->RequirementsMet(Parameters.SourceTags));
	bool TargetMet = (!TargetTagReqs || TargetTagReqs->RequirementsMet(Parameters.TargetTags));

	return SourceMet && TargetMet;
}

float FAggregator::Evaluate(const FAggregatorEvaluateParameters& Parameters) const
{
	return EvaluateWithBase(BaseValue, Parameters);
}

float FAggregator::EvaluateWithBase(float InlineBaseValue, const FAggregatorEvaluateParameters& Parameters) const
{
	for (const FAggregatorMod& Mod : Mods[EGameplayModOp::Override])
	{
		if (Mod.Qualifies(Parameters))
		{
			return Mod.EvaluatedMagnitude;
		}
	}

	float Additive = SumMods(Mods[EGameplayModOp::Additive], 0.f, Parameters);
	float Multiplicitive = SumMods(Mods[EGameplayModOp::Multiplicitive], 1.f, Parameters);
	float Division = SumMods(Mods[EGameplayModOp::Division], 1.f, Parameters);

	if (FMath::IsNearlyZero(Division))
	{
		ABILITY_LOG(Warning, TEXT("Division summation was 0.0f in FAggregator."));
		Division = 1.f;
	}

	return ((InlineBaseValue + Additive) * Multiplicitive) / Division;
}

void FAggregator::SetBaseValue(float NewBaseValue)
{
	BaseValue = NewBaseValue;
	BroadcastOnDirty();
}

float FAggregator::StaticExecModOnBaseValue(float BaseValue, TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude)
{
	switch (ModifierOp)
	{
	case EGameplayModOp::Override:
	{
		BaseValue = EvaluatedMagnitude;
		break;
	}
	case EGameplayModOp::Additive:
	{
		BaseValue += EvaluatedMagnitude;
		break;
	}
	case EGameplayModOp::Multiplicitive:
	{
		BaseValue *= EvaluatedMagnitude;
		break;
	}
	case EGameplayModOp::Division:
	{
		if (FMath::IsNearlyZero(EvaluatedMagnitude) == false)
		{
			BaseValue /= EvaluatedMagnitude;
		}
		break;
	}
	}

	return BaseValue;
}

void FAggregator::ExecModOnBaseValue(TEnumAsByte<EGameplayModOp::Type> ModifierOp, float EvaluatedMagnitude)
{
	BaseValue = StaticExecModOnBaseValue(BaseValue, ModifierOp, EvaluatedMagnitude);
	BroadcastOnDirty();
}

float FAggregator::SumMods(const TArray<FAggregatorMod> &Mods, float Bias, const FAggregatorEvaluateParameters& Parameters) const
{
	float Sum = Bias;

	for (const FAggregatorMod& Mod : Mods)
	{
		if (Mod.Qualifies(Parameters))
		{
			Sum += (Mod.EvaluatedMagnitude - Bias);
		}
	}

	return Sum;
}

void FAggregator::AddMod(float EvaluatedMagnitude, TEnumAsByte<EGameplayModOp::Type> ModifierOp, const FGameplayTagRequirements* SourceTagReqs, const FGameplayTagRequirements* TargetTagReqs, FActiveGameplayEffectHandle ActiveHandle)
{
	TArray<FAggregatorMod> &ModList = Mods[ModifierOp];

	int32 NewIdx = ModList.AddUninitialized();
	FAggregatorMod& NewMod = ModList[NewIdx];

	NewMod.SourceTagReqs = SourceTagReqs;
	NewMod.TargetTagReqs = TargetTagReqs;
	NewMod.EvaluatedMagnitude = EvaluatedMagnitude;
	NewMod.ActiveHandle = ActiveHandle;

	BroadcastOnDirty();
}

void FAggregator::RemoveMod(FActiveGameplayEffectHandle ActiveHandle)
{
	if (ActiveHandle.IsValid())
	{
		RemoveModsWithActiveHandle(Mods[EGameplayModOp::Additive], ActiveHandle);
		RemoveModsWithActiveHandle(Mods[EGameplayModOp::Multiplicitive], ActiveHandle);
		RemoveModsWithActiveHandle(Mods[EGameplayModOp::Division], ActiveHandle);
		RemoveModsWithActiveHandle(Mods[EGameplayModOp::Override], ActiveHandle);
	}

	BroadcastOnDirty();
}

void FAggregator::RemoveModsWithActiveHandle(TArray<FAggregatorMod>& Mods, FActiveGameplayEffectHandle ActiveHandle)
{
	check(ActiveHandle.IsValid());

	for(int32 idx=Mods.Num()-1; idx >= 0; --idx)
	{
		if (Mods[idx].ActiveHandle == ActiveHandle)
		{
			Mods.RemoveAtSwap(idx, 1, false);
		}
	}
}

void FAggregator::TakeSnapshotOf(const FAggregator& AggToSnapshot)
{
	BaseValue = AggToSnapshot.BaseValue;
	for (int32 idx=0; idx < ARRAY_COUNT(Mods); ++idx)
	{
		Mods[idx] = AggToSnapshot.Mods[idx];
	}
}

void FAggregator::BroadcastOnDirty()
{
	if (!CallbacksDisabled)
	{
		OnDirty.Broadcast(this);
	}

}

void FAggregator::DisableCallbacks()
{
	CallbacksDisabled= true;
}

void FAggregator::EnabledCallbacks()
{
	CallbacksDisabled = false;
}

void FAggregatorRef::TakeSnapshotOf(const FAggregatorRef& RefToSnapshot)
{
	if (RefToSnapshot.Data.IsValid())
	{
		FAggregator* SrcData = RefToSnapshot.Data.Get();

		Data = TSharedPtr<FAggregator>(new FAggregator());
		Data->TakeSnapshotOf(*SrcData);
	}
	else
	{
		Data.Reset();
	}
}
