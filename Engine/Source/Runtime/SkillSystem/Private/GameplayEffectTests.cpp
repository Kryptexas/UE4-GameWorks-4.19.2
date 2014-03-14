// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemModulePrivatePCH.h"
//#include "SkillSystemTestPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayEffectsTest, "SkillSystem.GameplayEffects", EAutomationTestFlags::ATF_Editor)

#define SKILL_TEST_TEXT( Format, ... ) FString::Printf(TEXT("%s - %d: %s"), TEXT(__FILE__) , __LINE__ , *FString::Printf(TEXT(Format), ##__VA_ARGS__) )

void GameplayTest_TickWorld(UWorld *World, float Time)
{
	const float step = 0.1f;
	while(Time > 0.f)
	{
		World->Tick(ELevelTick::LEVELTICK_All, FMath::Min(Time, step) );
		Time-=step;

		// This is terrible but required for subticking like this.
		// we could always cache the real GFrameCounter at the start of our tests and restore it when finished.
		GFrameCounter++;
	}
}

void MySharedPointerTest()
{
	// Test that outside will be invalid after inside goes out of scope
	{
		FAggregatorRef	Outside;
		check(!Outside.IsValid());
		{
			FAggregatorRef Inside(new FAggregator());
			check(Inside.IsValid());

			Outside.SetSoftRef(&Inside);
			check(Outside.IsValid());
		}

		check(!Outside.IsValid());
	}

	// Test that outside will valid since it calls MakeHardRef
	{
		FAggregatorRef	Outside;
		check(!Outside.IsValid());
		{
			FAggregatorRef Inside(new FAggregator());
			check(Inside.IsValid());

			Outside.SetSoftRef(&Inside);
			check(Outside.IsValid());

			// The difference
			Outside.MakeHardRef();
		}
	
		check(Outside.IsValid());
	}

	// TArray test
	TArray< FAggregatorRef >	Array;
	TArray< FAggregatorRef >	Array2;
	Array2.SetNum(640);

	{
		FAggregatorRef Inside(new FAggregator());
		Array.SetNum(1);
		Array[0].SetSoftRef(&Inside);
		Array[0].MakeHardRef();
	}

	check(Array[0].IsValid());
	Array.SetNum(1024);
	check(Array[0].IsValid());
}

void MySharedPointerTest_Array()
{
	TArray< TSharedPtr<FAggregator> >	Array1;
	TArray< TSharedPtr<FAggregator> >	Array2;
	
	Array1.Emplace( TSharedPtr<FAggregator>( new FAggregator() ) );

	Array2 = Array1;


	Array1[0].Reset();

	Array2[0] = TSharedPtr<FAggregator>( new FAggregator(*Array2[0].Get()) );


	check( Array2[0].IsValid() );
}

