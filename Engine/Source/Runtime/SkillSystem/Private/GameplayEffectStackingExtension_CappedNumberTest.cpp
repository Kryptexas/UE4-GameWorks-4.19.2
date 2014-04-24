// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "Crc.h"

const int32 MaxNumInStack = 2;

UGameplayEffectStackingExtension_CappedNumberTest::UGameplayEffectStackingExtension_CappedNumberTest(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	Handle = FCrc::StrCrc32("UGameplayEffectStackingExtension_CappedNumberTest");
}

void UGameplayEffectStackingExtension_CappedNumberTest::CalculateStack(TArray<FActiveGameplayEffect*>& CustomGameplayEffects, FActiveGameplayEffectsContainer& Container, FActiveGameplayEffect& CurrentEffect)
{
	// this effect shouldn't be in the array so be sure to count it as well
	int32 EffectiveCount = FMath::Min(CustomGameplayEffects.Num() + 1, MaxNumInStack);
	
	// find the most recent time one of these was applied
	float StartTime = CurrentEffect.StartTime;

	for (FActiveGameplayEffect* Effect : CustomGameplayEffects)
	{
		StartTime = FMath::Max(StartTime, Effect->StartTime);
	}

	{
		// set the start time to be equal to the most recent start time so that the stacked effects stick around if a new effect has been added
		CurrentEffect.StartTime = StartTime;
		int32 Idx = 0;
		while (Idx < FMath::Min(MaxNumInStack - 1, CustomGameplayEffects.Num()))
		{
			CustomGameplayEffects[Idx]->StartTime = StartTime;
			++Idx;
		}

		// we don't need any effects beyond the cap
		while (Idx < CustomGameplayEffects.Num())
		{
			Container.RemoveActiveGameplayEffect(CustomGameplayEffects[Idx]->Handle);
 			++Idx;
		}
	}

	for (FModifierSpec Mod : CurrentEffect.Spec.Modifiers)
	{
		if (Mod.Info.OwnedTags.HasTag("Stackable"))
		{
			// remove any stacking information that was already applied to the current modifier
			for (int32 Idx = 0; Idx < Mod.Aggregator.Get()->Mods[EGameplayModOp::Multiplicitive].Num(); ++Idx)
			{
				FAggregatorRef& Agg = Mod.Aggregator.Get()->Mods[EGameplayModOp::Multiplicitive][Idx];
				if (Agg.Get()->BaseData.Tags.HasTag("Stack.CappedNumber"))
				{
					Mod.Aggregator.Get()->Mods[EGameplayModOp::Multiplicitive].RemoveAtSwap(Idx);
					--Idx;
				}
			}
			FGameplayModifierInfo ModInfo;
			ModInfo.Magnitude.SetValue(EffectiveCount);
			ModInfo.ModifierOp = EGameplayModOp::Multiplicitive;
			ModInfo.OwnedTags.AddTag("Stack.CappedNumber");
			ModInfo.Attribute = Mod.Info.Attribute;

			TSharedPtr<FGameplayEffectLevelSpec> ModifierLevel(TSharedPtr< FGameplayEffectLevelSpec >(new FGameplayEffectLevelSpec()));
			ModifierLevel->ApplyNewDef(ModInfo.LevelInfo, ModifierLevel);

			FModifierSpec ModSpec(ModInfo, ModifierLevel, NULL);

			ModSpec.ApplyModTo(Mod, true);
		}
	}
}
