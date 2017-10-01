// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "FlexMigrateContentCommandlet.h"
#include "Misc/Paths.h"
#include "FileHelpers.h"
#include "AssetRegistryModule.h"
#include "Engine/World.h"

#include "Engine/StaticMesh.h"
#include "FlexStaticMesh.h"
#include "FlexAsset.h"

#include "Editor.h"

#include "Serialization/FindReferencersArchive.h"
#include "Serialization/ArchiveReplaceObjectRef.h"


DEFINE_LOG_CATEGORY_STATIC(LogFlexMigrateContentCommandlet, Log, All);


/**
 * UFlexMigrateContentCommandlet
 *
 * Commandlet used to migrate content from non-plugin Flex UE4 integration
 */

UFlexMigrateContentCommandlet::UFlexMigrateContentCommandlet(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

int32 UFlexMigrateContentCommandlet::Main(const FString& Params)
{
	bool bSuccess = true;

	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("FlexMigrateContentCommandlet: ProjectFilePath = '%s'"), *FPaths::GetProjectFilePath());

#if 0
	TArray<FString> AllPackageFilenames;
	FEditorFileUtils::FindAllPackageFiles(AllPackageFilenames);
	for (int32 PackageIndex = 0; PackageIndex < AllPackageFilenames.Num(); PackageIndex++)
	{
		const FString& Filename = AllPackageFilenames[PackageIndex];

		if (FPaths::GetExtension(Filename, true) == FPackageName::GetMapPackageExtension())
		{
			UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("FlexMigrateContentCommandlet: Map = '%s'"), *Filename);
		}
	}
#endif
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

	// Update Registry Module
	AssetRegistryModule.Get().SearchAllAssets(true);

	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssetsByPath(FName("/Game"), AssetList, true);

	// load all assets
	for (auto AssetIt = AssetList.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& AssetData = *AssetIt;
		UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("FlexMigrateContentCommandlet: load Asset - %s"), *AssetData.GetFullName());

		AssetData.GetPackage()->FullyLoad();
	}

	TMap<UObject*, UObject*> ReplacementMap;
	TArray<UPackage*> DirtiedPackages;

	// find StaticMeshes with FlexAsset property
	for (auto AssetIt = AssetList.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& AssetData = *AssetIt;
		if (AssetData.AssetClass == UStaticMesh::StaticClass()->GetFName())
		{
			UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
			if (StaticMesh && StaticMesh->FlexAsset_DEPRECATED)
			{
				UObject* NewObject = MigrateStaticMesh(StaticMesh, DirtiedPackages);
				if (NewObject == nullptr)
				{
					UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("failed to migrate StaticMesh: %s"), *StaticMesh->GetFullName());
					bSuccess = false;
					break;
				}

				ReplacementMap.Add(StaticMesh, NewObject);
			}
		}
	}

	if (bSuccess && ReplacementMap.Num() > 0)
	{
		ForceReplaceReferences(ReplacementMap, DirtiedPackages);
	}

	// save all dirtied packages
	for (auto PackageIt = DirtiedPackages.CreateConstIterator(); PackageIt; ++PackageIt)
	{
		UPackage* Package = *PackageIt;
		check(Package->FileName != NAME_None);

		const FString PackageName = Package->FileName.ToString();
		const FString PackageExtension = Package->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
		const FString PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, PackageExtension);

		UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("Saving Package '%s' to file '%s'"), *PackageName, *PackageFilename);

		//return UPackage::SavePackage(Package, nullptr, RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
		GEditor->SavePackage(Package, nullptr, RF_Standalone, *PackageFilename, GError, nullptr, false, true, SAVE_NoError);
	}

	return bSuccess ? 0 : 1;
}