bool GameplayEffectsTest_InstantDamage(UWorld *World, FAutomationTestBase * Test)
{
	const float StartHealth = 100.f;
	const float DamageValue = -5.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = SourceActor->AttributeComponent;
	UAttributeComponent * DestComponent = DestActor->AttributeComponent;
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	{
		SKILL_LOG_SCOPE(TEXT("Apply InstantDamage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BaseDmgEffect"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;

		SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);
		
		Test->TestTrue(TEXT("Basic Instant Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == (StartHealth + DamageValue)));
		SKILL_LOG(Log, TEXT("Final Health: %.2f"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health);
	}

	return true;
}

bool GameplayEffectsTest_InstantDamage_Buffed(UWorld *World, FAutomationTestBase * Test)
{
	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float BonusDamageMultiplier = 2.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = SourceActor->AttributeComponent;
	UAttributeComponent * DestComponent = DestActor->AttributeComponent;
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Setup a GE to modify OutgoingGEs
	{
		SKILL_LOG_SCOPE(TEXT("Apply DamageBuff"))
		
		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("DamageBuff"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(BonusDamageMultiplier);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.SetValue(UGameplayEffect::INFINITE_DURATION);

		// Apply to self
		SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, SourceComponent, 1.f);
	}

	// Apply Damage
	{
		SKILL_LOG_SCOPE(TEXT("Apply InstantDamage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;

		// Apply to target
		SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * BonusDamageMultiplier));

		Test->TestTrue(TEXT("Buff Instant Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("Final Health: %.2f"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health);
	}

	return true;
}

bool GameplayEffectsTest_TemporaryDamage(UWorld *World, FAutomationTestBase * Test)
{
	/**
	 *	This test applies a temporary -10 Health GE then removes it to show Health goes back to start
	 */

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = SourceActor->AttributeComponent;
	UAttributeComponent * DestComponent = DestActor->AttributeComponent;
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle AppliedHandle;
	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		AppliedHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + DamageValue);

		Test->TestTrue(TEXT("INFINITE_DURATION Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Damage Applied: Health: %.2f"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health);
	}

	// Now remove the GameplayEffect we just added and confirm Health goes back to starting value
	{
		bool RemovedEffect = DestComponent->RemoveActiveGameplayEffect(AppliedHandle);
		float ExpectedValue = StartHealth;

		Test->TestTrue(TEXT("INFINITE_DURATION Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Removal. Health: %.2f. RemovedEffecte: %d"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health, RemovedEffect);
	}

	return true;
}

bool GameplayEffectsTest_TemporaryDamageBuffed(UWorld *World, FAutomationTestBase * Test)
{
	/**
	 *	This test applies a temporary -10 Health GE then buffs it with an executed (ActiveGE) GE, then removes it to show Health goes back to initial value
	 */

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = SourceActor->AttributeComponent;
	UAttributeComponent * DestComponent = DestActor->AttributeComponent;
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle AppliedHandle;
	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		AppliedHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + DamageValue);

		Test->TestTrue(TEXT("INFINITE_DURATION Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Damage Applied: Health: %.2f"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health);

		Test->TestTrue(TEXT("Number of GameplayEffects=1"), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	// Now Buff the GameplayEffect we just added and confirm the health removal is increased 2x
	{
		SKILL_LOG_SCOPE(TEXT("Buff Permanent Damage"))

		UGameplayEffect * BuffDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffDamage"))));
		BuffDmgEffect->Modifiers.SetNum(1);
		BuffDmgEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffDmgEffect->Modifiers[0].ModifierType = EGameplayMod::ActiveGE;
		BuffDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BuffDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;

		// Apply to target
		SourceComponent->ApplyGameplayEffectSpecToTarget(BuffDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * DamageBuffMultiplier));

		Test->TestTrue(TEXT("INFINITE_DURATION Buffed Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Damage Applied: Health: %.2f"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health);

		// Confirm still only 1 active GE (since this was instant application
		Test->TestTrue(TEXT("Number of GameplayEffects=1"), DestComponent->GetNumActiveGameplayEffect() == 1);
	}


	// Now remove the GameplayEffect we just added and confirm Health goes back to starting value
	{
		bool RemovedEffect = DestComponent->RemoveActiveGameplayEffect(AppliedHandle);
		float ExpectedValue = StartHealth;

		Test->TestTrue(TEXT("INFINITE_DURATION Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Removal. Health: %.2f. RemovedEffecte: %d"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health, RemovedEffect);

		// Confirm no more GEs
		Test->TestTrue(TEXT("Number of GameplayEffects=0"), DestComponent->GetNumActiveGameplayEffect() == 0);
	}

	return true;
}

bool GameplayEffectsTest_TemporaryDamageTemporaryBuff(UWorld *World, FAutomationTestBase * Test)
{
	/**
	 *	This test applies a temporary -10 Health GE then applies a temporary buff to the health modification. It then removes the buff, then the damage, and checks the health values return to what is expected
	 */

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = SourceActor->AttributeComponent;
	UAttributeComponent * DestComponent = DestActor->AttributeComponent;
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle AppliedHandle;
	FActiveGameplayEffectHandle AppliedHandleBuff;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		AppliedHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + DamageValue);

		Test->TestTrue(TEXT("GameplayEffectsTest_TemporaryDamageTemporaryBuff INFINITE_DURATION Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Damage Applied: Health: %.2f"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health);

		Test->TestTrue(TEXT("GameplayEffectsTest_TemporaryDamageTemporaryBuff Number of GameplayEffects=1"), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	// Now Buff the GameplayEffect we just added and confirm the health removal is increased 2x
	{
		SKILL_LOG_SCOPE(TEXT("Buff Permanent Damage"))

		UGameplayEffect * BuffDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffDamage"))));
		BuffDmgEffect->Modifiers.SetNum(1);
		BuffDmgEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffDmgEffect->Modifiers[0].ModifierType = EGameplayMod::ActiveGE;
		BuffDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BuffDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffDmgEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysLink;		// Force this to link, so that when we remove it it will go away to any modifier it was applied to

		// Apply to target
		AppliedHandleBuff = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * DamageBuffMultiplier));

		Test->TestTrue(TEXT("GameplayEffectsTest_TemporaryDamageTemporaryBuff INFINITE_DURATION Buffed Damage Applied"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Damage Applied: Health: %.2f"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health);

		// Confirm there are 2 GEs
		Test->TestTrue(TEXT("GameplayEffectsTest_TemporaryDamageTemporaryBuff Number of GameplayEffects=1"), DestComponent->GetNumActiveGameplayEffect() == 2);
	}

	// Print out the whole enchillada
	{
		// DestComponent->PrintAllGameplayEffects();
	}

	// Remove the buff GE
	{
		bool RemovedEffect = DestComponent->RemoveActiveGameplayEffect(AppliedHandleBuff);
		float ExpectedValue = StartHealth + DamageValue;

		Test->TestTrue(TEXT("GameplayEffectsTest_TemporaryDamageTemporaryBuff INFINITE_DURATION Damage Buff Removed"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Buff Removal. Health: %.2f. RemovedEffecte: %d"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health, RemovedEffect);

		// Confirm 1 more GEs
		Test->TestTrue(TEXT("Number of GameplayEffects=1"), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	// Remove the damage GE
	{
		bool RemovedEffect = DestComponent->RemoveActiveGameplayEffect(AppliedHandle);
		float ExpectedValue = StartHealth;

		Test->TestTrue(TEXT("GameplayEffectsTest_TemporaryDamageTemporaryBuff INFINITE_DURATION Damage Removed"), (DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health == ExpectedValue));
		SKILL_LOG(Log, TEXT("After Removal. Health: %.2f. RemovedEffecte: %d"), DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health, RemovedEffect);

		// Confirm no more GEs
		Test->TestTrue(TEXT("GameplayEffectsTest_TemporaryDamageTemporaryBuff Number of GameplayEffects=0"), DestComponent->GetNumActiveGameplayEffect() == 0);
	}

	return true;
}

bool GameplayEffectsTest_LinkedBuffDestroy(UWorld *World, FAutomationTestBase * Test)
{
	/**
	 *	Apply a perm health reduction that is buffed by an outgoing GE buff. Then destroy the buff and see what happens to the perm applied Ge.
	 */

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = SourceActor->AttributeComponent;
	UAttributeComponent * DestComponent = DestActor->AttributeComponent;
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle AppliedHandle;
	FActiveGameplayEffectHandle AppliedHandleBuff;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage Buff"))

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffOutgoingDamage"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysLink;		// Always link so that when this is destroyed, health return to normal

		// Apply to target
		AppliedHandleBuff = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);

		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 1", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent (infinite duration) Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		AppliedHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * DamageBuffMultiplier));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive GameplayEffects GameplayEffects %d == 1", DestComponent->GetNumActiveGameplayEffect() ), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	// Remove the buff GE
	{
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(AppliedHandleBuff);
		float ExpectedValue = StartHealth + DamageValue;
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		// Confirm we regained healtht
		Test->TestTrue(SKILL_TEST_TEXT("After Buff Removal. ActualValue: %.2f. ExpectedValue: %.2f. RemovedEffecte: %d", ActualValue, ExpectedValue, RemovedEffect), (ActualValue  == ExpectedValue));

		// Confirm number of GEs
		Test->TestTrue(SKILL_TEST_TEXT("Dest Number of GameplayEffects=%d", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 0);
		Test->TestTrue(SKILL_TEST_TEXT("Src Number of GameplayEffects=%d", DestComponent->GetNumActiveGameplayEffect()), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	// DestComponent->PrintAllGameplayEffects();

	return true;
}

bool GameplayEffectsTest_SnapshotBuffDestroy(UWorld *World, FAutomationTestBase * Test)
{
	/**
	*	Apply a perm health reduction that is buffed by an outgoing GE buff. Then destroy the buff and see what happens to the perm applied Ge.
	*/

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle AppliedHandle;
	FActiveGameplayEffectHandle AppliedHandleBuff;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage Buff"))

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffOutgoingDamage"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysSnapshot;		// Always snapshot (though the default for oitgoing should already be snapshot - but this could change per project at some point)

		// Apply to target
		AppliedHandleBuff = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);

		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 1", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent (infinite duration) Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		AppliedHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * DamageBuffMultiplier));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive GameplayEffects GameplayEffects %d == 1", DestComponent->GetNumActiveGameplayEffect()), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	// Remove the buff GE
	{
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(AppliedHandleBuff);
		
		// Check health again - it shouldn't have changed!
		float ExpectedValue = (StartHealth + (DamageValue * DamageBuffMultiplier));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		// Confirm we regained healtht
		Test->TestTrue(SKILL_TEST_TEXT("After Buff Removal. ActualValue: %.2f. ExpectedValue: %.2f. RemovedEffecte: %d", ActualValue, ExpectedValue, RemovedEffect), (ActualValue == ExpectedValue));

		// Confirm number of GEs
		Test->TestTrue(SKILL_TEST_TEXT("Dest Number of GameplayEffects=%d", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 0);
		Test->TestTrue(SKILL_TEST_TEXT("Src Number of GameplayEffects=%d", DestComponent->GetNumActiveGameplayEffect()), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	// DestComponent->PrintAllGameplayEffects();

	return true;
}

bool GameplayEffectsTest_DurationBuff(UWorld *World, FAutomationTestBase * Test)
{
	/**
	*	Apply a perm health reduction that is buffed by an outgoing GE buff. Then destroy the buff and see what happens to the perm applied Ge.
	*/

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float BaseDuration = 30.f;
	const float DurationMod = -10.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply Damage with 30 duration

	FActiveGameplayEffectHandle AppliedDamageHandle;
	FActiveGameplayEffectHandle AppliedDurationHandle;
	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage mod that lasts 10 seconds"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.SetValue(BaseDuration);

		// Apply to target
		AppliedDamageHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedHealthValue = (StartHealth + (DamageValue));
		float ActualHealthValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedDuration = BaseDuration;
		float ActualDuration = DestComponent->GetGameplayEffectDuration(AppliedDamageHandle);
		
		Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. %.2f == %.2f", ActualHealthValue, ExpectedHealthValue), ActualHealthValue == ExpectedHealthValue);
		
		Test->TestTrue(SKILL_TEST_TEXT("Duration of GameplayEffect. %.2f == %.2f", ActualDuration, ExpectedDuration), ActualDuration == ExpectedDuration);
	}

	// Modify the duration of the effect
	{
		UGameplayEffect * DurationEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Duration Mod"))));
		DurationEffect->Modifiers.SetNum(1);
		DurationEffect->Modifiers[0].Magnitude.SetValue(DurationMod);
		DurationEffect->Modifiers[0].ModifierType = EGameplayMod::ActiveGE;
		DurationEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		DurationEffect->Modifiers[0].EffectType = EGameplayModEffect::Duration;
		DurationEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		DurationEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		DurationEffect->Duration.SetValue(UGameplayEffect::INFINITE_DURATION);

		// Apply to target
		AppliedDurationHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(DurationEffect, DestComponent, 1.f);

		float ExpectedDuration = BaseDuration + DurationMod;
		float ActualDuration = DestComponent->GetGameplayEffectDuration(AppliedDamageHandle);

		// Confirm that our duration changed
		Test->TestTrue(SKILL_TEST_TEXT("Duration of GameplayEffect PostMod. %.2f == %.2f", ActualDuration, ExpectedDuration), ActualDuration == ExpectedDuration);
	}

	// Remove the duration effect and see if the duration goes back to the original duration
	{
		bool RemovedEffect = DestComponent->RemoveActiveGameplayEffect(AppliedDurationHandle);
		
		float ExpectedDuration = BaseDuration;
		float ActualDuration = DestComponent->GetGameplayEffectDuration(AppliedDamageHandle);
		
		Test->TestTrue(SKILL_TEST_TEXT("Duration of GameplayEffect Post Mod Remove. %.2f == %.2f", ActualDuration, ExpectedDuration), ActualDuration == ExpectedDuration);
	}

	//DestComponent->PrintAllGameplayEffects();
	return true;
}

bool GameplayEffectsTest_DamageBuffBuff_Basic(UWorld *World, FAutomationTestBase * Test)
{
	/**
	*	Buff a Damage Buff, then apply damage
	*/

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;			// Damage is buff and multiplied by 2
	const float DamageBuffMultiplierBonus = 1.f;	// The above multiplier receives a +1 bonus (we expect a final multiplier of 3)

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle BuffBuffHandle;
	FActiveGameplayEffectHandle BuffHandle;
	FActiveGameplayEffectHandle DamageHandle;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Buff Buff"))

		// Here we are choosing to do this by adding an perm IncomingGE buff first. There are other ways to do this.

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffBuff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplierBonus);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::IncomingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysLink;

		// Apply to target
		BuffBuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);

		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 1", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage Buff"))

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Buff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysSnapshot;

		// Apply to target
		BuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);
		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 2", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 2);

		// Check that the buff was buffed
		float ExpectedBuffMagnitude = DamageBuffMultiplier + DamageBuffMultiplierBonus;
		float ActualBuffMagnitude = SourceComponent->GetGameplayEffectMagnitude(BuffHandle, FGameplayAttribute(HealthProperty));

		Test->TestTrue(SKILL_TEST_TEXT("Buff Applied. Check Magnitude. ActualValue: %.2f. ExpectedValue: %.2f.", ActualBuffMagnitude, ExpectedBuffMagnitude), (ActualBuffMagnitude == ExpectedBuffMagnitude));
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent (infinite duration) Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		DamageHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive Dest GameplayEffects %d == 1", DestComponent->GetNumActiveGameplayEffect()), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		// Clear DependantsUpdates stat
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the original buff buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffBuffHandle);

		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated exactly 1 dependant by removing the BuffBuff (it should have forced the Buff to update - but not the applied Damage GE)
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 1", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 1));
#endif
	}

	{
		SKILL_LOG_SCOPE(TEXT("Remove Buff"));
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffHandle);

		// No change to health since we applied a snapshot of the buff to the damage GE
		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated 0 dependants
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 0", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 0));
#endif
	}

	// DestComponent->PrintAllGameplayEffects();

	return true;
}



bool GameplayEffectsTest_DamageBuffBuff_FullLink(UWorld *World, FAutomationTestBase * Test)
{
	/**
	*	Buff a Damage Buff, then apply damage
	*/

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;			// Damage is buff and multiplied by 2
	const float DamageBuffMultiplierBonus = 1.f;	// The above multiplier receives a +1 bonus (we expect a final multiplier of 3)

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle BuffBuffHandle;
	FActiveGameplayEffectHandle BuffHandle;
	FActiveGameplayEffectHandle DamageHandle;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Buff Buff"))

		// Here we are choosing to do this by adding an perm IncomingGE buff first. There are other ways to do this.

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffBuff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplierBonus);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::IncomingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysLink;

		// Apply to target
		BuffBuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);

		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 1", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage Buff"))

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Buff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysLink;

		// Apply to target
		BuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);
		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 2", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 2);

		// Check that the buff was buffed
		float ExpectedBuffMagnitude = DamageBuffMultiplier + DamageBuffMultiplierBonus;
		float ActualBuffMagnitude = SourceComponent->GetGameplayEffectMagnitude(BuffHandle, FGameplayAttribute(HealthProperty));

		Test->TestTrue(SKILL_TEST_TEXT("Buff Applied. Check Magnitude. ActualValue: %.2f. ExpectedValue: %.2f.", ActualBuffMagnitude, ExpectedBuffMagnitude), (ActualBuffMagnitude == ExpectedBuffMagnitude));
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent (infinite duration) Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		DamageHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive Dest GameplayEffects %d == 1", DestComponent->GetNumActiveGameplayEffect()), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		// Clear DependantsUpdates stat
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the original buff buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffBuffHandle);

		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier)));

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated exactly 3 dependant by removing the BuffBuff (it should have forced the Buff, the applied damage GE, and the attribute aggregator to update)
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 2", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 3));
#endif
	}

	{
		SKILL_LOG_SCOPE(TEXT("Remove Buff"));
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffHandle);

		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = StartHealth + DamageValue;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated 2 dependants - the damage GE and the attribute aggregator
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 2", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 2));
#endif
	}

	// DestComponent->PrintAllGameplayEffects();

	return true;
}

