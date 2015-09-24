// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "MaterialUtilities.h"

/**
 * All data used for simplification of single StaticMesh pocked into a single structure.
 * This structure has constructor/destructor implemented inside SimplygonUtilities module, so
 * it should be allocated with ISimplygonUtilities::CreateMeshReductionData().
 */
struct FMeshMaterialReductionData
{
	bool bReleaseMesh;
	struct FRawMesh* Mesh;
	TIndirectArray<FFlattenMaterial> FlattenMaterials;
	TArray<UMaterialInterface*> NonFlatternMaterials;
	TArray<FBox2D> TexcoordBounds; //! remove
	TArray<FVector2D> NewUVs;

	FMeshMaterialReductionData(FRawMesh* InMesh, bool InReleaseMesh = false);
	virtual ~FMeshMaterialReductionData();
};

/**
 * The public interface to this module
 */
class ISimplygonUtilities : public IModuleInterface
{

public:

	/**
	 * Singleton-like access to this module's interface.  This is just for convenience!
	 * Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	 *
	 * @return Returns singleton instance, loading the module on demand if needed
	 */
	static inline ISimplygonUtilities& Get()
	{
		return FModuleManager::LoadModuleChecked< ISimplygonUtilities >( "SimplygonUtilities" );
	}

	/**
	 * Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	 *
	 * @return True if the module is loaded and ready to use
	 */
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded( "SimplygonUtilities" );
	}

	virtual FMeshMaterialReductionData* CreateMeshReductionData(FRawMesh* InMesh, bool InReleaseMesh = false) = 0;

	/**
	 * Convert Unreal material to FFlatterMaterial using simple square renderer.
	 */
	virtual bool ExportMaterial(
		UMaterialInterface* InMaterial,
		FFlattenMaterial& OutFlattenMaterial) = 0;

	/**
	 * Convert Unreal material to FFlattenMaterial using StaticMesh (FRawMesh) geometry for rendering more detailed materials.
	 */
	virtual bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FRawMesh* InMesh,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		FFlattenMaterial& OutFlattenMaterial) = 0;

	/**
	 * Convert Unreal material to FFlattenMaterial using SkeletalMesh (FStaticLODModel) geometry for rendering more detailed materials.
	 */
	virtual bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FStaticLODModel* InMesh,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		FFlattenMaterial& OutFlattenMaterial) = 0;

	virtual void GetTextureCoordinateBoundsForRawMesh( const FRawMesh& InRawMesh, TArray<FBox2D>& OutBounds) = 0;
	virtual void GetTextureCoordinateBoundsForSkeletalMesh(const FStaticLODModel& LODModel, TArray<FBox2D>& OutBounds) = 0;
	
	virtual void FullyLoadMaterial(UMaterialInterface* Material) = 0;
	virtual void PurgeMaterialTextures(UMaterialInterface* Material) = 0;
	virtual void SaveMaterial(UMaterialInterface* Material) = 0;
};
