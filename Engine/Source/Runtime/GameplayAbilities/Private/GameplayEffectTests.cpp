// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "AbilitySystemPrivatePCH.h"
#include "AbilitySystemTestPawn.h"
#include "AbilitySystemTestAttributeSet.h"
#include "GameplayEffect.h"
#include "AttributeSet.h"
#include "GameplayTagsModule.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension_LifestealTest.h"
#include "GameplayEffectExtension_ShieldTest.h"
#include "GameplayEffectStackingExtension_CappedNumberTest.h"
#include "GameplayEffectStackingExtension_DiminishingReturnsTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayEffectsTest, "AbilitySystem.GameplayEffects", EAutomationTestFlags::ATF_Editor)

#define SKILL_TEST_TEXT( Format, ... ) FString::Printf(TEXT("%s - %d: %s"), TEXT(__FILE__) , __LINE__ , *FString::Printf(TEXT(Format), ##__VA_ARGS__) )

#if WITH_EDITOR

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

static UDataTable* CreateGameplayDataTable()
{
	FString CSV(TEXT(",Tag,CategoryText,"));
	CSV.Append(TEXT("\r\n0,Damage"));
	CSV.Append(TEXT("\r\n1,Damage.Basic"));
	CSV.Append(TEXT("\r\n2,Damage.Type1"));
	CSV.Append(TEXT("\r\n3,Damage.Type2"));
	CSV.Append(TEXT("\r\n4,Damage.Reduce"));
	CSV.Append(TEXT("\r\n5,Damage.Buffable"));
	CSV.Append(TEXT("\r\n6,Damage.Buff"));
	CSV.Append(TEXT("\r\n7,Damage.Physical"));
	CSV.Append(TEXT("\r\n8,Damage.Fire"));
	CSV.Append(TEXT("\r\n9,Damage.Buffed.FireBuff"));
	CSV.Append(TEXT("\r\n10,Damage.Mitigated.Armor"));
	CSV.Append(TEXT("\r\n11,Lifesteal"));
	CSV.Append(TEXT("\r\n12,Shield"));
	CSV.Append(TEXT("\r\n13,Buff"));
	CSV.Append(TEXT("\r\n14,Immune"));
	CSV.Append(TEXT("\r\n15,FireDamage"));
	CSV.Append(TEXT("\r\n16,ShieldAbsorb"));
	CSV.Append(TEXT("\r\n17,Stackable"));
	CSV.Append(TEXT("\r\n18,Stack"));
	CSV.Append(TEXT("\r\n19,Stack.CappedNumber"));
	CSV.Append(TEXT("\r\n20,Stack.DiminishingReturns"));
	CSV.Append(TEXT("\r\n21,Protect.Damage"));
	CSV.Append(TEXT("\r\n22,SpellDmg.Buff"));
	CSV.Append(TEXT("\r\n23,GameplayCue.Burning"));

	UDataTable * DataTable = Cast<UDataTable>(StaticConstructObject(UDataTable::StaticClass(), GetTransientPackage(), FName(TEXT("TempDataTable"))));
	DataTable->RowStruct = FGameplayTagTableRow::StaticStruct();
	DataTable->CreateTableFromCSVString(CSV);

	FGameplayTagTableRow * Row = (FGameplayTagTableRow*)DataTable->RowMap["0"];
	if (Row)
	{
		check(Row->Tag == TEXT("Damage"));
	}
	return DataTable;
}

#define GET_FIELD_CHECKED(Class, Field) FindFieldChecked<UProperty>(Class::StaticClass(), GET_MEMBER_NAME_CHECKED(Class, Field))
#define CONSTRUCT_CLASS(Class, Name) Class * Name = Cast<Class>(StaticConstructObject(Class::StaticClass(), GetTransientPackage(), FName(TEXT(#Name))))