bool GameplayEffectsTest_DamageBuffBuff_FullSnapshot(UWorld *World, FAutomationTestBase * Test)
{
	/**
	*	Buff a Damage Buff, then apply damage
	*/

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;			// Damage is buff and multiplied by 2
	const float DamageBuffMultiplierBonus = 1.f;	// The above multiplier receives a +1 bonus (we expect a final multiplier of 3)

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle BuffBuffHandle;
	FActiveGameplayEffectHandle BuffHandle;
	FActiveGameplayEffectHandle DamageHandle;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Buff Buff"))

		// Here we are choosing to do this by adding an perm IncomingGE buff first. There are other ways to do this.

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffBuff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplierBonus);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::IncomingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysSnapshot;

		// Apply to target
		BuffBuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);

		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 1", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage Buff"))

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Buff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysSnapshot;

		// Apply to target
		BuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);
		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 2", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 2);

		// Check that the buff was buffed
		float ExpectedBuffMagnitude = DamageBuffMultiplier + DamageBuffMultiplierBonus;
		float ActualBuffMagnitude = SourceComponent->GetGameplayEffectMagnitude(BuffHandle, FGameplayAttribute(HealthProperty));

		Test->TestTrue(SKILL_TEST_TEXT("Buff Applied. Check Magnitude. ActualValue: %.2f. ExpectedValue: %.2f.", ActualBuffMagnitude, ExpectedBuffMagnitude), (ActualBuffMagnitude == ExpectedBuffMagnitude));
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent (infinite duration) Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		DamageHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive Dest GameplayEffects %d == 1", DestComponent->GetNumActiveGameplayEffect()), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		// Clear DependantsUpdates stat
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the original buff buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffBuffHandle);

		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated 0 dependants - since everything was applied via snapshot, no dependants should be updated
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 2", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 0));
#endif
	}

	{
		SKILL_LOG_SCOPE(TEXT("Remove Buff"));
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffHandle);

		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated 0 dependants
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 0", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 0));
#endif
	}

	// DestComponent->PrintAllGameplayEffects();

	return true;
}

