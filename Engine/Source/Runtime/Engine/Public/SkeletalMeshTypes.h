// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PrimitiveSceneProxy.h"
#include "Materials/MaterialInterface.h"

class FMaterialRenderProxy;
class FMeshElementCollector;
class FPrimitiveDrawInterface;
class FRawStaticIndexBuffer16or32Interface;
class UMorphTarget;
class UPrimitiveComponent;
class USkeletalMesh;
class USkinnedMeshComponent;
class USkeletalMesh;
class FSkeletalMeshRenderData;
class FSkeletalMeshLODRenderData;

// Custom serialization version for SkeletalMesh types
struct FSkeletalMeshCustomVersion
{
	enum Type
	{
		// Before any version changes were made
		BeforeCustomVersionWasAdded = 0,
		// Remove Chunks array in FStaticLODModel and combine with Sections array
		CombineSectionWithChunk = 1,
		// Remove FRigidSkinVertex and combine with FSoftSkinVertex array
		CombineSoftAndRigidVerts = 2,
		// Need to recalc max bone influences
		RecalcMaxBoneInfluences = 3,
		// Add NumVertices that can be accessed when stripping editor data
		SaveNumVertices = 4,
		// Regenerated clothing section shadow flags from source sections
		RegenerateClothingShadowFlags = 5,
		// Share color buffer structure with StaticMesh
		UseSharedColorBufferFormat = 6,
		// Use separate buffer for skin weights
		UseSeparateSkinWeightBuffer = 7,
		// Added new clothing systems
		NewClothingSystemAdded = 8,
		// Cached inv mass data for clothing assets
		CachedClothInverseMasses = 9,
		// Compact cloth vertex buffer, without dummy entries
		CompactClothVertexBuffer = 10,
		// Remove SourceData
		RemoveSourceData = 11,
		// Split data into Model and RenderData
		SplitModelAndRenderData = 12,
		// Remove triangle sorting support
		RemoveTriangleSorting = 13,
		// Remove the duplicated clothing sections that were a legacy holdover from when we didn't use our own render data
		RemoveDuplicatedClothingSections = 14,
		// Remove 'Disabled' flag from SkelMesh asset sections
		DeprecateSectionDisabledFlag = 15,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FSkeletalMeshCustomVersion() {}
};

// Custom serialization version for RecomputeTangent
struct FRecomputeTangentCustomVersion
{
	enum Type
{
		// Before any version changes were made in the plugin
		BeforeCustomVersionWasAdded = 0,
		// We serialize the RecomputeTangent Option
		RuntimeRecomputeTangent = 1,
		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
	
	// The GUID for this custom version number
	const static FGuid GUID;
	
private:
	FRecomputeTangentCustomVersion() {}
};

// custom version for overlapping vertcies code
struct FOverlappingVerticesCustomVersion
{
	enum Type
	{
		// Before any version changes were made in the plugin
		BeforeCustomVersionWasAdded = 0,
		// Converted to use HierarchicalInstancedStaticMeshComponent
		DetectOVerlappingVertices = 1,
		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version number
	const static FGuid GUID;

private:
	FOverlappingVerticesCustomVersion() {}
};

/** Flags used when building vertex buffers. */
struct ESkeletalMeshVertexFlags
{
	enum
	{
		None = 0x0,
		UseFullPrecisionUVs = 0x1,
		HasVertexColors = 0x2
	};
};


/**
 * A structure for holding mesh-to-mesh triangle influences to skin one mesh to another (similar to a wrap deformer)
 */
struct FMeshToMeshVertData
{
	// Barycentric coords and distance along normal for the position of the final vert
	FVector4 PositionBaryCoordsAndDist;

	// Barycentric coords and distance along normal for the location of the unit normal endpoint
	// Actual normal = ResolvedNormalPosition - ResolvedPosition
	FVector4 NormalBaryCoordsAndDist;

	// Barycentric coords and distance along normal for the location of the unit Tangent endpoint
	// Actual normal = ResolvedNormalPosition - ResolvedPosition
	FVector4 TangentBaryCoordsAndDist;