class GameplayEffectsTestSuite
{
public:
	GameplayEffectsTestSuite(UWorld* WorldIn, FAutomationTestBase* TestIn)
	: World(WorldIn)
	, Test(TestIn)
	{
		// run before each test

		const float StartingHealth = 100.f;

		// set up the source actor
		SourceActor = World->SpawnActor<AAbilitySystemTestPawn>();
		SourceComponent = SourceActor->GetAbilitySystemComponent();
		SourceComponent->GetSet<UAbilitySystemTestAttributeSet>()->Health = StartingHealth;

		// set up the destination actor
		DestActor = World->SpawnActor<AAbilitySystemTestPawn>();
		DestComponent = DestActor->GetAbilitySystemComponent();
		DestComponent->GetSet<UAbilitySystemTestAttributeSet>()->Health = StartingHealth;
	}

	~GameplayEffectsTestSuite()
	{
		// run after each test

		// destroy the actors
		if (SourceActor)
		{
			World->EditorDestroyActor(SourceActor, false);
		}
		if (DestActor)
		{
			World->EditorDestroyActor(DestActor, false);
		}
	}

public: // the tests

	void Test_InstantDamage()
	{
		ABILITY_LOG_SCOPE(TEXT("Apply InstantDamage"));

		const float DamageValue = -5.f;
		const float StartingHealth = DestComponent->GetSet<UAbilitySystemTestAttributeSet>()->Health;

		CONSTRUCT_CLASS(UGameplayEffect, BaseDmgEffect);
		BaseDmgEffect->Modifiers.SetNum(1);
		BaseDmgEffect->Modifiers[0].ModifierMagnitude = FScalableFloat(DamageValue);
		BaseDmgEffect->Modifiers[0].ModifierOp = EGameplayModOp::Additive;
		BaseDmgEffect->Modifiers[0].Attribute.SetUProperty(GET_FIELD_CHECKED(UAbilitySystemTestAttributeSet, Health));
		BaseDmgEffect->Duration.Value = UGameplayEffect::INSTANT_APPLICATION;

		SourceComponent->ApplyGameplayEffectToTarget(BaseDmgEffect, DestComponent, 1.f);

		Test->TestEqual(SKILL_TEST_TEXT("Basic Instant Damage Applied"), DestComponent->GetSet<UAbilitySystemTestAttributeSet>()->Health, StartingHealth + DamageValue);
	}

public:

	// used to run tests consistently
	static void RunTest(UWorld* WorldIn, FAutomationTestBase* TestIn, void (GameplayEffectsTestSuite::*TestFunc)())
	{
		uint64 InitialFrameCounter = GFrameCounter;
		{
			GameplayEffectsTestSuite Tester(WorldIn, TestIn);
			(Tester.*TestFunc)();
		}
		GFrameCounter = InitialFrameCounter;
	}

private:
	UWorld* World;
	FAutomationTestBase* Test;

	AAbilitySystemTestPawn* SourceActor;
	UAbilitySystemComponent* SourceComponent;

	AAbilitySystemTestPawn* DestActor;
	UAbilitySystemComponent* DestComponent;
};

#endif //WITH_EDITOR

bool FGameplayEffectsTest::RunTest( const FString& Parameters )
{
#if WITH_EDITOR

	UCurveTable *CurveTable = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetGlobalCurveTable();
	UDataTable *DataTable = IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->GetGlobalAttributeMetaDataTable();

	// setup required GameplayTags
	UDataTable* TagTable = CreateGameplayDataTable();

	IGameplayTagsModule::Get().GetGameplayTagsManager().PopulateTreeFromDataTable(TagTable);

	UWorld *World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext &WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	
	FURL URL;
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();
	
	GameplayEffectsTestSuite::RunTest(World, this, &GameplayEffectsTestSuite::Test_InstantDamage);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->AutomationTestOnly_SetGlobalCurveTable(CurveTable);
	IGameplayAbilitiesModule::Get().GetAbilitySystemGlobals()->AutomationTestOnly_SetGlobalAttributeDataTable(DataTable);

#endif //WITH_EDITOR
	return true;
}