bool GameplayEffectsTest_DamageBuffBuff_SnapshotLink(UWorld *World, FAutomationTestBase * Test)
{
	/**
	*	Buff a Damage Buff, then apply damage
	*/

	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	const float DamageBuffMultiplier = 2.f;			// Damage is buff and multiplied by 2
	const float DamageBuffMultiplierBonus = 1.f;	// The above multiplier receives a +1 bonus (we expect a final multiplier of 3)

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	// Apply "Damage" but set to INFINITE_DURATION
	// (An odd example for damage, but would make sense for something like run speed, etc)
	FActiveGameplayEffectHandle BuffBuffHandle;
	FActiveGameplayEffectHandle BuffHandle;
	FActiveGameplayEffectHandle DamageHandle;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Buff Buff"))

		// Here we are choosing to do this by adding an perm IncomingGE buff first. There are other ways to do this.

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BuffBuff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplierBonus);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::IncomingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysSnapshot;

		// Apply to target
		BuffBuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);

		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 1", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage Buff"))

		UGameplayEffect * BuffEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Buff"))));
		BuffEffect->Modifiers.SetNum(1);
		BuffEffect->Modifiers[0].Magnitude.SetValue(DamageBuffMultiplier);
		BuffEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		BuffEffect->Modifiers[0].ModifierOp = EGameplayModOp::Multiplicitive;
		BuffEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BuffEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		BuffEffect->CopyPolicy = EGameplayEffectCopyPolicy::AlwaysLink;

		// Apply to target
		BuffHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BuffEffect, SourceComponent, 1.f);
		Test->TestTrue(SKILL_TEST_TEXT("Number of Source GameplayEffect: %d == 2", SourceComponent->GetNumActiveGameplayEffect()), SourceComponent->GetNumActiveGameplayEffect() == 2);

		// Check that the buff was buffed
		float ExpectedBuffMagnitude = DamageBuffMultiplier + DamageBuffMultiplierBonus;
		float ActualBuffMagnitude = SourceComponent->GetGameplayEffectMagnitude(BuffHandle, FGameplayAttribute(HealthProperty));

		Test->TestTrue(SKILL_TEST_TEXT("Buff Applied. Check Magnitude. ActualValue: %.2f. ExpectedValue: %.2f.", ActualBuffMagnitude, ExpectedBuffMagnitude), (ActualBuffMagnitude == ExpectedBuffMagnitude));
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Permanent (infinite duration) Damage"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("Damage"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;

		// Apply to target
		DamageHandle = SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive Dest GameplayEffects %d == 1", DestComponent->GetNumActiveGameplayEffect()), DestComponent->GetNumActiveGameplayEffect() == 1);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Remove Buff Buff"));
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the original buff buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffBuffHandle);

		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = (StartHealth + (DamageValue * (DamageBuffMultiplier + DamageBuffMultiplierBonus)));

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated 0 dependants - since everything was applied via snapshot, no dependants should be updated
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 2", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 0));
#endif
	}

	{
		SKILL_LOG_SCOPE(TEXT("Remove Buff"));
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		FAggregator::AllocationStats.DependantsUpdated = 0;
#endif
		// Remove the buff
		bool RemovedEffect = SourceComponent->RemoveActiveGameplayEffect(BuffHandle);

		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = StartHealth + DamageValue;

		Test->TestTrue(SKILL_TEST_TEXT("Damaged Applied. ActualValue: %.2f. ExpectedValue: %.2f.", ActualValue, ExpectedValue), (ActualValue == ExpectedValue));

		// Check that we updated 2 dependants - the damage GE and the attribute aggregator
#if SKILL_SYSTEM_AGGREGATOR_DEBUG
		Test->TestTrue(SKILL_TEST_TEXT("DependantsUpdated %d == 2", FAggregator::AllocationStats.DependantsUpdated), (FAggregator::AllocationStats.DependantsUpdated == 2));
#endif
	}

	// DestComponent->PrintAllGameplayEffects();

	return true;
}

