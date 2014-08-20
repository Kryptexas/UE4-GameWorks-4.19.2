// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "UnrealEd.h"
#include "AutomationEditorCommon.h"
#include "AssetEditorManager.h"
#include "ModuleManager.h"
#include "LevelEditor.h"
#include "ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAutomationEditorCommon, Log, All);

namespace AutomationEditorCommonUtils
{
	/**
	* Converts a package path to an asset path
	*
	* @param PackagePath - The package path to convert
	*/
	FString ConvertPackagePathToAssetPath(const FString& PackagePath)
	{
		const FString Filename = FPaths::ConvertRelativePathToFull(PackagePath);
		FString EngineFileName = Filename;
		FString GameFileName = Filename;
		if (FPaths::MakePathRelativeTo(EngineFileName, *FPaths::EngineContentDir()) && !FPaths::IsRelative(EngineFileName))
		{
			const FString ShortName = FPaths::GetBaseFilename(EngineFileName);
			const FString PathName = FPaths::GetPath(EngineFileName);
			const FString AssetName = FString::Printf(TEXT("/Engine/%s/%s.%s"), *PathName, *ShortName, *ShortName);
			return AssetName;
		}
		else if (FPaths::MakePathRelativeTo(GameFileName, *FPaths::GameContentDir()) && !FPaths::IsRelative(GameFileName))
		{
			const FString ShortName = FPaths::GetBaseFilename(GameFileName);
			const FString PathName = FPaths::GetPath(GameFileName);
			const FString AssetName = FString::Printf(TEXT("/Game/%s/%s.%s"), *PathName, *ShortName, *ShortName);
			return AssetName;
		}
		else
		{
			UE_LOG(LogAutomationEditorCommon, Error, TEXT("PackagePath (%s) is invalid for the current project"), *PackagePath);
			return TEXT("");
		}
	}

	/**
	* Imports an object using a given factory
	*
	* @param ImportFactory - The factory to use to import the object
	* @param ObjectName - The name of the object to create
	* @param PackagePath - The full path of the package file to create
	* @param ImportPath - The path to the object to import
	*/
	UObject* ImportAssetUsingFactory(UFactory* ImportFactory, const FString& ObjectName, const FString& PackagePath, const FString& ImportPath)
	{
		UObject* ImportedAsset = NULL;

		UPackage* Pkg = CreatePackage(NULL, *PackagePath);
		if (Pkg)
		{
			// Make sure the destination package is loaded
			Pkg->FullyLoad();

			UClass* ImportAssetType = ImportFactory->ResolveSupportedClass();
			bool bDummy = false;

			//If we are a texture factory suppress some warning dialog that we don't want
			if (ImportFactory->IsA(UTextureFactory::StaticClass()))
			{
				UTextureFactory::SuppressImportResolutionWarningDialog();
				UTextureFactory::SuppressImportOverwriteDialog();
			}

			ImportedAsset = UFactory::StaticImportObject(ImportAssetType, Pkg, FName(*ObjectName), RF_Public | RF_Standalone, bDummy, *ImportPath, NULL, ImportFactory, NULL, GWarn, 0);

			if (ImportedAsset)
			{
				UE_LOG(LogAutomationEditorCommon, Display, TEXT("Imported %s"), *ImportPath);
			}
			else
			{
				UE_LOG(LogAutomationEditorCommon, Error, TEXT("Failed to import asset using factory %s!"), *ImportFactory->GetName());
			}
		}
		else
		{
			UE_LOG(LogAutomationEditorCommon, Error, TEXT("Failed to create a package!"));
		}

		return ImportedAsset;
	}

