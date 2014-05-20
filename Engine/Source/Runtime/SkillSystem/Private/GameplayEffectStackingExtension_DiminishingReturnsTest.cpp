// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "GameplayTagsModule.h"

UCurveTable* GetCurveTable()
{
	FString CSV(TEXT(", 0, 1, 2, 3, 100\r\nReturnValue, 0, 5, 7, 8, 105"));

	UCurveTable* CurveTable = Cast<UCurveTable>(StaticConstructObject(UCurveTable::StaticClass(), GetTransientPackage(), FName(TEXT("TempCurveTable"))));
	CurveTable->CreateTableFromCSVString(CSV);

	return CurveTable;
}

FGameplayTag RequestGameplayTag_DiminishingReturnsTest(FName Name)
{
	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
	return GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(Name);
}


UGameplayEffectStackingExtension_DiminishingReturnsTest::UGameplayEffectStackingExtension_DiminishingReturnsTest(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{
	Handle = FCrc::StrCrc32("UGameplayEffectStackingExtension_DiminishingReturnsTest");
}

void UGameplayEffectStackingExtension_DiminishingReturnsTest::CalculateStack(TArray<FActiveGameplayEffect*>& CustomGameplayEffects, FActiveGameplayEffectsContainer& Container, FActiveGameplayEffect& CurrentEffect)
{
	// look up the correct value in the curve table
	UCurveTable* CurveTable = GetCurveTable();
	float Multiplier = 1.f;
	if (CurveTable)
	{
		FRichCurve* RichCurve = CurveTable->FindCurve(FName(TEXT("ReturnValue")), TEXT("Stack"));
		if (RichCurve)
		{
			Multiplier = RichCurve->Eval(CustomGameplayEffects.Num() + 1); // + 1 because the current effect isn't in the TArray
		}
	}

// 	// remove any old stack modifiers
// 	for (int32 Idx = 0; Idx < CurrentEffect.Spec.Modifiers.Num(); ++Idx)
// 	{
// 		if (CurrentEffect.Spec.Modifiers[Idx].Info.OwnedTags.HasTag("Stack"))
// 		{
// 			CurrentEffect.Spec.Modifiers.RemoveAtSwap(Idx);
// 			--Idx;
// 		}
// 	}
// 	
// 	for (FModifierSpec Mod : CurrentEffect.Spec.Modifiers)
// 	{
// 		if (Mod.Info.OwnedTags.HasTag("Stackable"))
// 		{
// 			FGameplayModifierInfo ModInfo;
// 			ModInfo.Magnitude.SetValue(Multiplier);
// 			ModInfo.ModifierOp = EGameplayModOp::Multiplicitive;
// 			ModInfo.OwnedTags.AddTag("Stack");
// 			ModInfo.Attribute = Mod.Info.Attribute;
// 
// 			TSharedPtr<FGameplayEffectLevelSpec> ModifierLevel(TSharedPtr< FGameplayEffectLevelSpec >(new FGameplayEffectLevelSpec()));
// 			ModifierLevel->ApplyNewDef(ModInfo.LevelInfo, ModifierLevel);
// 
// 			FModifierSpec ModSpec(ModInfo, ModifierLevel, NULL);
// 
// 			ModSpec.ApplyModTo(Mod, false);
// 			CurrentEffect.Spec.Modifiers.Add(ModSpec);
// 		}
// 	}

	for (FModifierSpec Mod : CurrentEffect.Spec.Modifiers)
	{
		if (Mod.Info.OwnedTags.HasTag(RequestGameplayTag_DiminishingReturnsTest("Stackable"), EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit))
		{
			// remove any stacking information that was already applied to the current modifier
			for (int32 Idx = 0; Idx < Mod.Aggregator.Get()->Mods[EGameplayModOp::Multiplicitive].Num(); ++Idx)
			{
				FAggregatorRef& Agg = Mod.Aggregator.Get()->Mods[EGameplayModOp::Multiplicitive][Idx];
				if (Agg.Get()->BaseData.Tags.HasTag(RequestGameplayTag_DiminishingReturnsTest("Stack.DiminishingReturns"), EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit))
				{
					Mod.Aggregator.Get()->Mods[EGameplayModOp::Multiplicitive].RemoveAtSwap(Idx);
					--Idx;
				}
			}
			FGameplayModifierInfo ModInfo;
			ModInfo.Magnitude.SetValue(Multiplier);
			ModInfo.ModifierOp = EGameplayModOp::Multiplicitive;
			ModInfo.OwnedTags.AddTag(RequestGameplayTag_DiminishingReturnsTest("Stack.DiminishingReturns"));
			ModInfo.Attribute = Mod.Info.Attribute;

			TSharedPtr<FGameplayEffectLevelSpec> ModifierLevel(TSharedPtr< FGameplayEffectLevelSpec >(new FGameplayEffectLevelSpec()));
			ModifierLevel->ApplyNewDef(ModInfo.LevelInfo, ModifierLevel);

			FModifierSpec ModSpec(ModInfo, ModifierLevel, NULL);

			ModSpec.ApplyModTo(Mod, true);
		}
	}
}