	// Contains the 3 indices for verts in the source mesh forming a triangle, the last element
	// is a flag to decide how the skinning works, 0xffff uses no simulation, and just normal
	// skinning, anything else uses the source mesh and the above skin data to get the final position
	uint16	 SourceMeshVertIndices[4];

	// Dummy for alignment (16 bytes)
	uint32	 Padding[2];

	/**
	 * Serializer
	 *
	 * @param Ar - archive to serialize with
	 * @param V - vertex to serialize
	 * @return archive that was used
	 */
	friend FArchive& operator<<(FArchive& Ar, FMeshToMeshVertData& V)
	{
		Ar	<< V.PositionBaryCoordsAndDist 
			<< V.NormalBaryCoordsAndDist
			<< V.TangentBaryCoordsAndDist
			<< V.SourceMeshVertIndices[0]
			<< V.SourceMeshVertIndices[1]
			<< V.SourceMeshVertIndices[2]
			<< V.SourceMeshVertIndices[3]
			<< V.Padding[0]
			<< V.Padding[1];
		return Ar;
	}
};

struct FClothingSectionData
{
	FClothingSectionData()
		: AssetGuid()
		, AssetLodIndex(INDEX_NONE)
	{}

	bool IsValid()
	{
		return AssetGuid.IsValid() && AssetLodIndex != INDEX_NONE;
	}

	/** Guid of the clothing asset applied to this section */
	FGuid AssetGuid;

	/** LOD inside the applied asset that is used */
	int32 AssetLodIndex;

	friend FArchive& operator<<(FArchive& Ar, FClothingSectionData& Data)
	{
		Ar << Data.AssetGuid;
		Ar << Data.AssetLodIndex;

		return Ar;
	}
};


/*-----------------------------------------------------------------------------
FSkeletalMeshSceneProxy
-----------------------------------------------------------------------------*/

/**
 * A skeletal mesh component scene proxy.
 */
class ENGINE_API FSkeletalMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override;

	/** 
	 * Constructor. 
	 * @param	Component - skeletal mesh primitive being added
	 */
	FSkeletalMeshSceneProxy(const USkinnedMeshComponent* Component, FSkeletalMeshRenderData* InSkelMeshRenderData);

#if WITH_EDITOR
	virtual HHitProxy* CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies) override;
#endif
	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual bool CanBeOccluded() const override;
	
	virtual bool HasDynamicIndirectShadowCasterRepresentation() const override;
	virtual void GetShadowShapes(TArray<FCapsuleShape>& CapsuleShapes) const override;

	/** Returns a pre-sorted list of shadow capsules's bone indicies */
	const TArray<uint16>& GetSortedShadowBoneIndices() const
	{
		return ShadowCapsuleBoneIndices;
	}

	/**
	 * Returns the world transform to use for drawing.
	 * @param OutLocalToWorld - Will contain the local-to-world transform when the function returns.
	 * @param OutWorldToLocal - Will contain the world-to-local transform when the function returns.
	 * 
	 * @return true if out matrices are valid 
	 */
	bool GetWorldMatrices( FMatrix& OutLocalToWorld, FMatrix& OutWorldToLocal ) const;

	/** Util for getting LOD index currently used by this SceneProxy. */
	int32 GetCurrentLODIndex();

	/** 
	 * Render physics asset for debug display
	 */
	virtual void DebugDrawPhysicsAsset(int32 ViewIndex, FMeshElementCollector& Collector, const FEngineShowFlags& EngineShowFlags) const;

	/** Render the bones of the skeleton for debug display */
	void DebugDrawSkeleton(int32 ViewIndex, FMeshElementCollector& Collector, const FEngineShowFlags& EngineShowFlags) const;

	virtual uint32 GetMemoryFootprint( void ) const override { return( sizeof( *this ) + GetAllocatedSize() ); }
	uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() + LODSections.GetAllocatedSize() ); }

	/**
	* Updates morph material usage for materials referenced by each LOD entry
	*
	* @param bNeedsMorphUsage - true if the materials used by this skeletal mesh need morph target usage
	*/
	void UpdateMorphMaterialUsage_GameThread(TArray<UMaterialInterface*>& MaterialUsingMorphTarget);


