// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ClothingAsset.h"
#include "ClothingAssetFactoryInterface.h"
#include "GPUSkinPublicDefs.h"
#include "GenericOctree.h"

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
	virtual UClothingAssetBase* CreateFromSkeletalMesh(USkeletalMesh* TargetMesh, FSkeletalMeshClothBuildParams& Params) override;
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

	// Octree definitions for auto fixing when creating a clothing asset from a skeletal mesh
	struct FQueryTri
	{
		uint32 Id;
		uint32 SectionIndex;
		FVector Vertices[3];
		FBoxCenterAndExtent Bounds;

		bool operator==(const FQueryTri& Other) const
		{
			return Id == Other.Id && SectionIndex == Other.SectionIndex;
		}
	};

	struct FQueryVertexOctreeSemantics
	{
		enum { MaxElementsPerLeaf = 16 };
		enum { MinInclusiveElementsPerNode = 7 };
		enum { MaxNodeDepth = 12 };

		typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

		FORCEINLINE static FBoxCenterAndExtent GetBoundingBox(const FQueryTri& Element)
		{
			return Element.Bounds;
		}

		FORCEINLINE static bool AreElementsEqual(const FQueryTri& First, const FQueryTri& Second)
		{
			return First == Second;
		}

		FORCEINLINE static void SetElementId(const FQueryTri& Element, FOctreeElementId Id)
		{}
	};
	typedef TOctree<FQueryTri, FQueryVertexOctreeSemantics> FQueryTriOctree;

	// Utility methods for skeletal mesh extraction //////////////////////////

	/** 
	 * Using a physics asset, extract spheres and capsules and apply them to the provided collision container
	 * @param InPhysicsAsset The asset to pull body information from
	 * @param TargetMesh The mesh we are targetting
	 * @param TargetClothingAsset The clothing asset we are targetting
	 * @param OutCollisionData The collision container to fill
	 */
	void ExtractPhysicsAssetBodies(UPhysicsAsset* InPhysicsAsset, USkeletalMesh* TargetMesh, UClothingAsset* TargetClothingAsset, FClothCollisionData& OutCollisionData);

	void PopulateAutoFixOctree(FQueryTriOctree& OutOctree, const FStaticLODModel& InLodModel, int32 InExcludeSectionIndex = INDEX_NONE);

	//////////////////////////////////////////////////////////////////////////

};