	/**
	* Nulls out references to a given object
	*
	* @param InObject - Object to null references to
	*/
	void NullReferencesToObject(UObject* InObject)
	{
		TArray<UObject*> ReplaceableObjects;
		TMap<UObject*, UObject*> ReplacementMap;
		ReplacementMap.Add(InObject, NULL);
		ReplacementMap.GenerateKeyArray(ReplaceableObjects);

		// Find all the properties (and their corresponding objects) that refer to any of the objects to be replaced
		TMap< UObject*, TArray<UProperty*> > ReferencingPropertiesMap;
		for (FObjectIterator ObjIter; ObjIter; ++ObjIter)
		{
			UObject* CurObject = *ObjIter;

			// Find the referencers of the objects to be replaced
			FFindReferencersArchive FindRefsArchive(CurObject, ReplaceableObjects);

			// Inform the object referencing any of the objects to be replaced about the properties that are being forcefully
			// changed, and store both the object doing the referencing as well as the properties that were changed in a map (so that
			// we can correctly call PostEditChange later)
			TMap<UObject*, int32> CurNumReferencesMap;
			TMultiMap<UObject*, UProperty*> CurReferencingPropertiesMMap;
			if (FindRefsArchive.GetReferenceCounts(CurNumReferencesMap, CurReferencingPropertiesMMap) > 0)
			{
				TArray<UProperty*> CurReferencedProperties;
				CurReferencingPropertiesMMap.GenerateValueArray(CurReferencedProperties);
				ReferencingPropertiesMap.Add(CurObject, CurReferencedProperties);
				for (TArray<UProperty*>::TConstIterator RefPropIter(CurReferencedProperties); RefPropIter; ++RefPropIter)
				{
					CurObject->PreEditChange(*RefPropIter);
				}
			}

		}

		// Iterate over the map of referencing objects/changed properties, forcefully replacing the references and then
		// alerting the referencing objects the change has completed via PostEditChange
		int32 NumObjsReplaced = 0;
		for (TMap< UObject*, TArray<UProperty*> >::TConstIterator MapIter(ReferencingPropertiesMap); MapIter; ++MapIter)
		{
			++NumObjsReplaced;

			UObject* CurReplaceObj = MapIter.Key();
			const TArray<UProperty*>& RefPropArray = MapIter.Value();

			FArchiveReplaceObjectRef<UObject> ReplaceAr(CurReplaceObj, ReplacementMap, false, true, false);

			for (TArray<UProperty*>::TConstIterator RefPropIter(RefPropArray); RefPropIter; ++RefPropIter)
			{
				FPropertyChangedEvent PropertyEvent(*RefPropIter);
				CurReplaceObj->PostEditChangeProperty(PropertyEvent);
			}

			if (!CurReplaceObj->HasAnyFlags(RF_Transient) && CurReplaceObj->GetOutermost() != GetTransientPackage())
			{
				if (!CurReplaceObj->RootPackageHasAnyFlags(PKG_CompiledIn))
				{
					CurReplaceObj->MarkPackageDirty();
				}
			}
		}
	}

	/**
	* gets a factory class based off an asset file extension
	*
	* @param AssetExtension - The file extension to use to find a supporting UFactory
	*/
	UClass* GetFactoryClassForType(const FString& AssetExtension)
	{
		// First instantiate one factory for each file extension encountered that supports the extension
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			if ((*ClassIt)->IsChildOf(UFactory::StaticClass()) && !((*ClassIt)->HasAnyClassFlags(CLASS_Abstract)))
			{
				UFactory* Factory = Cast<UFactory>((*ClassIt)->GetDefaultObject());
				if (Factory->bEditorImport && Factory->ValidForCurrentGame())
				{
					TArray<FString> FactoryExtensions;
					Factory->GetSupportedFileExtensions(FactoryExtensions);

					// Case insensitive string compare with supported formats of this factory
					if (FactoryExtensions.Contains(AssetExtension))
					{
						return *ClassIt;
					}
				}
			}
		}

		return NULL;
	}

	/**
	* Applies settings to an object by finding UProperties by name and calling ImportText
	*
	* @param InObject - The object to search for matching properties
	* @param PropertyChain - The list UProperty names recursively to search through
	* @param Value - The value to import on the found property
	*/
	void ApplyCustomFactorySetting(UObject* InObject, TArray<FString>& PropertyChain, const FString& Value)
	{
		const FString PropertyName = PropertyChain[0];
		PropertyChain.RemoveAt(0);

		UProperty* TargetProperty = FindField<UProperty>(InObject->GetClass(), *PropertyName);
		if (TargetProperty)
		{
			if (PropertyChain.Num() == 0)
			{
				TargetProperty->ImportText(*Value, TargetProperty->ContainerPtrToValuePtr<uint8>(InObject), 0, InObject);
			}
			else
			{
				UStructProperty* StructProperty = Cast<UStructProperty>(TargetProperty);
				UObjectProperty* ObjectProperty = Cast<UObjectProperty>(TargetProperty);

				UObject* SubObject = NULL;
				bool bValidPropertyType = true;

				if (StructProperty)
				{
					SubObject = StructProperty->Struct;
				}
				else if (ObjectProperty)
				{
					SubObject = ObjectProperty->GetObjectPropertyValue(ObjectProperty->ContainerPtrToValuePtr<UObject>(InObject));
				}
				else
				{
					//Unknown nested object type
					bValidPropertyType = false;
					UE_LOG(LogAutomationEditorCommon, Error, TEXT("ERROR: Unknown nested object type for property: %s"), *PropertyName);
				}

				if (SubObject)
				{
					ApplyCustomFactorySetting(SubObject, PropertyChain, Value);
				}
				else if (bValidPropertyType)
				{
					UE_LOG(LogAutomationEditorCommon, Error, TEXT("Error accessing null property: %s"), *PropertyName);
				}
			}
		}
		else
		{
			UE_LOG(LogAutomationEditorCommon, Error, TEXT("ERROR: Could not find factory property: %s"), *PropertyName);
		}
	}

	/**
	* Applies the custom factory settings
	*
	* @param InFactory - The factory to apply custom settings to
	* @param FactorySettings - An array of custom settings to apply to the factory
	*/
	void ApplyCustomFactorySettings(UFactory* InFactory, const TArray<FImportFactorySettingValues>& FactorySettings)
	{
		bool bCallConfigureProperties = true;

		for (int32 i = 0; i < FactorySettings.Num(); ++i)
		{
			if (FactorySettings[i].SettingName.Len() > 0 && FactorySettings[i].Value.Len() > 0)
			{
				//Check if we are setting an FBX import type override.  If we are, we don't want to call ConfigureProperties because that enables bDetectImportTypeOnImport
				if (FactorySettings[i].SettingName.Contains(TEXT("MeshTypeToImport")))
				{
					bCallConfigureProperties = false;
				}

				TArray<FString> PropertyChain;
				FactorySettings[i].SettingName.ParseIntoArray(&PropertyChain, TEXT("."), false);
				ApplyCustomFactorySetting(InFactory, PropertyChain, FactorySettings[i].Value);
			}
		}

		if (bCallConfigureProperties)
		{
			InFactory->ConfigureProperties();
		}
	}
}

