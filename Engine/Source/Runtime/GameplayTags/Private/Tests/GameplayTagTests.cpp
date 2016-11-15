// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "GameplayTagsModulePrivatePCH.h"

#if WITH_DEV_AUTOMATION_TESTS

#if WITH_EDITOR

#define ADD_TAG_CSV(Name, Count) CSV.Append(FString::Printf(TEXT("\r\n%d,"#Name), Count++));

static UDataTable* CreateGameplayDataTable()
{
	int32 Count = 0;
	FString CSV(TEXT(",Tag,CategoryText,"));
	ADD_TAG_CSV(Effect.Damage, Count);
	ADD_TAG_CSV(Effect.Damage.Basic, Count);
	ADD_TAG_CSV(Effect.Damage.Type1, Count);
	ADD_TAG_CSV(Effect.Damage.Type2, Count);
	ADD_TAG_CSV(Effect.Damage.Reduce, Count);
	ADD_TAG_CSV(Effect.Damage.Buffable, Count);
	ADD_TAG_CSV(Effect.Damage.Buff, Count);
	ADD_TAG_CSV(Effect.Damage.Physical, Count);
	ADD_TAG_CSV(Effect.Damage.Fire, Count);
	ADD_TAG_CSV(Effect.Damage.Buffed.FireBuff, Count);
	ADD_TAG_CSV(Effect.Damage.Mitigated.Armor, Count);
	ADD_TAG_CSV(Effect.Lifesteal, Count);
	ADD_TAG_CSV(Effect.Shield, Count);
	ADD_TAG_CSV(Effect.Buff, Count);
	ADD_TAG_CSV(Effect.Immune, Count);
	ADD_TAG_CSV(Effect.FireDamage, Count);
	ADD_TAG_CSV(Effect.Shield.Absorb, Count);
	ADD_TAG_CSV(Effect.Protect.Damage, Count);
	ADD_TAG_CSV(Stackable, Count);
	ADD_TAG_CSV(Stack.DiminishingReturns, Count);
	ADD_TAG_CSV(GameplayCue.Burning, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.1, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.2, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.3, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.4, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.5, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.6, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.7, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.8, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.9, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.10, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.11, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.12, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.13, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.14, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.15, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.16, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.17, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.18, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.19, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.20, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.21, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.22, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.23, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.24, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.25, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.26, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.27, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.28, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.29, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.30, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.31, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.32, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.33, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.34, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.35, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.36, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.37, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.38, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.39, Count);
	ADD_TAG_CSV(Expensive.Status.Tag.Type.40, Count);

	auto DataTable = NewObject<UDataTable>(GetTransientPackage(), FName(TEXT("TempDataTable")));
	DataTable->RowStruct = FGameplayTagTableRow::StaticStruct();
	DataTable->CreateTableFromCSVString(CSV);

	FGameplayTagTableRow * Row = (FGameplayTagTableRow*)DataTable->RowMap["0"];
	if (Row)
	{
		check(Row->Tag == TEXT("Effect.Damage"));
	}
	return DataTable;
}

static FGameplayTag GetTagForString(const FString& String)
{
	return IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTag(FName(*String));
}

static bool GameplayTagTest_SimpleTest()
{
	FName TagName = FName(TEXT("Stack.DiminishingReturns"));
	FGameplayTag Tag = IGameplayTagsModule::Get().GetGameplayTagsManager().RequestGameplayTag(TagName);
	return Tag.GetTagName() == TagName;
}

