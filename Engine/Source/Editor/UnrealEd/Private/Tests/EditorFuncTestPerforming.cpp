// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AutomationCommon.h"
#include "FunctionalTestingModule.h"

#include "Engine/Brush.h"
#include "Editor/EditorEngine.h"
#include "EngineUtils.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "AssetRegistryModule.h"

//extern UNREALED_API class UEditorEngine* GEditor;

#define LOCTEXT_NAMESPACE "EditorFunctionalTesting"

DEFINE_LOG_CATEGORY_STATIC(LogFunctionalTesting, Log, All);

DEFINE_LATENT_AUTOMATION_COMMAND(FWaitForFTestsToFinish);
bool FWaitForFTestsToFinish::Update()
{
	return FFunctionalTestingModule::Get()->IsRunning() == false;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FTriggerFTests);
bool FTriggerFTests::Update()
{
	IFuncTestManager* Manager = FFunctionalTestingModule::Get();
	if (Manager->IsFinished())
	{
		// if tests have been already triggered by level script just make sure it's not looping
		if (Manager->IsRunning())
		{
			FFunctionalTestingModule::Get()->SetLooping(false);
		}
		else
		{
			FFunctionalTestingModule::Get()->RunAllTestsOnMap(false, false);
		}
	}

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FFTestEndPlay);
bool FFTestEndPlay::Update()
{
	if (GIsEditor && GEditor->PlayWorld != NULL)
	{
		GEditor->EndPlayMap();
	}
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FStartFTestsOnMap, bool, bQuitAfterCompletion);
bool FStartFTestsOnMap::Update()
{
	//should really be wait until the map is properly loaded....in PIE or gameplay....
	if (bQuitAfterCompletion)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(10.f));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FTriggerFTests);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForFTestsToFinish);
	if (bQuitAfterCompletion)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FFTestEndPlay);
	}
	
	return true;
}

/**
 * 
 */

// create test base class
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FEditorFunctionalTestingMapsBase, "Project.Maps.Editor Functional Testing", (EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter))

/** 
 * Requests a enumeration of all maps to be loaded
 */
void FEditorFunctionalTestingMapsBase::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	//Setting the Asset Registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	//Variable setups
	TArray<FAssetData> ObjectList;
	FARFilter AssetFilter;

	//Generating the list of assets.
	//This list is being filtered by the game folder and class type.  The results are placed into the ObjectList variable.
	AssetFilter.ClassNames.Add(UWorld::StaticClass()->GetFName());

	//removed path as a filter as it causes two large lists to be sorted.  Filtering on "game" directory on iteration
	//AssetFilter.PackagePaths.Add("/Game");
	AssetFilter.bRecursiveClasses = false;
	AssetFilter.bRecursivePaths = true;
	AssetRegistryModule.Get().GetAssets(AssetFilter, ObjectList);

	//Loop through the list of assets, make their path full and a string, then add them to the test.
	for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
	{
		const FAssetData& Asset = *ObjIter;
		FString Filename = Asset.ObjectPath.ToString();

		if (Filename.StartsWith("/Game"))
		{
			//convert to full paths
			Filename = FPackageName::LongPackageNameToFilename(Filename);

			const FString BaseFilename = FPaths::GetBaseFilename(Filename);
			// Disregard filenames that don't have the map extension if we're in MAPSONLY mode.
			if (BaseFilename.Find(TEXT("FTEST_")) == 0)
			{
				if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
				{
					OutBeautifiedNames.Add(BaseFilename);
					OutTestCommands.Add(Filename);
				}
			}
		}
	}
}

struct FFailedGameStartHandler
{
	bool bCanProceed;

	FFailedGameStartHandler()
	{
		bCanProceed = true;
		FEditorDelegates::EndPIE.AddRaw(this, &FFailedGameStartHandler::OnEndPIE);
	}

	~FFailedGameStartHandler()
	{
		FEditorDelegates::EndPIE.RemoveAll(this);
	}

	bool CanProceed() const { return bCanProceed; }

	void OnEndPIE(const bool bInSimulateInEditor)
	{
		bCanProceed = false;
	}
};

/** 
 * Execute the loading of each map and performance captures
 *
 * @param Parameters - Should specify which map name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FEditorFunctionalTestingMapsBase::RunTest(const FString& Parameters)
{
	FString MapName = Parameters;

	bool bCanProceed = true;

	bool bLoadAsTemplate = false;
	bool bShowProgress = false;

	bool bNeedLoadEditorMap = true;
	bool bNeedPieStart = true;
	bool bPieRunning = false;

	//check existing worlds
	const TIndirectArray<FWorldContext> WorldContexts = GEngine->GetWorldContexts();
	for (auto& Context : WorldContexts)
	{
		if (Context.WorldType == EWorldType::PIE)
		{
			//don't quit!  This was triggered while pie was already running!
			bNeedPieStart = !MapName.Contains(Context.World()->GetName());
			bPieRunning = true;
			break;
		}
		else if (Context.WorldType == EWorldType::Editor)
		{
			bNeedLoadEditorMap = !MapName.Contains(Context.World()->GetName());
		}
	}


	if (bNeedLoadEditorMap)
	{
		if (bPieRunning)
		{
			GEditor->EndPlayMap();
		}
		FEditorFileUtils::LoadMap(MapName, bLoadAsTemplate, bShowProgress);
		bNeedPieStart = true;
	}
	// special precaution needs to be taken while triggering PIE since it can
	// fail if there are BP compilation issues
	if (bNeedPieStart)
	{
		FFailedGameStartHandler FailHandler;
		GEditor->PlayInEditor(GWorld, /*bInSimulateInEditor=*/false);
		bCanProceed = FailHandler.CanProceed();
	}

	if (bCanProceed)
	{
		ADD_LATENT_AUTOMATION_COMMAND(FStartFTestsOnMap(bNeedPieStart));
		return true;
	}

	UE_LOG(LogFunctionalTesting, Error, TEXT("Failed to start the %s map (possibly due to BP compilation issues)"), *MapName);
	return false;
}

#undef LOCTEXT_NAMESPACE