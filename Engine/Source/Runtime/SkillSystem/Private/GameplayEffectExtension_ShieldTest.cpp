// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
#include "GameplayEffectExtension_ShieldTest.h"
#include "GameplayTagsModule.h"
#include "AttributeComponent.h"
#include "SkillSystemTestAttributeSet.h"

UGameplayEffectExtension_ShieldTest::UGameplayEffectExtension_ShieldTest(const class FPostConstructInitializeProperties& PCIP)
: Super(PCIP)
{

}


void UGameplayEffectExtension_ShieldTest::PreGameplayEffectExecute(const FGameplayModifierEvaluatedData &SelfData, FGameplayEffectModCallbackData &Data) const
{
	IGameplayTagsModule& GameplayTagsModule = IGameplayTagsModule::Get();
	UAttributeComponent *Source = Data.EffectSpec.InstigatorContext.GetOriginalInstigatorAttributeComponent();

	// FIXME: some annoyances here: Damage about to be applied = Data.EvaluatedData.Magnitude = negative. Do some sign flipping here that would make more sense if we were dealing with
	// 'damage' (positive) instead of 'health' (negative)

	// How much damage we will absorb
	float ShieldedDamage = FMath::Min(-Data.EvaluatedData.Magnitude, SelfData.Magnitude);

	// Decrease the magnitude of damage done
	Data.EvaluatedData.Magnitude += ShieldedDamage;

	if (ShieldedDamage >= SelfData.Magnitude)
	{
		check(SelfData.Handle.IsValid());

		// The shield is now depleted, so remove the gameplay effect altogether
		bool RemovedSomething = Data.Target.RemoveActiveGameplayEffect(SelfData.Handle);
		check(RemovedSomething);

		// Note we could get in trouble with the above checks, depending on how the GameplayEffect that called us was setup. 
		// For example if we were applied to a periodic damage effect, and were copied as a snapshot, our shield could expire
		// but our aggregator on the DOT gameplayeffect would still exist with a now invalid handle. This is ok, and possibly powerful
		// but just means that the way an effect like this is written would have to take it into account. This one doesn't.
		//
		// It even opens up interesting possibilities. You could have a shield specifically designed for dots: something that would absorb
		// the first X damage from each DOT applied to you. Practicaly? Maybe not, but demonstrates power of this setup.
	}
	else if (ShieldedDamage > 0.f)
	{
		// Apply a GameplayEffect to remove some of our shield
		UGameplayEffect * LocalShieldRemoval = ShieldRemoveGameplayEffect;

		if (!LocalShieldRemoval)
		{
			UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

			// Since this is a test class and we don't want to tie it any actual content assets, just construct a GameplayEffect here.
			LocalShieldRemoval = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("ShieldAbsorbRemoval"))));
			LocalShieldRemoval->Modifiers.SetNum(1);
			LocalShieldRemoval->Modifiers[0].Magnitude.SetValue(-ShieldedDamage);
			LocalShieldRemoval->Modifiers[0].ModifierType = EGameplayMod::ActiveGE;
			LocalShieldRemoval->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
			LocalShieldRemoval->Modifiers[0].Attribute.SetUProperty(HealthProperty);
			LocalShieldRemoval->Modifiers[0].OwnedTags.AddTag(GameplayTagsModule.GetGameplayTagsManager().RequestGameplayTag(FName(TEXT("ShieldAbsorb"))));
			LocalShieldRemoval->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;
			LocalShieldRemoval->Period.Value = UGameplayEffect::NO_PERIOD;
		}

		Data.Target.ApplyGameplayEffectToSelf(LocalShieldRemoval, ShieldedDamage, Source->GetOwner(), FModifierQualifier().ExclusiveTarget(SelfData.Handle));
	}
}

void UGameplayEffectExtension_ShieldTest::PostGameplayEffectExecute(const FGameplayModifierEvaluatedData &SelfData, const FGameplayEffectModCallbackData &Data) const
{

}