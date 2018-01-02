// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once 

class UStaticMesh;
struct FUsdImportContext;
struct FUsdAssetPrimToImport;
struct FUsdGeomData;

class FUSDStaticMeshImporter
{
public:
	static UStaticMesh* ImportStaticMesh(FUsdImportContext& ImportContext, const FUsdAssetPrimToImport& PrimToImport);
private:
	static bool IsTriangleMesh(const FUsdGeomData* GeomData);
};