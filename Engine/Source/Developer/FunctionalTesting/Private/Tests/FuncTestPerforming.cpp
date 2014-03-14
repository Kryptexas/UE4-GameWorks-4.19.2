// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "FunctionalTestingPrivatePCH.h"
#if WITH_EDITOR
//#include "UnrealEd.h"
#include "Editor/UnrealEd/Classes/Editor/EditorEngine.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#endif // WITH_EDITOR

#define LOCTEXT_NAMESPACE "FunctionalTesting"

struct FEditorProxy 
{
	FEditorProxy()
		: EditorEngine(NULL)
	{}

	UEditorEngine* operator->()
	{
		if (EditorEngine == NULL)
		{
			EditorEngine = Cast<UEditorEngine>(GEngine);
		}
		return EditorEngine;
	}

protected:
	UEditorEngine* EditorEngine;
};

static FEditorProxy EditorEngine;


DEFINE_LOG_CATEGORY_STATIC(LogFunctionalTesting, Log, All);

/**
 * Wait for the given amount of time
 */
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FFTestWaitLatentCommand, float, Duration);

bool FFTestWaitLatentCommand::Update()
{
	float NewTime = FPlatformTime::Seconds();
	if (NewTime - StartTime >= Duration)
	{
		return true;
	}
	return false;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FWaitForFTestsToFinish);
bool FWaitForFTestsToFinish::Update()
{
	return FFunctionalTestingModule::Get()->IsRunning() == false;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FTriggerFTests);
bool FTriggerFTests::Update()
{
	IFuncTestManager* Manager = FFunctionalTestingModule::Get();
	// if tests have been already triggered by level script just make sure it's not looping
	if (Manager->IsRunning())
	{
		FFunctionalTestingModule::Get()->SetLooping(false);
	}
	else
	{
		FFunctionalTestingModule::Get()->RunAllTestsOnMap(false, false);
	}

	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FFTestEndPlay);
bool FFTestEndPlay::Update()
{
#if WITH_EDITOR
	if (GIsEditor && EditorEngine->PlayWorld != NULL)
	{
		EditorEngine->EndPlayMap();
	}
#endif // WITH_EDITOR
	return true;
}

DEFINE_LATENT_AUTOMATION_COMMAND(FStartFTestsOnMap);
bool FStartFTestsOnMap::Update()
{
	ADD_LATENT_AUTOMATION_COMMAND(FFTestWaitLatentCommand(3.f));
	ADD_LATENT_AUTOMATION_COMMAND(FTriggerFTests);
	ADD_LATENT_AUTOMATION_COMMAND(FWaitForFTestsToFinish);	
	ADD_LATENT_AUTOMATION_COMMAND(FFTestEndPlay);
	
	return true;
}

/**
 * 
 */
IMPLEMENT_COMPLEX_AUTOMATION_TEST( FFunctionalTestingMaps, "Maps.Functional Testing", EAutomationTestFlags::ATF_Game | EAutomationTestFlags::ATF_Editor )


/** 
 * Requests a enumeration of all maps to be loaded
 */
void FFunctionalTestingMaps::GetTests(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands) const
{
	TArray<FString> FileList;
#if WITH_EDITOR
	FEditorFileUtils::FindAllPackageFiles(FileList);
#else
	// Look directly on disk. Very slow!
	{
		TArray<FString> RootContentPaths;
		FPackageName::QueryRootContentPaths( RootContentPaths );
		for( TArray<FString>::TConstIterator RootPathIt( RootContentPaths ); RootPathIt; ++RootPathIt )
		{
			const FString& RootPath = *RootPathIt;
			const FString& ContentFolder = FPackageName::LongPackageNameToFilename( RootPath );
			FileList.Add( ContentFolder );
		}
	}
#endif

	// Iterate over all files, adding the ones with the map extension..
	for( int32 FileIndex=0; FileIndex< FileList.Num(); FileIndex++ )
	{
		const FString& Filename = FileList[FileIndex];

		const FString BaseFilename = FPaths::GetBaseFilename(Filename);
		// Disregard filenames that don't have the map extension if we're in MAPSONLY mode.
		if ( FPaths::GetExtension(Filename, true) == FPackageName::GetMapPackageExtension() 
			&& BaseFilename.Find(TEXT("FTEST_")) == 0) 
		{
			if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
			{
				OutBeautifiedNames.Add(BaseFilename);
				OutTestCommands.Add(Filename);
			}
		}
	}

	FMessageLog("FunctionalTestingLog").NewPage(LOCTEXT("NewLogLabel", "Functional Test"));
}

/** 
 * Execute the loading of each map and performance captures
 *
 * @param Parameters - Should specify which map name to load
 * @return	TRUE if the test was successful, FALSE otherwise
 */
bool FFunctionalTestingMaps::RunTest(const FString& Parameters)
{
	FString MapName = Parameters;

#if WITH_EDITOR
	if (GIsEditor)
	{
		bool bLoadAsTemplate = false;
		bool bShowProgress = false;
		FEditorFileUtils::LoadMap(MapName, bLoadAsTemplate, bShowProgress);

		const bool bRunInPIE = MapName.EndsWith(TEXT("_PIE"));
		UE_LOG(LogFunctionalTesting, Display, TEXT("Running map %s in %s mode"), *MapName, bRunInPIE ? TEXT("PIE") : TEXT("SIMULATION"));

		EditorEngine->PlayInEditor( GWorld, bRunInPIE );
	}
	else
#endif // WITH_EDITOR
	{
		GEngine->Exec( NULL, *FString::Printf(TEXT("Open %s"), *MapName));
	}
	ADD_LATENT_AUTOMATION_COMMAND(FStartFTestsOnMap());

	return true;
}

#undef LOCTEXT_NAMESPACE