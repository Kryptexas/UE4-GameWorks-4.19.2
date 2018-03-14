// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "USDPrimResolverKind.h"
#include "USDImporter.h"
#include "USDConversionUtils.h"
#include "AssetData.h"
#include "ModuleManager.h"
#include "AssetRegistryModule.h"
#include "USDSceneImportFactory.h"
#include "Engine/StaticMesh.h"
#include "UnrealUSDWrapper.h"


void UUSDPrimResolverKind::FindActorsToSpawn(FUSDSceneImportContext& ImportContext, TArray<FActorSpawnData>& OutActorSpawnData) const
{
	IUsdPrim* RootPrim = ImportContext.RootPrim;

	IUsdPrim* ParentPrim = nullptr;

	FindActorsToSpawn_Recursive(ImportContext, RootPrim, ParentPrim, OutActorSpawnData);
}

void UUSDPrimResolverKind::FindActorsToSpawn_Recursive(FUSDSceneImportContext& ImportContext, IUsdPrim* Prim, IUsdPrim* ParentPrim, TArray<FActorSpawnData>& OutSpawnDatas) const
{
	FName PrimName = USDToUnreal::ConvertName(Prim->GetPrimName());

	UUSDSceneImportOptions* ImportOptions = Cast<UUSDSceneImportOptions>(ImportContext.ImportOptions);

	// Parent/child hierarchy will be ignored unless the kind is a group.  Keep track of the current parent and use it 
	IUsdPrim* GroupParent = ParentPrim;

	if (Prim->IsKindChildOf(USDKindTypes::Component))
	{
		bool bHasUnrealAssetPath = Prim->GetUnrealAssetPath() != nullptr;
		bool bHasUnrealActorClass = Prim->GetUnrealActorClass() != nullptr;

		FActorSpawnData SpawnData;

		if (bHasUnrealActorClass)
		{
			SpawnData.ActorClassName = USDToUnreal::ConvertString(Prim->GetUnrealActorClass());

			UE_LOG(LogUSDImport, Log, TEXT("Adding %s Actor with custom actor class to spawn"), *PrimName.ToString());
		}
		else if(bHasUnrealAssetPath)
		{
			SpawnData.AssetPath = USDToUnreal::ConvertString(Prim->GetUnrealAssetPath());
			FAssetData AssetData = AssetRegistry->GetAssetByObjectPath(FName(*SpawnData.AssetPath));
			if ( !AssetData.IsValid() || AssetData.AssetClass == UStaticMesh::StaticClass()->GetFName() )
			{
				// If the object is a static mesh allow it to be imported.  Import settings may override this though
				// Find the asset associated with this object that should be imported
				FindMeshAssetsToImport(ImportContext, Prim, SpawnData.AssetsToImport);
			}

			UE_LOG(LogUSDImport, Log, TEXT("Adding %s Actor with custom asset path to spawn"), *PrimName.ToString());
		}
		else 
		{
			FindMeshAssetsToImport(ImportContext, Prim, SpawnData.AssetsToImport);

			UE_LOG(LogUSDImport, Log, TEXT("Adding %s Actor with %d meshes"), *PrimName.ToString(), SpawnData.AssetsToImport.Num());
		}

		SpawnData.WorldTransform = USDToUnreal::ConvertMatrix(Prim->GetLocalToWorldTransform());
		SpawnData.ActorPrim = Prim;
		SpawnData.ActorName = PrimName;
		SpawnData.AttachParentPrim = GroupParent;

		if (ImportOptions->ExistingActorPolicy == EExistingActorPolicy::Replace && ImportContext.ExistingActors.Contains(SpawnData.ActorName))
		{
			ImportContext.ActorsToDestroy.Add(SpawnData.ActorName);
		}

		OutSpawnDatas.Add(SpawnData);

	
	}
	else if (Prim->IsKindChildOf(USDKindTypes::Group))
	{
		// Blank actor for group prims
		FActorSpawnData SpawnData;

		SpawnData.ActorPrim = Prim;
		SpawnData.ActorName = PrimName;

		SpawnData.WorldTransform = USDToUnreal::ConvertMatrix(Prim->GetLocalToWorldTransform());
		SpawnData.AttachParentPrim = GroupParent;

		// New parent of all children is this prim
		GroupParent = Prim;
		OutSpawnDatas.Add(SpawnData);

		if (ImportOptions->ExistingActorPolicy == EExistingActorPolicy::Replace && ImportContext.ExistingActors.Contains(SpawnData.ActorName))
		{
			ImportContext.ActorsToDestroy.Add(SpawnData.ActorName);
		}

		UE_LOG(LogUSDImport, Log, TEXT("Adding %s Group Actor to spawn"), *PrimName.ToString());
	}

	int32 NumChildren = Prim->GetNumChildren();
	for (int32 ChildIdx = 0; ChildIdx < NumChildren; ++ChildIdx)
	{
		FindActorsToSpawn_Recursive(ImportContext, Prim->GetChild(ChildIdx), GroupParent, OutSpawnDatas);
	}
}