// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "Tests/AutomationTestSettings.h"
#include "ModuleManager.h"
#include "AutomationEditorCommon.h"
#include "AutomationCommon.h"
#include "IMainFrameModule.h"



/**
* Map Performance in Editor tests
* Grabs certain performance numbers and saves it to a file.
*/
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FMapPerformanceInEditor, "Performance.Map Performance in Editor", EAutomationTestFlags::ATF_Editor | EAutomationTestFlags::ATF_RequiresUser)

/**
* Requests a enumeration of all maps to be loaded
*/
void FMapPerformanceInEditor::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);

	TArray<FString> PerformanceMapName;
	for (auto Iter = AutomationTestSettings->EditorPerformanceTestMaps.CreateConstIterator(); Iter; ++Iter)
	{
		if (Iter->PerformanceTestmap.FilePath.Len() > 0)
		{
			PerformanceMapName.AddUnique(Iter->PerformanceTestmap.FilePath);
		}
		
		for (int32 indexloop = 0; indexloop < PerformanceMapName.Num(); ++indexloop)
		{

			if (PerformanceMapName[indexloop].Equals(TEXT("CurrentLevel")))
			{
				OutBeautifiedNames.Add(TEXT("CurrentLevel"));
				OutTestCommands.Add(TEXT("CurrentLevel"));
				continue;
			}
		
			//Get the location of the map being used.
			FString Filename = FPaths::ConvertRelativePathToFull(PerformanceMapName[indexloop]);

			if (Filename.Contains(TEXT("/Engine/"), ESearchCase::IgnoreCase, ESearchDir::FromStart))
			{
				//If true it will proceed to add the asset to the test list.
				//This will be false if the map is on a different drive.
				if (FPaths::MakePathRelativeTo(Filename, *FPaths::EngineContentDir()))
				{
					FString ShortName = FPaths::GetBaseFilename(Filename);
					FString PathName = FPaths::GetPath(Filename);
					FString AssetName = FString::Printf(TEXT("/Engine/%s/%s.%s"), *PathName, *ShortName, *ShortName);
					OutBeautifiedNames.Add(ShortName);
					OutTestCommands.Add(AssetName);
				}
				else
				{
					UE_LOG(LogEditorAutomationTests, Error, TEXT("Invalid asset path: %s."), *Filename);
				}
			}
			else
			{
				//If true it will proceed to add the asset to the test list.
				//This will be false if the map is on a different drive.
				if (FPaths::MakePathRelativeTo(Filename, *FPaths::GameContentDir()))
				{
					FString ShortName = FPaths::GetBaseFilename(Filename);
					FString PathName = FPaths::GetPath(Filename);
					FString AssetName = FString::Printf(TEXT("/Game/%s/%s.%s"), *PathName, *ShortName, *ShortName);
					OutBeautifiedNames.Add(ShortName);
					OutTestCommands.Add(AssetName);
				}
				else
				{
					UE_LOG(LogEditorAutomationTests, Error, TEXT("Invalid asset path: %s."), *Filename);
				}
			}
		}
	}
}

bool FMapPerformanceInEditor::RunTest(const FString& Parameters)
{
	//This is used to hold the data that is being captured.
	EditorPerfCaptureParameters EditorPerformanceData;

	//Get the map name from the parameters.
	FString MapName = Parameters;
	
	//Gets the main frame module to get the name of our current level.
	const IMainFrameModule& MainFrameModule = FModuleManager::GetModuleChecked< IMainFrameModule >("MainFrame");
	EditorPerformanceData.MapName = MainFrameModule.GetLoadedLevelName();

	//Duration variable will be used to indicate how long the test will run for.  Defaults to a minute.
	EditorPerformanceData.TestDuration = 60;

	if (!MapName.Equals(TEXT("CurrentLevel"), ESearchCase::IgnoreCase))
	{
		//Get the base filename for the map that will be used.
		EditorPerformanceData.MapName = FPaths::GetBaseFilename(MapName);

		//Load Map and get the time it took to take to load the map.
		ADD_LATENT_AUTOMATION_COMMAND(FEditorLoadMap(MapName));
	}

	//This gets the info we need from the automation test settings in the engine.ini.
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);

	//Now we find the test timer(aka duration) for our test.
	for (auto Iter = AutomationTestSettings->EditorPerformanceTestMaps.CreateConstIterator(); Iter; ++Iter)
	{
		FString IterMapName = FPaths::GetBaseFilename(Iter->PerformanceTestmap.FilePath);
		if ((IterMapName == EditorPerformanceData.MapName || (IterMapName == TEXT("CurrentLevel"))))
		{
			EditorPerformanceData.TestDuration = ((Iter->TestTimer));
			//If the duration is equal to 0 then we simply warn the user that they need to set the test timer option for the performance test.
			//If the duration is less than 0 then we fail this test.
			if (EditorPerformanceData.TestDuration == 0)
			{
				UE_LOG(LogEditorAutomationTests, Warning, TEXT("Please set the test timer for '%s' in the automation preferences or engine.ini."), *EditorPerformanceData.MapName);
			}
			else
			if (EditorPerformanceData.TestDuration < 0)
			{
				UE_LOG(LogEditorAutomationTests, Error, TEXT("Test timer preference option for '%s' is less than 0."), *EditorPerformanceData.MapName);
				return false;
			}
		}
	}

	UE_LOG(LogEditorAutomationTests, Log, TEXT("Running the performance capture test for %.0i seconds on %s"), EditorPerformanceData.TestDuration, *EditorPerformanceData.MapName);
	
	//Move the viewport views to the first bookmark
	ADD_LATENT_AUTOMATION_COMMAND(FChangeViewportToFirstAvailableBookmarkCommand);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));

	//Wait for shaders to finish compile.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForShadersToFinishCompiling);
	
	//Grab the performance numbers based on the duration.
	ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(0.5f));
	ADD_LATENT_AUTOMATION_COMMAND(FEditorPerfCaptureCommand(EditorPerformanceData));

	return true;
}