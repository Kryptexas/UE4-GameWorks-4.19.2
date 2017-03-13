// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ClothingAsset.h"
#include "ClothingAssetFactoryInterface.h"
#include "GPUSkinPublicDefs.h"

#include "ClothingAssetFactory.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogClothingAssetFactory, Log, All);

namespace nvidia
{
	namespace apex
	{
		class ClothingAsset;
	}
}

namespace NvParameterized
{
	class Interface;
}

UCLASS(hidecategories=Object)
class CLOTHINGSYSTEMEDITOR_API UClothingAssetFactory : public UClothingAssetFactoryBase
{
	GENERATED_BODY()

public:

	UClothingAssetFactory(const FObjectInitializer& ObjectInitializer);

	// Import the given file, treating it as an APEX asset file and return the resulting asset
	virtual UClothingAssetBase* Import(const FString& Filename, USkeletalMesh* TargetMesh, FName InName = NAME_None) override;
	virtual UClothingAssetBase* Reimport(const FString& Filename, USkeletalMesh* TargetMesh, UClothingAssetBase* OriginalAsset) override;
	// Tests whether the given filename should be able to be imported
	virtual bool CanImport(const FString& Filename) override;

	// Given an APEX asset, build a UClothingAsset containing the required data
	virtual UClothingAssetBase* CreateFromApexAsset(nvidia::apex::ClothingAsset* InApexAsset, USkeletalMesh* TargetMesh, FName InName = NAME_None) override;

private:

#if WITH_APEX_CLOTHING

	struct FApexVertData
	{
		uint16 BoneIndices[MAX_TOTAL_INFLUENCES];
	};

	// Convert from APEX to UE coodinate system
	nvidia::apex::ClothingAsset* ConvertApexAssetCoordSystem(nvidia::apex::ClothingAsset* InAsset);

	// Convert APEX UV direction to UE UV direction
	void FlipAuthoringUvs(NvParameterized::Interface* InRenderMeshAuthoringInterface, bool bFlipU, bool bFlipV);

	// Extraction methods for pulling the required data from an APEX asset and
	// pushing it to a UClothingAsset
	void ExtractLodPhysicalData(UClothingAsset* NewAsset, nvidia::apex::ClothingAsset &InApexAsset, int32 InLodIdx, FClothLODData &InLodData, TArray<FApexVertData>& OutApexVertData);
	void ExtractBoneData(UClothingAsset* NewAsset, nvidia::apex::ClothingAsset &InApexAsset);
	void ExtractSphereCollisions(UClothingAsset* NewAsset, nvidia::apex::ClothingAsset &InApexAsset, int32 InLodIdx, FClothLODData &InLodData);
	void ExtractMaterialParameters(UClothingAsset* NewAsset, nvidia::apex::ClothingAsset &InApexAsset);
#endif

};