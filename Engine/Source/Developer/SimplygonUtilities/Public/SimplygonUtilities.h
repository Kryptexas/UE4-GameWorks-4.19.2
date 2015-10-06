// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleManager.h"
#include "MaterialUtilities.h"
#include "Editor/PropertyEditor/Public/IDetailCustomNodeBuilder.h"
#include "SimplygonTypes.h"

/**
 * All data used for simplification of single StaticMesh pocked into a single structure.
 * This structure has constructor/destructor implemented inside SimplygonUtilities module, so
 * it should be allocated with ISimplygonUtilities::CreateMeshReductionData().
 */
struct FMeshMaterialReductionData
{
	// Source mesh data. Either 'Mesh' or 'LODModel' (but not both) should be non-null.
	bool bReleaseMesh;
	struct FRawMesh* Mesh;
	FStaticLODModel* LODModel;
	UStaticMesh* StaticMesh;

	// Texture coordinate data
	TArray<FBox2D> TexcoordBounds; //! remove
	TArray<FVector2D> NewUVs;

	// Input material data
	TIndirectArray<FFlattenMaterial> FlattenMaterials;
	TArray<UMaterialInterface*> NonFlattenMaterials;

	// Output material data
	/*
	* Maps index of input material to index of output material.
	*/
	TArray<int32> OutputMaterialMap;
	/*
	* Blend modes for each output material.
	*/
	TArray<EBlendMode> OutputBlendMode;
	/*
	* TwoSided property for each output material.
	*/
	TArray<bool> OutputTwoSided;
	/*
	* Emissive color scale for each output material.
	*/
	TArray<float> OutputEmissiveScale;

	/*
	* Analyze blend modes of all materials and build OutputMaterialMap and OutputBlendMode.
	*/
	virtual void BuildOutputMaterialMap(const FSimplygonMaterialLODSettings& MaterialLODSettings, bool bAllowMultiMaterial);

	/*
	* Convert NonFlattenMaterials to FlattenMaterials.
	*/
	virtual void BuildFlattenMaterials(const FSimplygonMaterialLODSettings& MaterialLODSettings, int32 TextureWidth, int32 TextureHeight);

	/*
	* Determine maximal emissive level and adjust. Called automatically by BuildFlattenMaterials().
	*/
	virtual void AdjustEmissiveChannels();

	FORCEINLINE int32 GetInputMaterialCount() const
	{
		return NonFlattenMaterials.Num();
	}

	FORCEINLINE int32 GetOutputMaterialCount() const
	{
		return OutputBlendMode.Num();
	}

	// Constructor and destructor
	FMeshMaterialReductionData();
	virtual ~FMeshMaterialReductionData();

	FORCEINLINE void Setup(FRawMesh* InMesh, bool InReleaseMesh = false)
	{
		Mesh = InMesh;
		bReleaseMesh = InReleaseMesh;
	}

	FORCEINLINE void Setup(FStaticLODModel* InLODModel)
	{
		LODModel = InLODModel;
	}
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

	virtual FMeshMaterialReductionData* NewMeshReductionData() = 0;

	FORCEINLINE FMeshMaterialReductionData* CreateMeshReductionData(FRawMesh* InMesh, bool InReleaseMesh = false)
	{
		FMeshMaterialReductionData* Data = NewMeshReductionData();
		Data->Setup(InMesh, InReleaseMesh);
		return Data;
	}

	virtual void GetTextureCoordinateBoundsForRawMesh( const FRawMesh& InRawMesh, TArray<FBox2D>& OutBounds) = 0;
	virtual void GetTextureCoordinateBoundsForSkeletalMesh(const FStaticLODModel& LODModel, TArray<FBox2D>& OutBounds) = 0;
	
	virtual void FullyLoadMaterial(UMaterialInterface* Material) = 0;
	virtual void PurgeMaterialTextures(UMaterialInterface* Material) = 0;
	virtual void SaveMaterial(UMaterialInterface* Material) = 0;
};