UObject* UFlexMigrateContentCommandlet::MigrateStaticMesh(UStaticMesh* StaticMesh, TArray<UPackage*>& DirtiedPackages)
{
	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("MigrateStaticMesh: %s"), *StaticMesh->GetFullName());

	UPackage* Package = StaticMesh->GetOutermost();

	UPackage* TempPackage = GetTransientPackage();
	FString TempObjectName = MakeUniqueObjectName(TempPackage, UStaticMesh::StaticClass(), StaticMesh->GetFName()).ToString();

	if (StaticMesh->Rename(*TempObjectName, TempPackage, REN_DontCreateRedirectors) == false)
		return nullptr;

	UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticDuplicateObject(StaticMesh, Package, NAME_None, RF_AllFlags, UFlexStaticMesh::StaticClass()));
	if (FSM == nullptr)
		return nullptr;

	FSM->FlexAsset = Cast<UFlexAsset>(StaticMesh->FlexAsset_DEPRECATED);
	StaticMesh->FlexAsset_DEPRECATED = nullptr;
	if (FSM->FlexAsset)
	{
		FSM->FlexAsset->Rename(nullptr, FSM, REN_DontCreateRedirectors);
	}
#if 0
	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("AFTER: Objects in package %s:"), *Package->GetFullName());
	for (FObjectIterator It; It; ++It)
	{
		UObject* Object = *It;
		if (Object->IsIn(Package))
		{
			UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("%s of %s"), *Object->GetFullName(), *Object->GetOuter()->GetName());
		}
	}
#endif

	Package->SetDirtyFlag(true);
	DirtiedPackages.AddUnique(Package);

	return FSM;
}

bool UFlexMigrateContentCommandlet::ForceReplaceReferences(const TMap<UObject*, UObject*>& ReplacementMap, TArray<UPackage*>& DirtiedPackages)
{
	TArray<UObject*> ReplaceableObjects;
	ReplacementMap.GenerateKeyArray(ReplaceableObjects);

	// Find all the properties (and their corresponding objects) that refer to any of the objects to be replaced
	TMap< UObject*, TArray<UProperty*> > ReferencingPropertiesMap;
	for (FObjectIterator ObjIter; ObjIter; ++ObjIter)
	{
		UObject* CurObject = *ObjIter;

		// Unless the "object to replace with" is null, ignore any of the objects to replace to themselves
		if (ReplacementMap.Find(CurObject) == nullptr)
		{
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
				if (CurReferencedProperties.Num() > 0)
				{
					UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("Object %s has replacable properties:"), *CurObject->GetFullName());

					for (TArray<UProperty*>::TConstIterator RefPropIter(CurReferencedProperties); RefPropIter; ++RefPropIter)
					{
						UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("== %s"), *(*RefPropIter)->GetFullName());
						CurObject->PreEditChange(*RefPropIter);
					}
				}
				else
				{
					CurObject->PreEditChange(nullptr);
				}
			}
		}
	}

	// Iterate over the map of referencing objects/changed properties, forcefully replacing the references and
	for (TMap< UObject*, TArray<UProperty*> >::TConstIterator MapIter(ReferencingPropertiesMap); MapIter; ++MapIter)
	{
		UObject* CurReplaceObj = MapIter.Key();

		FArchiveReplaceObjectRef<UObject> ReplaceAr(CurReplaceObj, ReplacementMap, false, true, false);
	}

	// Now alter the referencing objects the change has completed via PostEditChange, 
	// this is done in a separate loop to prevent reading of data that we want to overwrite
	for (TMap< UObject*, TArray<UProperty*> >::TConstIterator MapIter(ReferencingPropertiesMap); MapIter; ++MapIter)
	{
		UObject* CurReplaceObj = MapIter.Key();
		const TArray<UProperty*>& RefPropArray = MapIter.Value();

		if (RefPropArray.Num() > 0)
		{
			for (TArray<UProperty*>::TConstIterator RefPropIter(RefPropArray); RefPropIter; ++RefPropIter)
			{
				FPropertyChangedEvent PropertyEvent(*RefPropIter, EPropertyChangeType::Redirected);
				CurReplaceObj->PostEditChangeProperty(PropertyEvent);
			}
		}
		else
		{
			FPropertyChangedEvent PropertyEvent(nullptr, EPropertyChangeType::Redirected);
			CurReplaceObj->PostEditChangeProperty(PropertyEvent);
		}

		if (!CurReplaceObj->HasAnyFlags(RF_Transient) && CurReplaceObj->GetOutermost() != GetTransientPackage())
		{
			if (!CurReplaceObj->RootPackageHasAnyFlags(PKG_CompiledIn))
			{
				CurReplaceObj->MarkPackageDirty();
				DirtiedPackages.AddUnique(CurReplaceObj->GetOutermost());
			}
		}
	}

	return true;
}