static bool GameplayTagTest_TagComparisonTest()
{
	FGameplayTag EffectDamageTag = GetTagForString(TEXT("Effect.Damage"));
	FGameplayTag EffectDamage1Tag = GetTagForString(TEXT("Effect.Damage.Type1"));
	FGameplayTag EffectDamage2Tag = GetTagForString(TEXT("Effect.Damage.Type2"));
	FGameplayTag CueTag = GetTagForString(TEXT("GameplayCue.Burning"));
	
	bool bResult = true;

	bResult &= (EffectDamage1Tag == EffectDamage1Tag);
	bResult &= (EffectDamage1Tag != EffectDamage2Tag);
	bResult &= (EffectDamage1Tag != EffectDamageTag);

	bResult &= (EffectDamage1Tag.Matches(EGameplayTagMatchType::IncludeParentTags, EffectDamageTag, EGameplayTagMatchType::Explicit));
	bResult &= (EffectDamage1Tag.Matches(EGameplayTagMatchType::IncludeParentTags, EffectDamageTag, EGameplayTagMatchType::IncludeParentTags));
	bResult &= (!EffectDamage1Tag.Matches(EGameplayTagMatchType::Explicit, EffectDamageTag, EGameplayTagMatchType::IncludeParentTags));
	bResult &= (!EffectDamage1Tag.Matches(EGameplayTagMatchType::Explicit, EffectDamageTag, EGameplayTagMatchType::Explicit));

	bResult &= (!EffectDamage1Tag.Matches(EGameplayTagMatchType::IncludeParentTags, CueTag, EGameplayTagMatchType::IncludeParentTags));

	bResult &= (EffectDamage1Tag.RequestDirectParent() == EffectDamageTag);

	return bResult;
}

static bool GameplayTagTest_TagContainerTest()
{
	FGameplayTag EffectDamageTag = GetTagForString(TEXT("Effect.Damage"));
	FGameplayTag EffectDamage1Tag = GetTagForString(TEXT("Effect.Damage.Type1"));
	FGameplayTag EffectDamage2Tag = GetTagForString(TEXT("Effect.Damage.Type2"));
	FGameplayTag CueTag = GetTagForString(TEXT("GameplayCue.Burning"));

	bool bResult = true;

	FGameplayTagContainer TagContainer;
	TagContainer.AddTag(EffectDamage1Tag);
	TagContainer.AddTag(CueTag);

	FGameplayTagContainer TagContainer2;
	TagContainer2.AddTag(EffectDamage2Tag);
	TagContainer2.AddTag(CueTag);

	FGameplayTagContainer EmptyContainer;

	bResult &= (TagContainer == TagContainer);
	bResult &= (TagContainer != TagContainer2);

	bResult &= (TagContainer.MatchesAny(TagContainer2, true));
	bResult &= (!TagContainer.MatchesAll(TagContainer2, true));

	bResult &= (TagContainer.MatchesAll(EmptyContainer, true));
	bResult &= (!TagContainer.MatchesAll(EmptyContainer, false));
	bResult &= (!EmptyContainer.MatchesAll(TagContainer, true));
	bResult &= (!EmptyContainer.MatchesAll(TagContainer, false));

	bResult &= (TagContainer.HasTagExplicit(EffectDamage1Tag));
	bResult &= (!TagContainer.HasTagExplicit(EffectDamage2Tag));

	bResult &= (TagContainer.HasTag(EffectDamageTag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit));
	bResult &= (TagContainer.HasTag(EffectDamageTag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::IncludeParentTags));
	bResult &= (!TagContainer.HasTag(EffectDamageTag, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::Explicit));
	bResult &= (!TagContainer.HasTag(EffectDamageTag, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::IncludeParentTags));

	bResult &= (!TagContainer.HasTag(EffectDamage2Tag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::Explicit));
	bResult &= (TagContainer.HasTag(EffectDamage2Tag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::IncludeParentTags));
	bResult &= (!TagContainer.HasTag(EffectDamage2Tag, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::Explicit));
	bResult &= (!TagContainer.HasTag(EffectDamage2Tag, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::IncludeParentTags));

	FGameplayTagContainer FilteredTagContainer = TagContainer.Filter(TagContainer2, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::Explicit);
	bResult &= (FilteredTagContainer.HasTagExplicit(CueTag));
	bResult &= (!FilteredTagContainer.HasTagExplicit(EffectDamage1Tag));

	FilteredTagContainer = TagContainer.Filter(TagContainer2, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::IncludeParentTags);
	bResult &= (FilteredTagContainer.HasTagExplicit(CueTag));
	bResult &= (FilteredTagContainer.HasTagExplicit(EffectDamage1Tag));

	return bResult;
}