bool TimerTest(UWorld *World, FAutomationTestBase * Test)
{
	const float StartHealth = 100.f;
	const float DamageValue = -5.f;

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();
	SourceActor->SetActorTickEnabled(true);
	DestActor->SetActorTickEnabled(true);

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->RegisterComponentWithWorld(World);
	DestComponent->RegisterComponentWithWorld(World);

	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	//SourceComponent->TEMP_TimerTest();

	World->Tick(ELevelTick::LEVELTICK_All, 3.f);
	World->Tick(ELevelTick::LEVELTICK_All, 3.f);
	World->Tick(ELevelTick::LEVELTICK_All, 3.f);

	return true;
}

bool GameplayEffectsTest_DurationDamage(UWorld *World, FAutomationTestBase * Test)
{
	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	float Duration = 5.f;
	float StartTime = World->GetTimeSeconds();

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->RegisterComponentWithWorld(World);
	DestComponent->RegisterComponentWithWorld(World);

	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Dot"));

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BaseDmgEffect"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = Duration;
		BaseDmgEffect->Period.Value = UGameplayEffect::NO_PERIOD;

		SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 1.f);

		// The effect should instantly execute one time without ticking (for now at least)
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = StartHealth + (DamageValue);

		Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
	}

	// Tick until the effect should expire
	for (int32 i = 0; i < 10; ++i)
	{
		GameplayTest_TickWorld(World, 1.f);
		if (World->GetTimeSeconds() > StartTime + Duration)
		{
			break;
		}

		// The effect should instantly execute one time without ticking (for now at least)
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = StartHealth + (DamageValue);

		Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Duration (left) %.2f. Actual: %.2f == Exected: %.2f", Duration, ActualValue, ExpectedValue), ActualValue == ExpectedValue);
	}

	// Ensure the effect expired
	{
		// The effect should instantly execute one time without ticking (for now at least)
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = StartHealth;

		Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Duration (left) %.2f. Actual: %.2f == Exected: %.2f", Duration, ActualValue, ExpectedValue), ActualValue == ExpectedValue);

		int32 NumEffects = DestComponent->GetNumActiveGameplayEffect();
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive Dest GameplayEffects %d == 0", NumEffects), NumEffects == 0);
	}

	return true;
}

