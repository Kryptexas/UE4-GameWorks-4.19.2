// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "AssetRegistryModule.h"
#include "ModuleManager.h"
#include "AutomationCommon.h"

#include "Tests/AutomationTestSettings.h"

//Includes needed for opening certain assets
#include "Materials/MaterialFunction.h"
#include "Slate/SlateBrushAsset.h"

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
	FString ConvertPackagePathToAssetPath(const FString& PackagePath);

	/**
	* Imports an object using a given factory
	*
	* @param ImportFactory - The factory to use to import the object
	* @param ObjectName - The name of the object to create
	* @param PackagePath - The full path of the package file to create
	* @param ImportPath - The path to the object to import
	*/
	UObject* ImportAssetUsingFactory(UFactory* ImportFactory, const FString& ObjectName, const FString& PackageName, const FString& ImportPath);

	/**
	* Nulls out references to a given object
	*
	* @param InObject - Object to null references to
	*/
	void NullReferencesToObject(UObject* InObject);

	/**
	* gets a factory class based off an asset file extension
	*
	* @param AssetExtension - The file extension to use to find a supporting UFactory
	*/
	UClass* GetFactoryClassForType(const FString& AssetExtension);

	/**
	* Applies settings to an object by finding UProperties by name and calling ImportText
	*
	* @param InObject - The object to search for matching properties
	* @param PropertyChain - The list UProperty names recursively to search through
	* @param Value - The value to import on the found property
	*/
	void ApplyCustomFactorySetting(UObject* InObject, TArray<FString>& PropertyChain, const FString& Value);

	/**
	* Applies the custom factory settings
	*
	* @param InFactory - The factory to apply custom settings to
	* @param FactorySettings - An array of custom settings to apply to the factory
	*/
	void ApplyCustomFactorySettings(UFactory* InFactory, const TArray<FImportFactorySettingValues>& FactorySettings);
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
	static void CollectGameContentTestsByClass(UClass * Class, bool bRecursiveClass, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
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
		AssetFilter.bRecursiveClasses = bRecursiveClass;
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

	/**
	* Generates a list of misc. assets from the GAME.
	* This is to be used by the GetTest() function.
	*/
	static void CollectMiscGameContentTestsByClass(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
	{
		//Setting the Asset Registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		TArray<FAssetData> ObjectList;
		FARFilter AssetFilter;

		//This is a list of classes that we don't want to be in the misc category.
		TArray<FName> ExcludeClassesList;
		ExcludeClassesList.Add(UTexture::StaticClass()->GetFName());
		ExcludeClassesList.Add(UTexture2D::StaticClass()->GetFName());
		ExcludeClassesList.Add(UTextureRenderTarget::StaticClass()->GetFName());
		ExcludeClassesList.Add(UTextureRenderTarget2D::StaticClass()->GetFName());
		ExcludeClassesList.Add(UTextureCube::StaticClass()->GetFName());
		ExcludeClassesList.Add(UAimOffsetBlendSpace::StaticClass()->GetFName());
		ExcludeClassesList.Add(UAimOffsetBlendSpace1D::StaticClass()->GetFName());
		ExcludeClassesList.Add(UAnimMontage::StaticClass()->GetFName());
		ExcludeClassesList.Add(UAnimSequence::StaticClass()->GetFName());
		ExcludeClassesList.Add(UBlendSpace::StaticClass()->GetFName());
		ExcludeClassesList.Add(UBlendSpace1D::StaticClass()->GetFName());
		ExcludeClassesList.Add(UDestructibleMesh::StaticClass()->GetFName());
		ExcludeClassesList.Add(UDialogueVoice::StaticClass()->GetFName());
		ExcludeClassesList.Add(UDialogueWave::StaticClass()->GetFName());
		ExcludeClassesList.Add(UMaterial::StaticClass()->GetFName());
		ExcludeClassesList.Add(UMaterialFunction::StaticClass()->GetFName());
		ExcludeClassesList.Add(UMaterialInstanceConstant::StaticClass()->GetFName());
		ExcludeClassesList.Add(UMaterialParameterCollection::StaticClass()->GetFName());
		ExcludeClassesList.Add(UParticleSystem::StaticClass()->GetFName());
		ExcludeClassesList.Add(UPhysicalMaterial::StaticClass()->GetFName());
		ExcludeClassesList.Add(UReverbEffect::StaticClass()->GetFName());
		ExcludeClassesList.Add(USlateWidgetStyleAsset::StaticClass()->GetFName());
		ExcludeClassesList.Add(USlateBrushAsset::StaticClass()->GetFName());
		ExcludeClassesList.Add(USoundAttenuation::StaticClass()->GetFName());
		ExcludeClassesList.Add(UStaticMesh::StaticClass()->GetFName());
		ExcludeClassesList.Add(USkeletalMesh::StaticClass()->GetFName());
		ExcludeClassesList.Add(USkeleton::StaticClass()->GetFName());
		ExcludeClassesList.Add(USoundClass::StaticClass()->GetFName());
		ExcludeClassesList.Add(USoundCue::StaticClass()->GetFName());
		ExcludeClassesList.Add(USoundMix::StaticClass()->GetFName());
		ExcludeClassesList.Add(USoundWave::StaticClass()->GetFName());
		ExcludeClassesList.Add(USubsurfaceProfile::StaticClass()->GetFName());
		ExcludeClassesList.Add(UFont::StaticClass()->GetFName());
		ExcludeClassesList.Add(UBlueprint::StaticClass()->GetFName());
		ExcludeClassesList.Add(UAnimBlueprint::StaticClass()->GetFName());
		
		//Generating the list of assets.
		//This list isn't expected to have anything that may be obtained with another function.
		//This list is being filtered by the game folder and class type.  The results are placed into the ObjectList variable.
		AssetFilter.PackagePaths.Add("/Game");
		AssetFilter.bRecursivePaths = true;
		AssetRegistryModule.Get().GetAssets(AssetFilter, ObjectList);
		
		//Loop through the list of assets, make their path full and a string, then add them to the test.
		for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
		{
			const FAssetData & Asset = *ObjIter;
			//Variable that holds the class FName for the current asset iteration.
			FName AssetClassFName = Asset.GetClass()->GetFName();

			//Counter used to keep track for the following for loop.
			float ExcludedClassesCounter = 1;

			for (auto ExcludeIter = ExcludeClassesList.CreateConstIterator(); ExcludeIter; ++ExcludeIter)
			{
				FName ExludedName = *ExcludeIter;
				
				//If the classes are the same then we don't want this asset. So we move onto the next one instead.
				if (AssetClassFName == ExludedName)
				{
					break;
				}

				//We run out of class names in our Excluded list then we want the current ObjectList asset.
				if ((ExcludedClassesCounter + 1) > ExcludeClassesList.Num())
				{
					FString Filename = Asset.ObjectPath.ToString();
					//convert to full paths
					Filename = FPackageName::LongPackageNameToFilename(Filename);

					if (FAutomationTestFramework::GetInstance().ShouldTestContent(Filename))
					{
						FString BeautifiedFilename = Asset.AssetName.ToString();
						OutBeautifiedNames.Add(BeautifiedFilename);
						OutTestCommands.Add(Asset.ObjectPath.ToString());
					}

					break;
				}
				
				//increment the counter.
				ExcludedClassesCounter++;
			}
		}
	}
};