static bool GameplayTagTest_PerfTest()
{
	FGameplayTag EffectDamageTag = GetTagForString(TEXT("Effect.Damage"));
	FGameplayTag EffectDamage1Tag = GetTagForString(TEXT("Effect.Damage.Type1"));
	FGameplayTag EffectDamage2Tag = GetTagForString(TEXT("Effect.Damage.Type2"));
	FGameplayTag CueTag = GetTagForString(TEXT("GameplayCue.Burning"));

	FGameplayTagContainer TagContainer;

	bool bResult = true;

	{
		FScopeLogTime LogTimePtr(*FString::Printf(TEXT("1000 container constructions")), nullptr, FScopeLogTime::ScopeLog_Milliseconds);
		for (int32 i = 0; i < 1000; i++)
		{
			TagContainer = FGameplayTagContainer();
			TagContainer.AddTag(EffectDamage1Tag);
			TagContainer.AddTag(EffectDamage2Tag);
			TagContainer.AddTag(CueTag);
			for (int32 j = 1; j <= 40; j++)
			{
				TagContainer.AddTag(GetTagForString(FString::Printf(TEXT("Expensive.Status.Tag.Type.%d"), j)));
			}
		}
	}

	{
		FScopeLogTime LogTimePtr(*FString::Printf(TEXT("1000 container copies")), nullptr, FScopeLogTime::ScopeLog_Milliseconds);
		for (int32 i = 0; i < 1000; i++)
		{
			FGameplayTagContainer TagContainerNew;

			for (auto It = TagContainer.CreateConstIterator(); It; ++It)
			{
				TagContainerNew.AddTag(*It);
			}
		}
	}
	
	FGameplayTagContainer TagContainer2;
	TagContainer2.AddTag(EffectDamage1Tag);
	TagContainer2.AddTag(EffectDamage2Tag);
	TagContainer2.AddTag(CueTag);

	{
		FScopeLogTime LogTimePtr(*FString::Printf(TEXT("10000 explicit tag checks")), nullptr, FScopeLogTime::ScopeLog_Milliseconds);
		for (int32 i = 0; i < 10000; i++)
		{
			bResult &= TagContainer.HasTag(EffectDamage1Tag, EGameplayTagMatchType::Explicit, EGameplayTagMatchType::Explicit);
		}
	}

	{
		FScopeLogTime LogTimePtr(*FString::Printf(TEXT("10000 parent tag checks")), nullptr, FScopeLogTime::ScopeLog_Milliseconds);
		for (int32 i = 0; i < 10000; i++)
		{
			bResult &= TagContainer.HasTag(EffectDamage1Tag, EGameplayTagMatchType::IncludeParentTags, EGameplayTagMatchType::IncludeParentTags);
		}
	}

	{
		FScopeLogTime LogTimePtr(*FString::Printf(TEXT("10000 container checks")), nullptr, FScopeLogTime::ScopeLog_Milliseconds);
		for (int32 i = 0; i < 10000; i++)
		{
			bResult &= TagContainer.MatchesAll(TagContainer2, true);
		}
	}

	return bResult;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameplayTagTest, "System.GameplayTags.GameplayTag", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGameplayTagTest::RunTest(const FString& Parameters)
{
	// Create Test Data 
	UDataTable* DataTable = CreateGameplayDataTable();

	IGameplayTagsModule::Get().GetGameplayTagsManager().PopulateTreeFromDataTable(DataTable);

	// Run Tests
	bool bSuccess = true;
	bSuccess &= GameplayTagTest_SimpleTest();
	bSuccess &= GameplayTagTest_TagComparisonTest();
	bSuccess &= GameplayTagTest_TagContainerTest();
	bSuccess &= GameplayTagTest_PerfTest();
	// Add more tests here... 

	return bSuccess;
}

#endif //WITH_EDITOR 

#endif //WITH_DEV_AUTOMATION_TESTS