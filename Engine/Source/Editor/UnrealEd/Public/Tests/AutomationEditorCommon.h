// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AssetRegistryModule.h"
#include "ModuleManager.h"
#include "AutomationCommon.h"

#include "Tests/AutomationTestSettings.h"

namespace AutomationEditorCommonUtils
{
	/**
	* Creates a new map for editing.  Also clears editor tools that could cause issues when changing maps
	*
	* @return - The UWorld for the new map
	*/
	static UWorld* CreateNewMap()
	{
		// Change out of Matinee when opening new map, so we avoid editing data in the old one.
		if (GLevelEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_InterpEdit))
		{
			GLevelEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_InterpEdit);
		}

		// Also change out of Landscape mode to ensure all references are cleared.
		if (GLevelEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_Landscape))
		{
			GLevelEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_Landscape);
		}

		// Also change out of Foliage mode to ensure all references are cleared.
		if (GLevelEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_Foliage))
		{
			GLevelEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_Foliage);
		}

		// Change out of mesh paint mode when opening a new map.
		if (GLevelEditorModeTools().IsModeActive(FBuiltinEditorModes::EM_MeshPaint))
		{
			GLevelEditorModeTools().DeactivateMode(FBuiltinEditorModes::EM_MeshPaint);
		}

		return GEditor->NewMap();
	}

	/**
	* Converts a package path to an asset path
	*
	* @param PackagePath - The package path to convert
	*/
	static FString ConvertPackagePathToAssetPath(const FString& PackagePath);

	/**
	* Imports an object using a given factory
	*
	* @param ImportFactory - The factory to use to import the object
	* @param ObjectName - The name of the object to create
	* @param PackagePath - The full path of the package file to create
	* @param ImportPath - The path to the object to import
	*/
	static UObject* ImportAssetUsingFactory(UFactory* ImportFactory, const FString& ObjectName, const FString& PackageName, const FString& ImportPath);

	/**
	* Nulls out references to a given object
	*
	* @param InObject - Object to null references to
	*/
	static void NullReferencesToObject(UObject* InObject);

	/**
	* gets a factory class based off an asset file extension
	*
	* @param AssetExtension - The file extension to use to find a supporting UFactory
	*/
	static UClass* GetFactoryClassForType(const FString& AssetExtension);

	/**
	* Applies settings to an object by finding UProperties by name and calling ImportText
	*
	* @param InObject - The object to search for matching properties
	* @param PropertyChain - The list UProperty names recursively to search through
	* @param Value - The value to import on the found property
	*/
	static void ApplyCustomFactorySetting(UObject* InObject, TArray<FString>& PropertyChain, const FString& Value);

	/**
	* Applies the custom factory settings
	*
	* @param InFactory - The factory to apply custom settings to
	* @param FactorySettings - An array of custom settings to apply to the factory
	*/
	static void ApplyCustomFactorySettings(UFactory* InFactory, const TArray<FImportFactorySettingValues>& FactorySettings);
}



//////////////////////////////////////////////////////////////////////////
//Common latent commands used for automated editor testing.

/**
* Creates a latent command which the user can either undo or redo an action.
* True will trigger Undo, False will trigger a redo
*/
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FUndoRedoCommand, bool, bUndo);

/**
* Open editor for a particular asset
*/
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FOpenEditorForAssetCommand, FString, AssetName);

/**
* Close all asset editors
*/
DEFINE_LATENT_AUTOMATION_COMMAND(FCloseAllAssetEditorsCommand);

/**
* Start PIE session
*/
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FStartPIECommand, bool, bSimulateInEditor);

/**
* End PlayMap session
*/
DEFINE_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand);


//////////////////////////////////////////////////////////////////////////
// FEditorAutomationTestUtilities

class FEditorAutomationTestUtilities
{
public:
	/**
	* Loads the map specified by an automation test
	*
	* @param MapName - Map to load
	*/
	static void LoadMap(const FString& MapName)
	{
		bool bLoadAsTemplate = false;
		bool bShowProgress = false;
		FEditorFileUtils::LoadMap(MapName, bLoadAsTemplate, bShowProgress);
	}

	/**
	* Run PIE
	*/
	static void RunPIE()
	{
		bool bInSimulateInEditor = true;
		//once in the editor
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(true));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());

		//wait between tests
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(1.0f));

		//once not in the editor
		ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
		ADD_LATENT_AUTOMATION_COMMAND(FWaitLatentCommand(3.0f));
		ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	}

	/**
	* Generates a list of assets from the ENGINE and the GAME by a specific type.
	* This is to be used by the GetTest() function.
	*/
	static void CollectTestsByClass(UClass * Class, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> ObjectList;
		AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), ObjectList);

		for (TObjectIterator<UClass> AllClassesIt; AllClassesIt; ++AllClassesIt)
		{
			UClass* ClassList = *AllClassesIt;
			FName ClassName = ClassList->GetFName();
			//UE_LOG(LogEditorAutomationTests, Log, TEXT("Here: %s"), *ClassName);
		}


		for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
		{
			const FAssetData & Asset = *ObjIter;
			FString Filename = Asset.ObjectPath.ToString();
			//convert to full paths
			Filename = FPackageName::LongPackageNameToFilename(Filename);
			if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
			{
				FString BeautifiedFilename = Asset.AssetName.ToString();
				OutBeautifiedNames.Add(BeautifiedFilename);
				OutTestCommands.Add(Asset.ObjectPath.ToString());
			}
		}
	}

	/**
	* Generates a list of assets from the GAME by a specific type.
	* This is to be used by the GetTest() function.
	*/
	static void CollectGameContentTestsByClass(UClass * Class, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
	{
		//Setting the Asset Registry

		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		
		//Variable setups
		TArray<FAssetData> ObjectList;
		FARFilter AssetFilter;

		//Generating the list of assets.
		//This list is being filtered by the game folder and class type.  The results are placed into the ObjectList variable.
		AssetFilter.ClassNames.Add(Class->GetFName());
		AssetFilter.PackagePaths.Add("/Game");
		AssetFilter.bRecursivePaths = true;
		AssetRegistryModule.Get().GetAssets(AssetFilter, ObjectList);

		//Loop through the list of assets, make their path full and a string, then add them to the test.
		for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
		{
			const FAssetData & Asset = *ObjIter;
			FString Filename = Asset.ObjectPath.ToString();
			//convert to full paths
			Filename = FPackageName::LongPackageNameToFilename(Filename);
			if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
			{
				FString BeautifiedFilename = Asset.AssetName.ToString();
				OutBeautifiedNames.Add(BeautifiedFilename);
				OutTestCommands.Add(Asset.ObjectPath.ToString());
			}
		}
	}
};


