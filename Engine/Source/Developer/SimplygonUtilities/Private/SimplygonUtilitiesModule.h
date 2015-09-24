#pragma once

#include "SimplygonUtilities.h"

class IMaterialLODSettingsLayout;

/************************************************************************/
/* Simplygon Utilities Module                                           */
/************************************************************************/

class FSimplygonUtilities : public ISimplygonUtilities
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	virtual FMeshMaterialReductionData* CreateMeshReductionData(FRawMesh* InMesh, bool InReleaseMesh) override;

	virtual bool ExportMaterial(
		UMaterialInterface* InMaterial,
		FFlattenMaterial& OutFlattenMaterial) override;

	virtual bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FRawMesh* InMesh,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		FFlattenMaterial& OutFlattenMaterial) override;

	virtual bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FStaticLODModel* InMesh,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		FFlattenMaterial& OutFlattenMaterial) override;

	virtual void GetTextureCoordinateBoundsForRawMesh(
		const FRawMesh& InRawMesh,
		TArray<FBox2D>& OutBounds) override;

	virtual void GetTextureCoordinateBoundsForSkeletalMesh(
		const FStaticLODModel& LODModel,
		TArray<FBox2D>& OutBounds) override;

	virtual void FullyLoadMaterial(UMaterialInterface* Material) override;
	virtual void PurgeMaterialTextures(UMaterialInterface* Material) override;
	virtual void SaveMaterial(UMaterialInterface* Material) override;
};