bool GameplayEffectsTest_PeriodicDamage(UWorld *World, FAutomationTestBase * Test)
{
	const float StartHealth = 100.f;
	const float DamageValue = -5.f;
	float Duration = 5.f;
	float StartTime = World->GetTimeSeconds();

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->RegisterComponentWithWorld(World);
	DestComponent->RegisterComponentWithWorld(World);

	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	float ApplyCount = 0;
	{
		SKILL_LOG_SCOPE(TEXT("Apply Dot"))

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BaseDmgEffect"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = Duration;
		BaseDmgEffect->Period.Value = 1.f; // Apply every 1 second
		BaseDmgEffect->GameplayCues.Add( FGameplayEffectCue(FName(TEXT("GameplayCue.Burning")), 1.f, 10.f) );

		SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 5.f);
		ApplyCount++;

		// The effect should instantly execute one time without ticking (for now at least)
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = StartHealth + (DamageValue * ApplyCount);

		Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue );
	}
		
	for (int32 i=0; i < 10; ++i)
	{
		GameplayTest_TickWorld(World, 1.f);
		if (World->GetTimeSeconds() <= StartTime + Duration + KINDA_SMALL_NUMBER)
		{
			// We should have applied as long as there was still some duration left
			ApplyCount++;
		}

		// The effect should instantly execute one time without ticking (for now at least)
		float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
		float ExpectedValue = StartHealth + (DamageValue * ApplyCount);

		Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Duration (left) %.2f. Actual: %.2f == Exected: %.2f", Duration, ActualValue, ExpectedValue), ActualValue == ExpectedValue);
	}

	// Ensure the effect expired
	{
		int32 NumEffects = DestComponent->GetNumActiveGameplayEffect();
		Test->TestTrue(SKILL_TEST_TEXT("NumberOfActive Dest GameplayEffects %d == 0", NumEffects), NumEffects == 0);
	}

	return true;
}

bool GameplayEffectsTest_LifestealExtension(UWorld *World, FAutomationTestBase * Test)
{
	const float StartHealth = 100.f;
	const float DamageValue = -10.f;
	const float LifestealPCT = 0.20f;
	float StartTime = World->GetTimeSeconds();

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));
	UProperty *DamageProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Damage));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->RegisterComponentWithWorld(World);
	DestComponent->RegisterComponentWithWorld(World);

	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	{
		SKILL_LOG_SCOPE(TEXT("Apply Lifesteal"));

		UGameplayEffect * LifestealEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("LifestealPassive"))));
		LifestealEffect->Modifiers.SetNum(1);
		LifestealEffect->Modifiers[0].Magnitude.SetValue(LifestealPCT);
		LifestealEffect->Modifiers[0].ModifierType = EGameplayMod::OutgoingGE;
		LifestealEffect->Modifiers[0].ModifierOp = EGameplayModOp::Callback;
		LifestealEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		LifestealEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Lifesteal")));
		LifestealEffect->Modifiers[0].Callbacks.ExtensionClasses.Add( UGameplayEffectExtension_LifestealTest::StaticClass() );
		LifestealEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		LifestealEffect->Period.Value = UGameplayEffect::NO_PERIOD;

		SourceComponent->ApplyGameplayEffectToSelf(LifestealEffect, 1.f, SourceActor);
	}
	
	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage"));

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BaseDmgEffect"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;
		BaseDmgEffect->Period.Value = UGameplayEffect::NO_PERIOD;

		SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 5.f);

		// The effect should instantly execute one time without ticking (for now at least)
		{
			float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
			float ExpectedValue = StartHealth + (DamageValue);

			Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
		}

		// Test that the source received extra health back
		{
			float ActualValue = SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
			float ExpectedValue = StartHealth + (-DamageValue * LifestealPCT);

			Test->TestTrue(SKILL_TEST_TEXT("Health after lifesteal. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
		}		

	}

	return true;
}