#if WITH_EDITORONLY_DATA
	virtual bool GetPrimitiveDistance(int32 LODIndex, int32 SectionIndex, const FVector& ViewOrigin, float& PrimitiveDistance) const override;
	virtual bool GetMeshUVDensities(int32 LODIndex, int32 SectionIndex, FVector4& WorldUVDensities) const override;
	virtual bool GetMaterialTextureScales(int32 LODIndex, int32 SectionIndex, const FMaterialRenderProxy* MaterialRenderProxy, FVector4* OneOverScales, FIntVector4* UVChannelIndices) const override;
#endif

	friend class FSkeletalMeshSectionIter;

protected:
	AActor* Owner;
	class FSkeletalMeshObject* MeshObject;
	FSkeletalMeshRenderData* SkeletalMeshRenderData;

	/** The points to the skeletal mesh and physics assets are purely for debug purposes. Access is NOT thread safe! */
	const USkeletalMesh* SkeletalMeshForDebug;
	class UPhysicsAsset* PhysicsAssetForDebug;

	/** data copied for rendering */
	uint32 bForceWireframe : 1;
	uint32 bIsCPUSkinned : 1;
	uint32 bCanHighlightSelectedSections : 1;
	FMaterialRelevance MaterialRelevance;

	ERHIFeatureLevel::Type FeatureLevel;

	/** info for section element in an LOD */
	struct FSectionElementInfo
	{
		FSectionElementInfo(UMaterialInterface* InMaterial, bool bInEnableShadowCasting, int32 InUseMaterialIndex)
		: Material( InMaterial )
		, bEnableShadowCasting( bInEnableShadowCasting )
		, UseMaterialIndex( InUseMaterialIndex )
#if WITH_EDITOR
		, HitProxy(NULL)
#endif
		{}
		
		UMaterialInterface* Material;
		
		/** Whether shadow casting is enabled for this section. */
		bool bEnableShadowCasting;
		
		/** Index into the materials array of the skel mesh or the component after LOD mapping */
		int32 UseMaterialIndex;

#if WITH_EDITOR
		/** The editor needs to be able to individual sub-mesh hit detection, so we store a hit proxy on each mesh. */
		HHitProxy* HitProxy;
#endif
	};

	/** Section elements for a particular LOD */
	struct FLODSectionElements
	{
		TArray<FSectionElementInfo> SectionElements;
	};
	
	/** Array of section elements for each LOD */
	TArray<FLODSectionElements> LODSections;

	/** 
	 * BoneIndex->capsule pairs used for rendering sphere/capsule shadows 
	 * Note that these are in refpose component space, NOT bone space.
	 */
	TArray<TPair<int32, FCapsuleShape>> ShadowCapsuleData;
	TArray<uint16> ShadowCapsuleBoneIndices;

	/** Set of materials used by this scene proxy, safe to access from the game thread. */
	TSet<UMaterialInterface*> MaterialsInUse_GameThread;
	bool bMaterialsNeedMorphUsage_GameThread;
	
#if WITH_EDITORONLY_DATA
	/** The component streaming distance multiplier */
	float StreamingDistanceMultiplier;
#endif

	void GetDynamicElementsSection(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap,
									const FSkeletalMeshLODRenderData& LODData, const int32 LODIndex, const int32 SectionIndex, bool bSectionSelected,
								   const FSectionElementInfo& SectionElementInfo, bool bInSelectable, FMeshElementCollector& Collector) const;

	void GetMeshElementsConditionallySelectable(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, bool bInSelectable, uint32 VisibilityMap, FMeshElementCollector& Collector) const;
};

/** Used to recreate all skinned mesh components for a given skeletal mesh */
class ENGINE_API FSkinnedMeshComponentRecreateRenderStateContext
{
public:

	/** Initialization constructor. */
	FSkinnedMeshComponentRecreateRenderStateContext(USkeletalMesh* InSkeletalMesh, bool InRefreshBounds = false);

	/** Destructor: recreates render state for all components that had their render states destroyed in the constructor. */
	~FSkinnedMeshComponentRecreateRenderStateContext();
	
private:

	/** List of components to reset */
	TArray< class USkinnedMeshComponent*> MeshComponents;

	/** Whether we'll refresh the component bounds as we reset */
	bool bRefreshBounds;
};

