// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnitTestFrameworkEditorPCH.h"
#include "UTFEditorAutomationTests.h"
#include "AutomationTest.h"
#include "AssetRegistryModule.h"	// for FAssetRegistry::GetAssets()
#include "AssetData.h"
#include "Engine/World.h"			// for UWorld::StaticClass()
#include "PackageName.h"			// for FPackageName::LongPackageNameToFilename()
#include "FileHelpers.h"			// for FEditorFileUtils::LoadMap()
#include "Editor.h"					// for FEditorDelegates::OnMapOpened
#include "UTFUnitTestInterface.h"
#include "Script.h"					// for FEditorScriptExecutionGuard

/*******************************************************************************
 * FUTFEditorAutomationTests
 ******************************************************************************/
DEFINE_LOG_CATEGORY_STATIC(LogUTFEditorAutomationTests, Log, All);

const FName FUTFEditorAutomationTests::UnitTestAssetTag(TEXT("UTF_UnitTestLevel"));

/*******************************************************************************
 * FUTFEditorMapTest
 ******************************************************************************/

IMPLEMENT_COMPLEX_AUTOMATION_TEST(FUTFEditorMapTest, "UnitTestFramework.Editor.RunUnitTestMaps", EAutomationTestFlags::ATF_Editor)

//------------------------------------------------------------------------------
void FUTFEditorMapTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	FARFilter AssetFilter;
	AssetFilter.ClassNames.Add(UWorld::StaticClass()->GetFName());
	AssetFilter.TagsAndValues.Add(FUTFEditorAutomationTests::UnitTestAssetTag, TEXT("TRUE"));

	TArray<FAssetData> UnitTestLevels;

	IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	AssetRegistry.GetAssets(AssetFilter, UnitTestLevels);

	for (FAssetData const& LevelAsset : UnitTestLevels)
	{
		OutTestCommands.Add(LevelAsset.ObjectPath.ToString());
		OutBeautifiedNames.Add(LevelAsset.AssetName.ToString());
	}
}

//------------------------------------------------------------------------------
bool FUTFEditorMapTest::RunTest(const FString& LevelAssetPath)
{
	// don't care about any map load warnings/errors, etc. 
	SetSuppressLogs(true);

	FDelegateHandle OnMapOpenedHandle = FEditorDelegates::OnMapOpened.AddLambda([this, &LevelAssetPath](const FString& /*Filename*/, bool /*bAsTemplate*/)
	{ 
		SetSuppressLogs(false);

		FString PackageName, AssetName;
		LevelAssetPath.Split(TEXT("."), &PackageName, &AssetName);

		UPackage* WorldPackage = FindPackage(/*Outer =*/nullptr, *PackageName);
		if (UWorld* TestWorld = FindObject<UWorld>(WorldPackage, *AssetName))
		{
			if (ULevel* TestLevel = TestWorld->PersistentLevel)
			{
				bool bExecutedTests = false;
				for (AActor* Actor : TestLevel->Actors)
				{
					if (Actor == nullptr)
					{
						continue;
					}

					UClass* ActorClass = Actor->GetClass();
					if (ActorClass->ImplementsInterface(UUTFUnitTestInterface::StaticClass()))
					{
						// notifies ProcessEvent() that it is ok to execute the following functions in the editor
						FEditorScriptExecutionGuard ScriptGuard;

						const FString TestName = IUTFUnitTestInterface::Execute_GetTestName(Actor);
						// @TODO: consider the need for flagging tests as "runtime only" (cannot be ran in the editor)
						EUTFUnitTestResult TestResult = IUTFUnitTestInterface::Execute_GetTestResult(Actor);
						switch (TestResult)
						{
						case EUTFUnitTestResult::UTF_Success:
							{
								AddLogItem(FString::Printf(TEXT("Unit Test '%s': SUCCESS"), *TestName));
							} break;

						case EUTFUnitTestResult::UTF_Failure:
							{
								AddError(FString::Printf(TEXT("Unit Test '%s': FAILURE"), *TestName));
							} break;

						default:
						case EUTFUnitTestResult::UTF_Unresolved:
							{
								AddWarning(FString::Printf(TEXT("Unit Test '%s': UNRESOLVED"), *TestName));
							} break;
						}
						
						bExecutedTests = true;
					}
				}

				if (!bExecutedTests)
				{
					AddWarning(FString::Printf(TEXT("No editor unit tests found in '%s'."), *PackageName));
				}
			}
			else
			{
				AddError(FString::Printf(TEXT("Null PersistentLevel after loading '%s'."), *PackageName));
			}
		}
		else
		{
			AddError(FString::Printf(TEXT("Failed to find '%s' world object, post load."), *PackageName));
		}
	});

	const FString MapFilePath = FPackageName::LongPackageNameToFilename(LevelAssetPath);
	FEditorFileUtils::LoadMap(MapFilePath, /*bLoadAsTemplate =*/false, /*bShowProgress =*/false);
	FEditorDelegates::OnMapOpened.Remove(OnMapOpenedHandle);

	return (ExecutionInfo.Errors.Num() == 0);
}