bool GameplayEffectsTest_ShieldlExtension(UWorld *World, FAutomationTestBase * Test)
{
	const float StartHealth = 100.f;
	const float DamageValue = -10.f;
	const float ShieldAmount = 20.0f;
	float StartTime = World->GetTimeSeconds();

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));
	UProperty *DamageProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Damage));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->RegisterComponentWithWorld(World);
	DestComponent->RegisterComponentWithWorld(World);

	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	FActiveGameplayEffectHandle AppliedHandle;
	{
		SKILL_LOG_SCOPE(TEXT("Apply Shield"));

		UGameplayEffect * ShieldEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("ShieldPassive"))));
		ShieldEffect->Modifiers.SetNum(1);
		ShieldEffect->Modifiers[0].Magnitude.SetValue(ShieldAmount);
		ShieldEffect->Modifiers[0].ModifierType = EGameplayMod::IncomingGE;
		ShieldEffect->Modifiers[0].ModifierOp = EGameplayModOp::Callback;
		ShieldEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		ShieldEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Shield")));
		ShieldEffect->Modifiers[0].Callbacks.ExtensionClasses.Add(UGameplayEffectExtension_ShieldTest::StaticClass());
		ShieldEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		ShieldEffect->Period.Value = UGameplayEffect::NO_PERIOD;

		AppliedHandle = DestComponent->ApplyGameplayEffectToSelf(ShieldEffect, 1.f, DestActor);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage"));

		UGameplayEffect * BaseDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("BaseDmgEffect"))));
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		BaseDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;
		BaseDmgEffect->Period.Value = UGameplayEffect::NO_PERIOD;

		
		// Apply 1
		{
			SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 5.f);

			// Health should be the same
			{
				float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
				float ExpectedValue = StartHealth;
				Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}

			// Shield still up but weakened
			{
				float ActualValue = DestComponent->GetGameplayEffectMagnitude(AppliedHandle, FGameplayAttribute(HealthProperty));
				float ExpectedValue = ShieldAmount + DamageValue;
				Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}
		}

		// Apply 2
		{
			SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 5.f);

			// Health should be the same
			{
				float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
				float ExpectedValue = StartHealth;
				Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}

			// Shield should be done now (it absorbed the damage and then removed itself)
			{
				bool Removed = DestComponent->IsGameplayEffectActive(AppliedHandle);
				Test->TestTrue(SKILL_TEST_TEXT("Shield removed (Expected: 0 Actual: %d", Removed), !Removed);
			}
		}

		// Apply 3
		{
			// Now we lose health
			SourceComponent->ApplyGameplayEffectSpecToTarget(BaseDmgEffect, DestComponent, 5.f);

			// Now we should have lost some health
			float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
			float ExpectedValue = StartHealth + DamageValue;;
			Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);

			// For funsies, confirm shield is still definitely not there
			{
				bool Removed = DestComponent->IsGameplayEffectActive(AppliedHandle);
				Test->TestTrue(SKILL_TEST_TEXT("Shield removed (Expected: 0 Actual: %d", Removed), !Removed);
			}
		}
	}

	return true;
}