///////////////////////////////////////////////////////////////////////
// Common Latent commands

//Latent Undo and Redo command
//If bUndo is true then the undo action will occur otherwise a redo will happen.
bool FUndoRedoCommand::Update()
{
	if (bUndo == true)
	{
		//Undo
		GEditor->UndoTransaction();
	}
	else
	{
		//Redo
		GEditor->RedoTransaction();
	}

	return true;
}

/**
* Open editor for a particular asset
*/
bool FOpenEditorForAssetCommand::Update()
{
	UObject* Object = StaticLoadObject(UObject::StaticClass(), NULL, *AssetName);
	if (Object)
	{
		FAssetEditorManager::Get().OpenEditorForAsset(Object);
		//This checks to see if the asset sub editor is loaded.
		if (FAssetEditorManager::Get().FindEditorForAsset(Object, true) != NULL)
		{
			UE_LOG(LogEditorAutomationTests, Log, TEXT("Verified asset editor for: %s."), *AssetName);
			UE_LOG(LogEditorAutomationTests, Display, TEXT("The editor successffully loaded for: %s."), *AssetName);
			return true;
		}
	}
	else
	{
		UE_LOG(LogEditorAutomationTests, Error, TEXT("Failed to find object: %s."), *AssetName);
	}
	return true;
}

/**
* Close all sub-editors
*/
bool FCloseAllAssetEditorsCommand::Update()
{
	FAssetEditorManager::Get().CloseAllAssetEditors();

	//Get all assets currently being tracked with open editors and make sure they are not still opened.
	if(FAssetEditorManager::Get().GetAllEditedAssets().Num() >= 1)
	{
		UE_LOG(LogEditorAutomationTests, Warning, TEXT("Not all of the editors were closed."));
		return true;
	}

	UE_LOG(LogEditorAutomationTests, Log, TEXT("Verified asset editors were closed"));
	UE_LOG(LogEditorAutomationTests, Display, TEXT("The asset editors closed successffully"));
	return true;
}

/**
* Start PIE session
*/
bool FStartPIECommand::Update()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
	TSharedPtr<class ILevelViewport> ActiveLevelViewport = LevelEditorModule.GetFirstActiveViewport();

	GUnrealEd->RequestPlaySession(false, ActiveLevelViewport, bSimulateInEditor, NULL, NULL, -1, false);
	return true;
}

/**
* End PlayMap session
*/
bool FEndPlayMapCommand::Update()
{
	GUnrealEd->RequestEndPlayMap();
	return true;
}

/**
* Generates a list of assets from the ENGINE and the GAME by a specific type.
* This is to be used by the GetTest() function.
*/
void FEditorAutomationTestUtilities::CollectTestsByClass(UClass * Class, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
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
void FEditorAutomationTestUtilities::CollectGameContentTestsByClass(UClass * Class, bool bRecursiveClass, TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
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
void FEditorAutomationTestUtilities::CollectMiscGameContentTestsByClass(TArray<FString>& OutBeautifiedNames, TArray <FString>& OutTestCommands)
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
	
	//if (ObjectList.Num() < 25000)
	//{

		//Loop through the list of assets, make their path full and a string, then add them to the test.
		for (auto ObjIter = ObjectList.CreateConstIterator(); ObjIter; ++ObjIter)
		{
			const FAssetData & Asset = *ObjIter;
			//First we check if the class is valid.  If not then we move onto the next object.
			if (Asset.GetClass() != NULL)
			{
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
	//}
}