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

	virtual FMeshMaterialReductionData* NewMeshReductionData() override;
	
	virtual void GetTextureCoordinateBoundsForRawMesh(
		const FRawMesh& InRawMesh,
		TArray<FBox2D>& OutBounds) override;
		
	SIMPLYGONUTILITIES_API static void AnalyzeMaterial(
		UMaterialInterface* InMaterial,
		const FSimplygonMaterialLODSettings& InMaterialLODSettings,
		int32& OutNumTexCoords,
		bool& OutUseVertexColor);

	virtual void GetTextureCoordinateBoundsForSkeletalMesh(
		const FStaticLODModel& LODModel,
		TArray<FBox2D>& OutBounds) override;


	virtual void FullyLoadMaterial(UMaterialInterface* Material) override;
	virtual void PurgeMaterialTextures(UMaterialInterface* Material) override;
	virtual void SaveMaterial(UMaterialInterface* Material) override;

	SIMPLYGONUTILITIES_API static FFlattenMaterial* CreateFlattenMaterial(
		const FSimplygonMaterialLODSettings& InMaterialLODSettings,
		int32 InTextureWidth,
		int32 InTextureHeight);
	
	/**
	* Convert Unreal material to FFlatterMaterial using simple square renderer.
	*/
	SIMPLYGONUTILITIES_API static bool ExportMaterial(
		UMaterialInterface* InMaterial,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

	/**
	* Convert Unreal material to FFlattenMaterial using StaticMesh (FRawMesh) geometry for rendering more detailed materials.
	*/
	SIMPLYGONUTILITIES_API static bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FRawMesh* InMesh,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

	/**
	* Convert Unreal material to FFlattenMaterial using SkeletalMesh (FStaticLODModel) geometry for rendering more detailed materials.
	*/
	SIMPLYGONUTILITIES_API static bool ExportMaterial(
		UMaterialInterface* InMaterial,
		const FStaticLODModel* InMesh,
		int32 InMaterialIndex,
		const FBox2D& InTexcoordBounds,
		const TArray<FVector2D>& InTexCoords,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

private:
	SIMPLYGONUTILITIES_API static bool ExportMaterial(
		struct FMaterialData& InMaterialData,
		FFlattenMaterial& OutFlattenMaterial,
		FExportMaterialProxyCache* CompiledMaterial = nullptr);

	SIMPLYGONUTILITIES_API static bool RenderMaterialPropertyToTexture(
		struct FMaterialData& InMaterialData,
		bool bInCompileOnly,
		EMaterialProperty InMaterialProperty,
		bool bInForceLinearGamma,
		EPixelFormat InPixelFormat,
		FIntPoint& InTargetSize,
		TArray<FColor>& OutSamples);

	SIMPLYGONUTILITIES_API static void FullyLoadMaterialStatic(UMaterialInterface* Material);

	void OnPreGarbageCollect();

	static UTextureRenderTarget2D* CreateRenderTarget(bool bInForceLinearGamma, EPixelFormat InPixelFormat, FIntPoint& InTargetSize);

	static void ClearRTPool();

private:
	static bool CurrentlyRendering;
	static TArray<UTextureRenderTarget2D*> RTPool;
};