bool GameplayEffectsTest_ShieldlExtensionMultiple(UWorld *World, FAutomationTestBase * Test)
{
	// This applies 2 instances of the shield and confirms that only 1 will absorb damage at at time

	const float StartHealth = 100.f;
	const float DamageValueSmall = -10.f;
	const float DamageValueLarge = -20.f;
	const float ShieldAmount = 20.0f;
	float StartTime = World->GetTimeSeconds();

	ASkillSystemTestPawn *SourceActor = World->SpawnActor<ASkillSystemTestPawn>();
	ASkillSystemTestPawn *DestActor = World->SpawnActor<ASkillSystemTestPawn>();

	UProperty *HealthProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Health));
	UProperty *DamageProperty = FindFieldChecked<UProperty>(USkillSystemTestAttributeSet::StaticClass(), GET_MEMBER_NAME_CHECKED(USkillSystemTestAttributeSet, Damage));

	UAttributeComponent * SourceComponent = Cast<UAttributeComponent>(SourceActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	UAttributeComponent * DestComponent = Cast<UAttributeComponent>(DestActor->CreateComponentFromTemplate(UAttributeComponent::StaticClass()->GetDefaultObject<UAttributeComponent>(), FString(TEXT("AttributeComponent"))));
	SourceComponent->RegisterComponentWithWorld(World);
	DestComponent->RegisterComponentWithWorld(World);

	SourceComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;
	DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health = StartHealth;

	FActiveGameplayEffectHandle AppliedHandle_1;
	FActiveGameplayEffectHandle AppliedHandle_2;
	{
		SKILL_LOG_SCOPE(TEXT("Apply Shields"));

		UGameplayEffect * ShieldEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("ShieldPassive"))));
		ShieldEffect->Modifiers.SetNum(1);
		ShieldEffect->Modifiers[0].Magnitude.SetValue(ShieldAmount);
		ShieldEffect->Modifiers[0].ModifierType = EGameplayMod::IncomingGE;
		ShieldEffect->Modifiers[0].ModifierOp = EGameplayModOp::Callback;
		ShieldEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		ShieldEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Shield")));
		ShieldEffect->Modifiers[0].Callbacks.ExtensionClasses.Add(UGameplayEffectExtension_ShieldTest::StaticClass());
		ShieldEffect->Duration.Value = UGameplayEffect::INFINITE_DURATION;
		ShieldEffect->Period.Value = UGameplayEffect::NO_PERIOD;

		AppliedHandle_1 = DestComponent->ApplyGameplayEffectToSelf(ShieldEffect, 1.f, DestActor);
		AppliedHandle_2 = DestComponent->ApplyGameplayEffectToSelf(ShieldEffect, 1.f, DestActor);
	}

	{
		SKILL_LOG_SCOPE(TEXT("Apply Damage"));

		UGameplayEffect * SmallDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("SmallDmgEffect"))));
		SmallDmgEffect->Modifiers.SetNum(1);
		SmallDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValueSmall);
		SmallDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		SmallDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		SmallDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		SmallDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		SmallDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;
		SmallDmgEffect->Period.Value = UGameplayEffect::NO_PERIOD;

		UGameplayEffect * LargeDmgEffect = Cast<UGameplayEffect>(StaticConstructObject(UGameplayEffect::StaticClass(), GetTransientPackage(), FName(TEXT("LargeDmgEffect"))));
		LargeDmgEffect->Modifiers.SetNum(1);
		LargeDmgEffect->Modifiers[0].Magnitude.SetValue(DamageValueLarge);
		LargeDmgEffect->Modifiers[0].ModifierType = EGameplayMod::Attribute;
		LargeDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		LargeDmgEffect->Modifiers[0].Attribute.SetUProperty(HealthProperty);
		LargeDmgEffect->Modifiers[0].OwnedTags.AddTag(FName(TEXT("Damage.Basic")));
		LargeDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;
		LargeDmgEffect->Period.Value = UGameplayEffect::NO_PERIOD;


		// Apply small damage
		{
			SourceComponent->ApplyGameplayEffectSpecToTarget(SmallDmgEffect, DestComponent, 5.f);

			// Health should be the same
			{
				float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
				float ExpectedValue = StartHealth;
				Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}

			// Shield 1 still up but weakened
			{
				float ActualValue = DestComponent->GetGameplayEffectMagnitude(AppliedHandle_1, FGameplayAttribute(HealthProperty));
				float ExpectedValue = ShieldAmount + DamageValueSmall;
				Test->TestTrue(SKILL_TEST_TEXT("Shield 1. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}

			// Shield 2 untouched
			{
				float ActualValue = DestComponent->GetGameplayEffectMagnitude(AppliedHandle_2, FGameplayAttribute(HealthProperty));
				float ExpectedValue = ShieldAmount;
				Test->TestTrue(SKILL_TEST_TEXT("Shield 1. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}
		}

		// Apply large damage
		{
			SourceComponent->ApplyGameplayEffectSpecToTarget(LargeDmgEffect, DestComponent, 5.f);

			// Health should still be the same
			{
				float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
				float ExpectedValue = StartHealth;
				Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}

			// Shield 1 should be gone
			{
				bool Exists = DestComponent->IsGameplayEffectActive(AppliedHandle_1);
				Test->TestTrue(SKILL_TEST_TEXT("Shield removed (Expected: 0 Actual: %d", Exists), !Exists);
			}

			// Shield 2 should be weakened
			{
				float DamageShield2Took = ShieldAmount + DamageValueSmall + DamageValueLarge;

				float ActualValue = DestComponent->GetGameplayEffectMagnitude(AppliedHandle_2, FGameplayAttribute(HealthProperty));
				float ExpectedValue = ShieldAmount + DamageShield2Took;
				Test->TestTrue(SKILL_TEST_TEXT("Shield 1. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);
			}
		}

		// Apply large damage again
		{
			// Now we lose health
			SourceComponent->ApplyGameplayEffectSpecToTarget(LargeDmgEffect, DestComponent, 5.f);

			float HealthDelta = ShieldAmount + ShieldAmount + DamageValueSmall + DamageValueLarge + DamageValueLarge;

			// Now we should have lost some health
			float ActualValue = DestComponent->GetSet<USkillSystemTestAttributeSet>()->Health;
			float ExpectedValue = StartHealth + HealthDelta;
			Test->TestTrue(SKILL_TEST_TEXT("Damage Applied. Actual: %.2f == Exected: %.2f", ActualValue, ExpectedValue), ActualValue == ExpectedValue);

			// For funsies, confirm shield is still definitely not there
			{
				bool Exists = DestComponent->IsGameplayEffectActive(AppliedHandle_2);
				Test->TestTrue(SKILL_TEST_TEXT("Shield removed (Expected: 0 Actual: %d", Exists), !Exists);
			}
		}
	}

	return true;
}

bool FGameplayEffectsTest::RunTest( const FString& Parameters )
{
	UWorld *World = UWorld::CreateWorld( EWorldType::Game, false );
	FURL URL;
	World->BeginPlay(URL);
	
	GameplayEffectsTest_InstantDamage(World, this);
	GameplayEffectsTest_InstantDamage_Buffed(World, this);
	GameplayEffectsTest_TemporaryDamage(World, this);
	GameplayEffectsTest_TemporaryDamageBuffed(World, this);
	GameplayEffectsTest_TemporaryDamageTemporaryBuff(World, this);
	GameplayEffectsTest_LinkedBuffDestroy(World, this);
	GameplayEffectsTest_SnapshotBuffDestroy(World, this);
	GameplayEffectsTest_DurationBuff(World, this);
	
	GameplayEffectsTest_DamageBuffBuff_Basic(World, this);
	GameplayEffectsTest_DamageBuffBuff_FullLink(World, this);
	GameplayEffectsTest_DamageBuffBuff_FullSnapshot(World, this);
	GameplayEffectsTest_DamageBuffBuff_SnapshotLink(World, this);

	GameplayEffectsTest_DurationDamage(World, this);
	GameplayEffectsTest_PeriodicDamage(World, this);

	GameplayEffectsTest_LifestealExtension(World, this);
	
	GameplayEffectsTest_ShieldlExtension(World, this);
	GameplayEffectsTest_ShieldlExtensionMultiple(World, this);

	World->DestroyWorld(false);
	return true;
}


