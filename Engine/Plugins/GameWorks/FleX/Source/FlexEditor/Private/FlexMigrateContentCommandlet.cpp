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
	bool bSuccess = false;

	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("FlexMigrateContentCommandlet: Params = %s"), *Params);
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

	FARFilter Filter;
	Filter.ClassNames.Add(UStaticMesh::StaticClass()->GetFName());
	Filter.PackagePaths.Add("/Game");
	Filter.bRecursivePaths = true;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	for (auto AssetIt = AssetList.CreateConstIterator(); AssetIt; ++AssetIt)
	{
		const FAssetData& AssetData = *AssetIt;

		UStaticMesh* StaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
		if (StaticMesh && StaticMesh->FlexAsset_DEPRECATED)
		{
			if (MigrateStaticMesh(StaticMesh) == false)
			{
				UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("failed to migrate StaticMesh: %s"), *StaticMesh->GetFullName());
			}
		}
	}

	FPlatformProcess::Sleep(0.005f);

	return bSuccess ? 0 : 1;
}

bool UFlexMigrateContentCommandlet::MigrateStaticMesh(UStaticMesh* StaticMesh)
{
	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("MigrateStaticMesh: %s"), *StaticMesh->GetFullName());

	UPackage* Package = StaticMesh->GetOutermost();

	const FString PackageName = Package->GetName();
	const FString ObjectName = StaticMesh->GetName();

	FString TempPackageName = FString::Printf(TEXT("/Temp/%s"), *ObjectName);
	UPackage* TempPackage = CreatePackage(nullptr, *TempPackageName);

	if (StaticMesh->Rename(nullptr, TempPackage, REN_DontCreateRedirectors) == false)
		return false;

	UFlexStaticMesh* FSM = Cast<UFlexStaticMesh>(StaticDuplicateObject(StaticMesh, Package, NAME_None, RF_AllFlags, UFlexStaticMesh::StaticClass()));
	if (FSM == nullptr)
		return false;

	FSM->FlexAsset = Cast<UFlexAsset>(StaticMesh->FlexAsset_DEPRECATED);
	StaticMesh->FlexAsset_DEPRECATED = nullptr;
	if (FSM->FlexAsset)
	{
		FSM->FlexAsset->Rename(nullptr, FSM, REN_DontCreateRedirectors);
	}

	UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("AFTER: Objects in package %s:"), *Package->GetFullName());
	for (FObjectIterator It; It; ++It)
	{
		UObject* Object = *It;
		if (Object->IsIn(Package))
		{
			UE_LOG(LogFlexMigrateContentCommandlet, Log, TEXT("%s of %s"), *Object->GetFullName(), *Object->GetOuter()->GetName());
		}
	}

	// save package
	FString const PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	//return UPackage::SavePackage(Package, nullptr, RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
	return GEditor->SavePackage(Package, nullptr, RF_Standalone, *PackageFileName, GError, nullptr, false, true, SAVE_NoError);
}
