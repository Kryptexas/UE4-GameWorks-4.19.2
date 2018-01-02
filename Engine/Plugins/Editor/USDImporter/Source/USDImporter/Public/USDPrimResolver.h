// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "CoreMinimal.h"
#include "SubclassOf.h"
#include "USDPrimResolver.generated.h"

class IUsdPrim;
class IAssetRegistry;
class UActorFactory;
struct FUsdImportContext;
struct FUsdGeomData;
struct FUSDSceneImportContext;

struct FUsdAssetPrimToImport
{
	FUsdAssetPrimToImport()
		: Prim(nullptr)
		, NumLODs(1)
		, CustomPrimTransform(FMatrix::Identity)
	{}

	/** The prim that represents the root most prim of the mesh asset being created */
	IUsdPrim* Prim;
	/** Each prim in this list represents a list of prims which have LODs at a specific lod index */
	TArray<IUsdPrim*> MeshPrims;
	int32 NumLODs;
	FMatrix CustomPrimTransform;
	FString AssetPath;
};

struct FActorSpawnData
{
	FActorSpawnData()
		: ActorPrim(nullptr)
		, AttachParentPrim(nullptr)
	{}

	FMatrix WorldTransform;
	/** The prim that represents this actor */
	IUsdPrim* ActorPrim;
	/** The prim that represents the parent of this actor for attachment (not necessarily the parent of this prim) */
	IUsdPrim* AttachParentPrim;
	/** List of assets under this actor to create */
	TArray<FUsdAssetPrimToImport> AssetsToImport;
	FString ActorClassName;
	FString AssetPath;
	FName ActorName;
};

/** Base class for all evaluation of prims for geometry and actors */
UCLASS(transient, MinimalAPI)
class UUSDPrimResolver : public UObject
{
	GENERATED_BODY()

public:
	virtual void Init();

	virtual void FindMeshAssetsToImport(FUsdImportContext& ImportContext, IUsdPrim* StartPrim, TArray<FUsdAssetPrimToImport>& OutAssetsToImport, bool bRecursive = true) const;

	/**
	 * Finds any mesh children of a parent prim
	 *
	 * @param ImportContext		Contextual information about the current import
	 * @param ParentPrim		The parent prim to find children from
	 * @param bOnlyLODRoots		Only return prims which are parents of LOD meshes (i.e the prim has an LOD variant set)
	 * @param OutMeshChilden	Flattened list of descendant prims with geometry
	 */
	virtual void FindMeshChildren(FUsdImportContext& ImportContext, IUsdPrim* ParentPrim, bool bOnlyLODRoots, TArray<IUsdPrim*>& OutMeshChildren) const;

	virtual void FindActorsToSpawn(FUSDSceneImportContext& ImportContext, TArray<FActorSpawnData>& OutActorSpawnDatas) const;

	
	virtual AActor* SpawnActor(FUSDSceneImportContext& ImportContext, const FActorSpawnData& SpawnData);

	virtual TSubclassOf<AActor> FindActorClass(FUSDSceneImportContext& ImportContext, const FActorSpawnData& SpawnData) const;

protected:
	virtual void FindActorsToSpawn_Recursive(FUSDSceneImportContext& ImportContext, IUsdPrim* Prim, IUsdPrim* ParentPrim, TArray<FActorSpawnData>& OutSpawnDatas) const;
	bool IsValidPathForImporting(const FString& TestPath) const;
protected:
	IAssetRegistry* AssetRegistry;
	TMap<IUsdPrim*, AActor*> PrimToActorMap;
};