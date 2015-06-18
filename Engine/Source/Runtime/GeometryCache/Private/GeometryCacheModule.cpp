// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "GeometryCacheModulePrivatePCH.h"
#include "AssetToolsModule.h"
#include "AssetTypeActions_GeometryCache.h"

#include "GeometryCacheModule.h"

IMPLEMENT_MODULE(FGeometryCacheModule, GeometryCache)

void FGeometryCacheModule::StartupModule()
{
	// This code will execute after your module is loaded into memory (but after global variables are initialized, of course.)

	FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();

	IAssetTools& AssetTools = AssetToolsModule.Get();

	AssetTools.RegisterAssetTypeActions(MakeShareable(new FAssetTypeActions_GeometryCache));

	// Register to be notified when an asset is reimported
	OnAssetReimportDelegateHandle = FEditorDelegates::OnAssetReimport.AddRaw(this, &FGeometryCacheModule::OnObjectReimported);

}

void FGeometryCacheModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FGeometryCacheModule::OnObjectReimported(UObject* InObject)
{
	if (UGeometryCache* GeometryCache = Cast<UGeometryCache>(InObject))
	{
		for (TObjectIterator<UGeometryCacheComponent> CacheIt; CacheIt; ++CacheIt)
		{
			CacheIt->OnObjectReimported(GeometryCache);
		}
	}
}
