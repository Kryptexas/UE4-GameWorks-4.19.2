// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"
#if WITH_EDITOR
#include "FileHelpers.h"
#endif

#include "AutomationTestCommon.h"
#include "PlatformFeatures.h"
#include "SaveGameSystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogEngineAutomationTests, Log, All);

namespace
{
	UWorld* GetSimpleEngineAutomationTestGameWorld(const int32 TestFlags)
	{
		// Accessing the game world is only valid for game-only 
		check((TestFlags & EAutomationTestFlags::ATF_ApplicationMask) == EAutomationTestFlags::ATF_Game);
		check(GEngine->GetWorldContexts().Num() == 1);
		check(GEngine->GetWorldContexts()[0].WorldType == EWorldType::Game);

		return GEngine->GetWorldContexts()[0].World();
	}
}

/**
 * SetRes Verification - Verify changing resolution works
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FSetResTest, "Windows.Set Resolution", EAutomationTestFlags::ATF_Game )

/** 
 * Change resolutions, wait, and change back
 *
 * @param Parameters - Unused for this test
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FSetResTest::RunTest(const FString& Parameters)
{
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);
	FString MapName = AutomationTestSettings->AutomationTestmap.FilePath;
	GEngine->Exec(GetSimpleEngineAutomationTestGameWorld(GetTestFlags()), *FString::Printf(TEXT("Open %s"), *MapName));

	int32 ResX = GSystemResolution.ResX;
	int32 ResY = GSystemResolution.ResY;
	FString RestoreResolutionString = FString::Printf(TEXT("setres %dx%d"), ResX, ResY);

	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("setres 640x480")));
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(2.0f));
	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(RestoreResolutionString));

	return true;
}

/**
 * Stats verification - Toggle various "stats" commands
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FStatsVerificationMapTest, "Maps.Stats Verification", EAutomationTestFlags::ATF_Game )

/** 
 * Execute the loading of one map to verify screen captures and performance captures work
 *
 * @param Parameters - Unused for this test
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FStatsVerificationMapTest::RunTest(const FString& Parameters)
{
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);
	FString MapName = AutomationTestSettings->AutomationTestmap.FilePath;

	GEngine->Exec(GetSimpleEngineAutomationTestGameWorld(GetTestFlags()), *FString::Printf(TEXT("Open %s"), *MapName));

	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat game")));
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0));
	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat game")));

	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat scenerendering")));
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0));
	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat scenerendering")));

	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat memory")));
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0));
	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat memory")));

	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat slate")));
	ADD_LATENT_AUTOMATION_COMMAND(FEngineWaitLatentCommand(1.0));
	ADD_LATENT_AUTOMATION_COMMAND(FExecStringLatentCommand(TEXT("stat slate")));

	return true;
}

/**
 * LoadAutomationMap
 * Verification automation test to make sure features of map loading work (load, screen capture, performance capture)
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FPerformanceCaptureTest, "Maps.Performance Capture", EAutomationTestFlags::ATF_Game )

/** 
 * Execute the loading of one map to verify screen captures and performance captures work
 *
 * @param Parameters - Unused for this test
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FPerformanceCaptureTest::RunTest(const FString& Parameters)
{
	UAutomationTestSettings const* AutomationTestSettings = GetDefault<UAutomationTestSettings>();
	check(AutomationTestSettings);
	FString MapName = AutomationTestSettings->AutomationTestmap.FilePath;

	GEngine->Exec(GetSimpleEngineAutomationTestGameWorld(GetTestFlags()), *FString::Printf(TEXT("Open %s"), *MapName));
	ADD_LATENT_AUTOMATION_COMMAND(FEnqueuePerformanceCaptureCommands());

	return true;
}

/**
 * LoadAllMapsInGame
 * Verification automation test to make sure loading all maps succeed without crashing AND does performance captures
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FLoadAllMapsInGameTest, "Maps.Load All In Game", EAutomationTestFlags::ATF_Game )

/** 
 * Requests a enumeration of all maps to be loaded
 */
void FLoadAllMapsInGameTest::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	TArray<FString> FileList;
#if WITH_EDITOR
	FEditorFileUtils::FindAllPackageFiles(FileList);
#else
	// Look directly on disk. Very slow!
	FPackageName::FindPackagesInDirectory(FileList, *FPaths::GameContentDir());
#endif

	// Iterate over all files, adding the ones with the map extension..
	for( int32 FileIndex=0; FileIndex< FileList.Num(); FileIndex++ )
	{
		const FString& Filename = FileList[FileIndex];

		// Disregard filenames that don't have the map extension if we're in MAPSONLY mode.
		if ( FPaths::GetExtension(Filename, true) == FPackageName::GetMapPackageExtension() ) 
		{
			if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
			{
				OutBeautifiedNames.Add(FPaths::GetBaseFilename(Filename));
				OutTestCommands.Add(Filename);
			}
		}
	}
}

/** 
 * Execute the loading of each map and performance captures
 *
 * @param Parameters - Should specify which map name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FLoadAllMapsInGameTest::RunTest(const FString& Parameters)
{
	FString MapName = Parameters;

	GEngine->Exec(GetSimpleEngineAutomationTestGameWorld(GetTestFlags()), *FString::Printf(TEXT("Open %s"), *MapName));
	ADD_LATENT_AUTOMATION_COMMAND(FEnqueuePerformanceCaptureCommands());

	return true;
}

/**
 * SaveGameTest
 * Test makes sure a save game (without UI) saves and loads correctly
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST( FSaveGameTest, "Engine.Game.Noninteractive Save", EAutomationTestFlags::ATF_Game )

/** 
 * Saves and loads a savegame file
 *
 * @param Parameters - Unused for this test
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FSaveGameTest::RunTest(const FString& Parameters)
{
	// automation save name
	const TCHAR* SaveName = TEXT("AutomationSaveTest");
	uint32 SavedData = 99;

	// the blob we are going to write out
	TArray<uint8> Blob;
	FMemoryWriter WriteAr(Blob);
	WriteAr << SavedData;

	// get the platform's save system
	ISaveGameSystem* Save = IPlatformFeaturesModule::Get().GetSaveGameSystem();

	// write it out
	if (Save->SaveGame(false, SaveName, Blob) == false)
	{
		return false;
	}

	// make sure it was written
	if (Save->DoesSaveGameExist(SaveName) == false)
	{
		return false;
	}

	// read it back in
	Blob.Empty();
	if (Save->LoadGame(false, SaveName, Blob) == false)
	{
		return false;
	}

	// make sure it's the same data
	FMemoryReader ReadAr(Blob);
	uint32 LoadedData;
	ReadAr << LoadedData;

	return LoadedData == SavedData;
}


/* UAutomationTestSettings interface
 *****************************************************************************/

UAutomationTestSettings::UAutomationTestSettings( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
}
