// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	StaticMesh.h: Static mesh class definition.
=============================================================================*/

#pragma once

#include "CoreMinimal.h"
#include "Containers/IndirectArray.h"
#include "Misc/Guid.h"
#include "Engine/EngineTypes.h"
#include "UObject/UObjectIterator.h"
#include "Templates/ScopedPointer.h"
#include "Materials/MaterialInterface.h"
#include "RenderResource.h"
#include "PackedNormal.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "RawIndexBuffer.h"
#include "Components.h"
#include "LocalVertexFactory.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "Engine/MeshMerging.h"
#include "UObject/UObjectHash.h"
#include "MeshBatch.h"
#include "SceneManagement.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodySetupEnums.h"
#include "Materials/MaterialInterface.h"
#include "Rendering/ColorVertexBuffer.h"
#include "Rendering/StaticMeshVertexBuffer.h"
#include "Rendering/PositionVertexBuffer.h"
#include "Rendering/StaticMeshVertexDataInterface.h"
#include "UniquePtr.h"
#include "WeightedRandomSampler.h"

class FDistanceFieldVolumeData;
class UBodySetup;

/** The maximum number of static mesh LODs allowed. */
#define MAX_STATIC_MESH_LODS 8

struct FStaticMaterial;

/**
 * The LOD settings to use for a group of static meshes.
 */
class FStaticMeshLODGroup
{
public:
	/** Default values. */
	FStaticMeshLODGroup()
		: DefaultNumLODs(1)
		, DefaultLightMapResolution(64)
		, BasePercentTrianglesMult(1.0f)
		, DisplayName( NSLOCTEXT( "UnrealEd", "None", "None" ) )
	{
		FMemory::Memzero(SettingsBias);
		SettingsBias.PercentTriangles = 1.0f;
	}

	/** Returns the default number of LODs to build. */
	int32 GetDefaultNumLODs() const
	{
		return DefaultNumLODs;
	}

	/** Returns the default lightmap resolution. */
	int32 GetDefaultLightMapResolution() const
	{
		return DefaultLightMapResolution;
	}

	/** Returns default reduction settings for the specified LOD. */
	FMeshReductionSettings GetDefaultSettings(int32 LODIndex) const
	{
		check(LODIndex >= 0 && LODIndex < MAX_STATIC_MESH_LODS);
		return DefaultSettings[LODIndex];
	}

	/** Applies global settings tweaks for the specified LOD. */
	ENGINE_API FMeshReductionSettings GetSettings(const FMeshReductionSettings& InSettings, int32 LODIndex) const;

private:
	/** FStaticMeshLODSettings initializes group entries. */
	friend class FStaticMeshLODSettings;
	/** The default number of LODs to build. */
	int32 DefaultNumLODs;
	/** Default lightmap resolution. */
	int32 DefaultLightMapResolution;
	/** An additional reduction of base meshes in this group. */
	float BasePercentTrianglesMult;
	/** Display name. */
	FText DisplayName;
	/** Default reduction settings for meshes in this group. */
	FMeshReductionSettings DefaultSettings[MAX_STATIC_MESH_LODS];
	/** Biases applied to reduction settings. */
	FMeshReductionSettings SettingsBias;
};

/**
 * Per-group LOD settings for static meshes.
 */
class FStaticMeshLODSettings
{
public:

	/**
	 * Initializes LOD settings by reading them from the passed in config file section.
	 * @param IniFile Preloaded ini file object to load from
	 */
	ENGINE_API void Initialize(const FConfigFile& IniFile);

	/** Retrieve the settings for the specified LOD group. */
	const FStaticMeshLODGroup& GetLODGroup(FName LODGroup) const
	{
		const FStaticMeshLODGroup* Group = Groups.Find(LODGroup);
		if (Group == NULL)
		{
			Group = Groups.Find(NAME_None);
		}
		check(Group);
		return *Group;
	}

	/** Retrieve the names of all defined LOD groups. */
	void GetLODGroupNames(TArray<FName>& OutNames) const;

	/** Retrieves the localized display names of all LOD groups. */
	void GetLODGroupDisplayNames(TArray<FText>& OutDisplayNames) const;

private:
	/** Reads an entry from the INI to initialize settings for an LOD group. */
	void ReadEntry(FStaticMeshLODGroup& Group, FString Entry);
	/** Per-group settings. */
	TMap<FName,FStaticMeshLODGroup> Groups;
};


/**
 * A set of static mesh triangles which are rendered with the same material.
 */
struct FStaticMeshSection
{
	/** The index of the material with which to render this section. */
	int32 MaterialIndex;

	/** Range of vertices and indices used when rendering this section. */
	uint32 FirstIndex;
	uint32 NumTriangles;
	uint32 MinVertexIndex;
	uint32 MaxVertexIndex;

	/** If true, collision is enabled for this section. */
	bool bEnableCollision;
	/** If true, this section will cast a shadow. */
	bool bCastShadow;

#if WITH_EDITORONLY_DATA
	/** The UV channel density in LocalSpaceUnit / UV Unit. */
	float UVDensities[MAX_STATIC_TEXCOORDS];
	/** The weigths to apply to the UV density, based on the area. */
	float Weights[MAX_STATIC_TEXCOORDS];
#endif

	/** Constructor. */
	FStaticMeshSection()
		: MaterialIndex(0)
		, FirstIndex(0)
		, NumTriangles(0)
		, MinVertexIndex(0)
		, MaxVertexIndex(0)
		, bEnableCollision(false)
		, bCastShadow(true)
	{
#if WITH_EDITORONLY_DATA
		FMemory::Memzero(UVDensities);
		FMemory::Memzero(Weights);
#endif
	}

	/** Serializer. */
	friend FArchive& operator<<(FArchive& Ar,FStaticMeshSection& Section);
};


struct FStaticMeshLODResources;

/** Creates distribution for uniformly sampling a mesh section. */
struct ENGINE_API FStaticMeshSectionAreaWeightedTriangleSampler : FWeightedRandomSampler
{
	FStaticMeshSectionAreaWeightedTriangleSampler();
	void Init(FStaticMeshLODResources* InOwner, int32 InSectionIdx);
	virtual float GetWeights(TArray<float>& OutWeights) override;

protected:

	FStaticMeshLODResources* Owner;
	int32 SectionIdx;
};

struct ENGINE_API FStaticMeshAreaWeightedSectionSampler : FWeightedRandomSampler
{
	FStaticMeshAreaWeightedSectionSampler();
	void Init(FStaticMeshLODResources* InOwner);
	virtual float GetWeights(TArray<float>& OutWeights)override;

protected:
	FStaticMeshLODResources* Owner;
};


struct FDynamicMeshVertex;
struct FModelVertex;

struct FStaticMeshVertexBuffers
{
	/** The buffer containing vertex data. */
	FStaticMeshVertexBuffer StaticMeshVertexBuffer;
	/** The buffer containing the position vertex data. */
	FPositionVertexBuffer PositionVertexBuffer;
	/** The buffer containing the vertex color data. */
	FColorVertexBuffer ColorVertexBuffer;

	/* This is a temporary function to refactor and convert old code, do not copy this as is and try to build your data as SoA from the beginning.*/
	void ENGINE_API InitWithDummyData(FLocalVertexFactory* VertexFactory, uint32 NumVerticies, uint32 NumTexCoords = 1, uint32 LightMapIndex = 0);

	/* This is a temporary function to refactor and convert old code, do not copy this as is and try to build your data as SoA from the beginning.*/
	void ENGINE_API InitFromDynamicVertex(FLocalVertexFactory* VertexFactory, TArray<FDynamicMeshVertex>& Vertices, uint32 NumTexCoords = 1, uint32 LightMapIndex = 0);

	/* This is a temporary function to refactor and convert old code, do not copy this as is and try to build your data as SoA from the beginning.*/
	void ENGINE_API InitModelBuffers(TArray<FModelVertex>& Vertices);

	/* This is a temporary function to refactor and convert old code, do not copy this as is and try to build your data as SoA from the beginning.*/
	void ENGINE_API InitModelVF(FLocalVertexFactory* VertexFactory);
};

/** Rendering resources needed to render an individual static mesh LOD. */
struct FStaticMeshLODResources
{
	FStaticMeshVertexBuffers VertexBuffers;

	/** Index buffer resource for rendering. */
	FRawStaticIndexBuffer IndexBuffer;
	/** Reversed index buffer, used to prevent changing culling state between drawcalls. */
	FRawStaticIndexBuffer ReversedIndexBuffer;
	/** Index buffer resource for rendering in depth only passes. */
	FRawStaticIndexBuffer DepthOnlyIndexBuffer;
	/** Reversed depth only index buffer, used to prevent changing culling state between drawcalls. */
	FRawStaticIndexBuffer ReversedDepthOnlyIndexBuffer;
	/** Index buffer resource for rendering wireframe mode. */
	FRawStaticIndexBuffer WireframeIndexBuffer;
	/** Index buffer containing adjacency information required by tessellation. */
	FRawStaticIndexBuffer AdjacencyIndexBuffer;

	/** Sections for this LOD. */
	TArray<FStaticMeshSection> Sections;

	/** Distance field data associated with this mesh, null if not present.  */
	class FDistanceFieldVolumeData* DistanceFieldData; 

	/** The maximum distance by which this LOD deviates from the base from which it was generated. */
	float MaxDeviation;

	/** True if the adjacency index buffer contained data at init. Needed as it will not be available to the CPU afterwards. */
	uint32 bHasAdjacencyInfo : 1;

	/** True if the depth only index buffers contained data at init. Needed as it will not be available to the CPU afterwards. */
	uint32 bHasDepthOnlyIndices : 1;

	/** True if the reversed index buffers contained data at init. Needed as it will not be available to the CPU afterwards. */
	uint32 bHasReversedIndices : 1;

	/** True if the reversed index buffers contained data at init. Needed as it will not be available to the CPU afterwards. */
	uint32 bHasReversedDepthOnlyIndices: 1;
	
	/**	Allows uniform random selection of mesh sections based on their area. */
	FStaticMeshAreaWeightedSectionSampler AreaWeightedSampler;
	/**	Allows uniform random selection of triangles on each mesh section based on triangle area. */
	TArray<FStaticMeshSectionAreaWeightedTriangleSampler> AreaWeightedSectionSamplers;

	uint32 DepthOnlyNumTriangles;

#if STATS
	uint32 StaticMeshIndexMemory;
#endif
	
	/** Default constructor. */
	FStaticMeshLODResources();

	~FStaticMeshLODResources();

	/** Initializes all rendering resources. */
	void InitResources(UStaticMesh* Parent);

	/** Releases all rendering resources. */
	void ReleaseResources();

	/** Serialize. */
	void Serialize(FArchive& Ar, UObject* Owner, int32 Idx);

	/** Return the triangle count of this LOD. */
	ENGINE_API int32 GetNumTriangles() const;

	/** Return the number of vertices in this LOD. */
	ENGINE_API int32 GetNumVertices() const;

	ENGINE_API int32 GetNumTexCoords() const;
};

struct ENGINE_API FStaticMeshVertexFactories
{
	FStaticMeshVertexFactories(ERHIFeatureLevel::Type InFeatureLevel)
		: VertexFactory(InFeatureLevel, "FStaticMeshVertexFactories")
		, VertexFactoryOverrideColorVertexBuffer(InFeatureLevel, "FStaticMeshVertexFactories_Override")
		, SplineVertexFactory(nullptr)
		, SplineVertexFactoryOverrideColorVertexBuffer(nullptr)
	{}

	~FStaticMeshVertexFactories();

	/** The vertex factory used when rendering this mesh. */
	FLocalVertexFactory VertexFactory;

	/** The vertex factory used when rendering this mesh with vertex colors. This is lazy init.*/
	FLocalVertexFactory VertexFactoryOverrideColorVertexBuffer;

	struct FSplineMeshVertexFactory* SplineVertexFactory;

	struct FSplineMeshVertexFactory* SplineVertexFactoryOverrideColorVertexBuffer;

	/**
	* Initializes a vertex factory for rendering this static mesh
	*
	* @param	InOutVertexFactory				The vertex factory to configure
	* @param	InParentMesh					Parent static mesh
	* @param	bInOverrideColorVertexBuffer	If true, make a vertex factory ready for per-instance colors
	*/
	void InitVertexFactory(const FStaticMeshLODResources& LodResources, FLocalVertexFactory& InOutVertexFactory, const UStaticMesh* InParentMesh, bool bInOverrideColorVertexBuffer);

	/** Initializes all rendering resources. */
	void InitResources(const FStaticMeshLODResources& LodResources, const UStaticMesh* Parent);

	/** Releases all rendering resources. */
	void ReleaseResources();
};

/**
 * FStaticMeshRenderData - All data needed to render a static mesh.
 */
class FStaticMeshRenderData
{
public:
	/** Default constructor. */
	FStaticMeshRenderData();

	/** Per-LOD resources. */
	TIndirectArray<FStaticMeshLODResources> LODResources;
	TIndirectArray<FStaticMeshVertexFactories> LODVertexFactories;

	/** Screen size to switch LODs */
	float ScreenSize[MAX_STATIC_MESH_LODS];

	/** Bounds of the renderable mesh. */
	FBoxSphereBounds Bounds;

	/** True if LODs share static lighting data. */
	bool bLODsShareStaticLighting;

#if WITH_EDITORONLY_DATA
	/** The derived data key associated with this render data. */
	FString DerivedDataKey;

	/** Map of wedge index to vertex index. */
	TArray<int32> WedgeMap;

	/** Map of material index -> original material index at import time. */
	TArray<int32> MaterialIndexToImportIndex;

	/** UV data used for streaming accuracy debug view modes. In sync for rendering thread */
	TArray<FMeshUVChannelInfo> UVChannelDataPerMaterial;

	void SyncUVChannelData(const TArray<FStaticMaterial>& ObjectData);

	/** The next cached derived data in the list. */
	TUniquePtr<class FStaticMeshRenderData> NextCachedRenderData;

	/**
	 * Cache derived renderable data for the static mesh with the provided
	 * level of detail settings.
	 */
	void Cache(UStaticMesh* Owner, const FStaticMeshLODSettings& LODSettings);
#endif // #if WITH_EDITORONLY_DATA

	/** Serialization. */
	void Serialize(FArchive& Ar, UStaticMesh* Owner, bool bCooked);

	/** Initialize the render resources. */
	void InitResources(ERHIFeatureLevel::Type InFeatureLevel, UStaticMesh* Owner);

	/** Releases the render resources. */
	ENGINE_API void ReleaseResources();

	/** Compute the size of this resource. */
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const;

	/** Allocate LOD resources. */
	ENGINE_API void AllocateLODResources(int32 NumLODs);

	/** Update LOD-SECTION uv densities. */
	void ComputeUVDensities();

	void BuildAreaWeighedSamplingData();

private:
#if WITH_EDITORONLY_DATA
	/** Allow the editor to explicitly update section information. */
	friend class FLevelOfDetailSettingsLayout;
#endif // #if WITH_EDITORONLY_DATA
#if WITH_EDITOR
	/** Resolve all per-section settings. */
	ENGINE_API void ResolveSectionInfo(UStaticMesh* Owner);
#endif // #if WITH_EDITORONLY_DATA
};

/**
 * FStaticMeshComponentRecreateRenderStateContext - Destroys render state for all StaticMeshComponents using a given StaticMesh and 
 * recreates them when it goes out of scope. Used to ensure stale rendering data isn't kept around in the components when importing
 * over or rebuilding an existing static mesh.
 */
class FStaticMeshComponentRecreateRenderStateContext
{
public:

	/** Initialization constructor. */
	FStaticMeshComponentRecreateRenderStateContext( UStaticMesh* InStaticMesh, bool InUnbuildLighting = true, bool InRefreshBounds = false )
		: bUnbuildLighting( InUnbuildLighting ),
		  bRefreshBounds( InRefreshBounds )
	{
		for ( TObjectIterator<UStaticMeshComponent> It;It;++It )
		{
			if ( It->GetStaticMesh() == InStaticMesh )
			{
				checkf( !It->IsUnreachable(), TEXT("%s"), *It->GetFullName() );

				if ( It->bRenderStateCreated )
				{
					check( It->IsRegistered() );
					It->DestroyRenderState_Concurrent();
					StaticMeshComponents.Add(*It);
				}
			}
		}

		// Flush the rendering commands generated by the detachments.
		// The static mesh scene proxies reference the UStaticMesh, and this ensures that they are cleaned up before the UStaticMesh changes.
		FlushRenderingCommands();
	}

	/** Destructor: recreates render state for all components that had their render states destroyed in the constructor. */
	~FStaticMeshComponentRecreateRenderStateContext()
	{
		const int32 ComponentCount = StaticMeshComponents.Num();
		for( int32 ComponentIndex = 0; ComponentIndex < ComponentCount; ++ComponentIndex )
		{
			UStaticMeshComponent* Component = StaticMeshComponents[ ComponentIndex ];
			if ( bUnbuildLighting )
			{
				// Invalidate the component's static lighting.
				// This unregisters and reregisters so must not be in the constructor
				Component->InvalidateLightingCache();
			}

			if( bRefreshBounds )
			{
				Component->UpdateBounds();
			}

			if ( Component->IsRegistered() && !Component->bRenderStateCreated )
			{
				Component->CreateRenderState_Concurrent();
			}
		}
	}

private:

	TArray<UStaticMeshComponent*> StaticMeshComponents;
	bool bUnbuildLighting;
	bool bRefreshBounds;
};

/**
 * A static mesh component scene proxy.
 */
class ENGINE_API FStaticMeshSceneProxy : public FPrimitiveSceneProxy
{
public:
	SIZE_T GetTypeHash() const override;

	/** Initialization constructor. */
	FStaticMeshSceneProxy(UStaticMeshComponent* Component, bool bForceLODsShareStaticLighting);

	virtual ~FStaticMeshSceneProxy();

	/** Gets the number of mesh batches required to represent the proxy, aside from section needs. */
	virtual int32 GetNumMeshBatches() const
	{
		return 1;
	}

	/** Sets up a shadow FMeshBatch for a specific LOD. */
	virtual bool GetShadowMeshElement(int32 LODIndex, int32 BatchIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshBatch, bool bDitheredLODTransition) const;

	/** Sets up a FMeshBatch for a specific LOD and element. */
	virtual bool GetMeshElement(
		int32 LODIndex, 
		int32 BatchIndex, 
		int32 ElementIndex, 
		uint8 InDepthPriorityGroup, 
		bool bUseSelectedMaterial, 
		bool bUseHoveredMaterial, 
		bool bAllowPreCulledIndices,
		FMeshBatch& OutMeshBatch) const;

	/** Sets up a wireframe FMeshBatch for a specific LOD. */
	virtual bool GetWireframeMeshElement(int32 LODIndex, int32 BatchIndex, const FMaterialRenderProxy* WireframeRenderProxy, uint8 InDepthPriorityGroup, bool bAllowPreCulledIndices, FMeshBatch& OutMeshBatch) const;

protected:
	/**
	 * Sets IndexBuffer, FirstIndex and NumPrimitives of OutMeshElement.
	 */
	virtual void SetIndexSource(int32 LODIndex, int32 ElementIndex, FMeshBatch& OutMeshElement, bool bWireframe, bool bRequiresAdjacencyInformation, bool bUseInversedIndices, bool bAllowPreCulledIndices) const;
	bool IsCollisionView(const FEngineShowFlags& EngineShowFlags, bool& bDrawSimpleCollision, bool& bDrawComplexCollision) const;

public:
	// FPrimitiveSceneProxy interface.
#if WITH_EDITOR
	virtual HHitProxy* CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies) override;
#endif
	virtual void DrawStaticElements(FStaticPrimitiveDrawInterface* PDI) override;
	virtual void OnTransformChanged() override;
	virtual int32 GetLOD(const FSceneView* View) const override;
	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;
	virtual bool CanBeOccluded() const override;
	virtual void GetLightRelevance(const FLightSceneProxy* LightSceneProxy, bool& bDynamic, bool& bRelevant, bool& bLightMapped, bool& bShadowMapped) const override;
	virtual void GetDistancefieldAtlasData(FBox& LocalVolumeBounds, FVector2D& OutDistanceMinMax, FIntVector& OutBlockMin, FIntVector& OutBlockSize, bool& bOutBuiltAsIfTwoSided, bool& bMeshWasPlane, float& SelfShadowBias, TArray<FMatrix>& ObjectLocalToWorldTransforms) const override;
	virtual void GetDistanceFieldInstanceInfo(int32& NumInstances, float& BoundsSurfaceArea) const override;
	virtual bool HasDistanceFieldRepresentation() const override;
	virtual bool HasDynamicIndirectShadowCasterRepresentation() const override;
	virtual uint32 GetMemoryFootprint( void ) const override { return( sizeof( *this ) + GetAllocatedSize() ); }
	uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() + LODs.GetAllocatedSize() ); }

	virtual void GetMeshDescription(int32 LODIndex, TArray<FMeshBatch>& OutMeshElements) const override;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;

	virtual void GetLCIs(FLCIArray& LCIs) override;

#if WITH_EDITORONLY_DATA
	virtual bool GetPrimitiveDistance(int32 LODIndex, int32 SectionIndex, const FVector& ViewOrigin, float& PrimitiveDistance) const override;
	virtual bool GetMeshUVDensities(int32 LODIndex, int32 SectionIndex, FVector4& WorldUVDensities) const override;
	virtual bool GetMaterialTextureScales(int32 LODIndex, int32 SectionIndex, const FMaterialRenderProxy* MaterialRenderProxy, FVector4* OneOverScales, FIntVector4* UVChannelIndices) const override;
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	virtual int32 GetLightMapResolution() const override { return LightMapResolution; }
#endif

protected:
	/** Information used by the proxy about a single LOD of the mesh. */
	class FLODInfo : public FLightCacheInterface
	{
	public:

		/** Information about an element of a LOD. */
		struct FSectionInfo
		{
			/** Default constructor. */
			FSectionInfo()
				: Material(NULL)
				, bSelected(false)
#if WITH_EDITOR
				, HitProxy(NULL)
#endif
				, FirstPreCulledIndex(0)
				, NumPreCulledTriangles(-1)
			{}

			/** The material with which to render this section. */
			UMaterialInterface* Material;

			/** True if this section should be rendered as selected (editor only). */
			bool bSelected;

#if WITH_EDITOR
			/** The editor needs to be able to individual sub-mesh hit detection, so we store a hit proxy on each mesh. */
			HHitProxy* HitProxy;
#endif

#if WITH_EDITORONLY_DATA
			// The material index from the component. Used by the texture streaming accuracy viewmodes.
			int32 MaterialIndex;
#endif

			int32 FirstPreCulledIndex;
			int32 NumPreCulledTriangles;
		};

		/** Per-section information. */
		TArray<FSectionInfo> Sections;

		/** Vertex color data for this LOD (or NULL when not overridden), FStaticMeshComponentLODInfo handle the release of the memory */
		FColorVertexBuffer* OverrideColorVertexBuffer;

		const FRawStaticIndexBuffer* PreCulledIndexBuffer;

		/** Initialization constructor. */
		FLODInfo(const UStaticMeshComponent* InComponent, const TIndirectArray<FStaticMeshVertexFactories>& InLODVertexFactories, int32 InLODIndex, bool bLODsShareStaticLighting);

		bool UsesMeshModifyingMaterials() const { return bUsesMeshModifyingMaterials; }

		// FLightCacheInterface.
		virtual FLightInteraction GetInteraction(const FLightSceneProxy* LightSceneProxy) const override;

	private:
		TArray<FGuid> IrrelevantLights;

		/** True if any elements in this LOD use mesh-modifying materials **/
		bool bUsesMeshModifyingMaterials;
	};

	AActor* Owner;
	const UStaticMesh* StaticMesh;
	UBodySetup* BodySetup;
	FStaticMeshRenderData* RenderData;

	TIndirectArray<FLODInfo> LODs;

	const FDistanceFieldVolumeData* DistanceFieldData;	

	/** Hierarchical LOD Index used for rendering */
	uint8 HierarchicalLODIndex;

	/**
	 * The forcedLOD set in the static mesh editor, copied from the mesh component
	 */
	int32 ForcedLodModel;

	/** Minimum LOD index to use.  Clamped to valid range [0, NumLODs - 1]. */
	int32 ClampedMinLOD;

	FVector TotalScale3D;

	uint32 bCastShadow : 1;
	ECollisionTraceFlag		CollisionTraceFlag;

	/** The view relevance for all the static mesh's materials. */
	FMaterialRelevance MaterialRelevance;

	/** Collision Response of this component**/
	FCollisionResponseContainer CollisionResponse;

#if WITH_EDITORONLY_DATA
	/** The component streaming distance multiplier */
	float StreamingDistanceMultiplier;
	/** The cacheed GetTextureStreamingTransformScale */
	float StreamingTransformScale;
	/** Material bounds used for texture streaming. */
	TArray<uint32> MaterialStreamingRelativeBoxes;

	/** Index of the section to preview. If set to INDEX_NONE, all section will be rendered */
	int32 SectionIndexPreview;
	/** Index of the material to preview. If set to INDEX_NONE, all section will be rendered */
	int32 MaterialIndexPreview;
#endif

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	/** LightMap resolution used for VMI_LightmapDensity */
	int32 LightMapResolution;
#endif

#if !(UE_BUILD_SHIPPING)
	/** LOD used for collision */
	int32 LODForCollision;
	/** Draw mesh collision if used for complex collision */
	uint32 bDrawMeshCollisionIfComplex : 1;
	/** Draw mesh collision if used for simple collision */
	uint32 bDrawMeshCollisionIfSimple : 1;
#endif

	/**
	 * Returns the display factor for the given LOD level
	 *
	 * @Param LODIndex - The LOD to get the display factor for
	 */
	float GetScreenSize(int32 LODIndex) const;

	/**
	 * Returns the LOD mask for a view, this is like the ordinary LOD but can return two values for dither fading
	 */
	FLODMask GetLODMask(const FSceneView* View) const;

private:
	void AddSpeedTreeWind();
	void RemoveSpeedTreeWind();
};

/*-----------------------------------------------------------------------------
	FStaticMeshInstanceData
-----------------------------------------------------------------------------*/

/** The implementation of the static mesh instance data storage type. */
class FStaticMeshInstanceData
{
	template<typename F>
	struct FInstanceTransformMatrix
	{
		F InstanceTransform1[4];
		F InstanceTransform2[4];
		F InstanceTransform3[4];

		friend FArchive& operator<<(FArchive& Ar, FInstanceTransformMatrix& V)
		{
			return Ar
				<< V.InstanceTransform1[0]
				<< V.InstanceTransform1[1]
				<< V.InstanceTransform1[2]
				<< V.InstanceTransform1[3]

				<< V.InstanceTransform2[0]
				<< V.InstanceTransform2[1]
				<< V.InstanceTransform2[2]
				<< V.InstanceTransform2[3]

				<< V.InstanceTransform3[0]
				<< V.InstanceTransform3[1]
				<< V.InstanceTransform3[2]
				<< V.InstanceTransform3[3];
		}

	};

	struct FInstanceLightMapVector
	{
		int16 InstanceLightmapAndShadowMapUVBias[4];

		friend FArchive& operator<<(FArchive& Ar, FInstanceLightMapVector& V)
		{
			return Ar
				<< V.InstanceLightmapAndShadowMapUVBias[0]
				<< V.InstanceLightmapAndShadowMapUVBias[1]
				<< V.InstanceLightmapAndShadowMapUVBias[2]
				<< V.InstanceLightmapAndShadowMapUVBias[3];
		}
	};

public:
	FStaticMeshInstanceData()
	{

	}
	/**
	 * Constructor
	 * @param InNeedsCPUAccess - true if resource array data should be CPU accessible
	 * @param bSupportsVertexHalfFloat - true if device has support for half float in vertex arrays
	 */
	FStaticMeshInstanceData(bool InNeedsCPUAccess, bool bInUseHalfFloat)
		: bUseHalfFloat(PLATFORM_BUILTIN_VERTEX_HALF_FLOAT || bInUseHalfFloat)
		, bNeedsCPUAccess(InNeedsCPUAccess)
	{
		InstanceOriginData = new TStaticMeshVertexData<FVector4>(bNeedsCPUAccess);
		InstanceLightmapData = new TStaticMeshVertexData<FInstanceLightMapVector>(bNeedsCPUAccess);

		if (bUseHalfFloat)
		{
			InstanceTransformData = new TStaticMeshVertexData<FInstanceTransformMatrix<FFloat16>>(bNeedsCPUAccess);
		}
		else
		{
			InstanceTransformData = new TStaticMeshVertexData<FInstanceTransformMatrix<float>>(bNeedsCPUAccess);
		}
	}

	~FStaticMeshInstanceData()
	{
		delete InstanceOriginData;
		delete InstanceLightmapData;
		delete InstanceTransformData;
	}

	static size_t GetResourceSize(int32 InNumInstances, bool bInUseHalfFloat)
	{
		return size_t(InNumInstances) * ((bInUseHalfFloat ? sizeof(FInstanceTransformMatrix<FFloat16>) : sizeof(FInstanceTransformMatrix<float>)) + sizeof(FInstanceLightMapVector) + sizeof(FVector4));
	}

	void Serialize(FArchive& Ar)
	{
		InstanceOriginData->Serialize(Ar);
		InstanceLightmapData->Serialize(Ar);
		InstanceTransformData->Serialize(Ar);
	}

	void AllocateInstances(int32 InNumInstances, bool DestroyExistingInstances)
	{
		int32 Delta = InNumInstances - NumInstances;
		NumInstances = InNumInstances;

		if (DestroyExistingInstances)
		{
			InstanceOriginData->Empty(NumInstances);
			InstanceLightmapData->Empty(NumInstances);
			InstanceTransformData->Empty(NumInstances);
		}

		// We cannot write directly to the data on all platforms,
		// so we make a TArray of the right type, then assign it
		InstanceOriginData->ResizeBuffer(NumInstances);
		InstanceOriginDataPtr = InstanceOriginData->GetDataPointer();

		InstanceLightmapData->ResizeBuffer(NumInstances);
		InstanceLightmapDataPtr = InstanceLightmapData->GetDataPointer();

		InstanceTransformData->ResizeBuffer(NumInstances);
		InstanceTransformDataPtr = InstanceTransformData->GetDataPointer();

		if (Delta > 0)
		{
			for (int32 i = 0; i < Delta; ++i)
			{
				InstancesUsage.Add(false);
			}
		}
	}

	FORCEINLINE_DEBUGGABLE int32 IsValidIndex(int32 Index) const
	{
		return InstanceOriginData->IsValidIndex(Index);
	}

	FORCEINLINE_DEBUGGABLE void GetInstanceTransform(int32 InstanceIndex, FMatrix& Transform) const
	{
		FVector4 TransformVec[3];
		if (bUseHalfFloat)
		{
			GetInstanceTransformInternal<FFloat16>(InstanceIndex, TransformVec);
		}
		else
		{
			GetInstanceTransformInternal<float>(InstanceIndex, TransformVec);
		}

		Transform.M[0][0] = TransformVec[0][0];
		Transform.M[0][1] = TransformVec[0][1];
		Transform.M[0][2] = TransformVec[0][2];
		Transform.M[0][3] = 0.f;

		Transform.M[1][0] = TransformVec[1][0];
		Transform.M[1][1] = TransformVec[1][1];
		Transform.M[1][2] = TransformVec[1][2];
		Transform.M[1][3] = 0.f;

		Transform.M[2][0] = TransformVec[2][0];
		Transform.M[2][1] = TransformVec[2][1];
		Transform.M[2][2] = TransformVec[2][2];
		Transform.M[2][3] = 0.f;

		FVector4 Origin;
		GetInstanceOriginInternal(InstanceIndex, Origin);

		Transform.M[3][0] = Origin.X;
		Transform.M[3][1] = Origin.Y;
		Transform.M[3][2] = Origin.Z;
		Transform.M[3][3] = 0.f;
	}

	FORCEINLINE_DEBUGGABLE void GetInstanceShaderValues(int32 InstanceIndex, FVector4 (&InstanceTransform)[3], FVector4& InstanceLightmapAndShadowMapUVBias, FVector4& InstanceOrigin) const
	{
		if (bUseHalfFloat)
		{
			GetInstanceTransformInternal<FFloat16>(InstanceIndex, InstanceTransform);
		}
		else
		{
			GetInstanceTransformInternal<float>(InstanceIndex, InstanceTransform);
		}
		GetInstanceLightMapDataInternal(InstanceIndex, InstanceLightmapAndShadowMapUVBias);
		GetInstanceOriginInternal(InstanceIndex, InstanceOrigin);
	}

	FORCEINLINE int32 GetNextAvailableInstanceIndex() const
	{
		return InstancesUsage.Find(false);
	}

	FORCEINLINE_DEBUGGABLE void SetInstance(int32 InstanceIndex, const FMatrix& Transform, float RandomInstanceID)
	{
		FVector4 Origin(Transform.M[3][0], Transform.M[3][1], Transform.M[3][2], RandomInstanceID);
		SetInstanceOriginInternal(InstanceIndex, Origin);

		FVector4 InstanceTransform[3];
		InstanceTransform[0] = FVector4(Transform.M[0][0], Transform.M[0][1], Transform.M[0][2], 0.0f);
		InstanceTransform[1] = FVector4(Transform.M[1][0], Transform.M[1][1], Transform.M[1][2], 0.0f);
		InstanceTransform[2] = FVector4(Transform.M[2][0], Transform.M[2][1], Transform.M[2][2], 0.0f);

		if (bUseHalfFloat)
		{
			SetInstanceTransformInternal<FFloat16>(InstanceIndex, InstanceTransform);
		}
		else
		{
			SetInstanceTransformInternal<float>(InstanceIndex, InstanceTransform);
		}

		SetInstanceLightMapDataInternal(InstanceIndex, FVector4(0, 0, 0, 0));

		InstancesUsage[InstanceIndex] = true;
	}
	
	FORCEINLINE_DEBUGGABLE void SetInstance(int32 InstanceIndex, const FMatrix& Transform, float RandomInstanceID, const FVector2D& LightmapUVBias, const FVector2D& ShadowmapUVBias)
	{
		FVector4 Origin(Transform.M[3][0], Transform.M[3][1], Transform.M[3][2], RandomInstanceID);
		SetInstanceOriginInternal(InstanceIndex, Origin);

		FVector4 InstanceTransform[3];
		InstanceTransform[0] = FVector4(Transform.M[0][0], Transform.M[0][1], Transform.M[0][2], 0.0f);
		InstanceTransform[1] = FVector4(Transform.M[1][0], Transform.M[1][1], Transform.M[1][2], 0.0f);
		InstanceTransform[2] = FVector4(Transform.M[2][0], Transform.M[2][1], Transform.M[2][2], 0.0f);

		if (bUseHalfFloat)
		{
			SetInstanceTransformInternal<FFloat16>(InstanceIndex, InstanceTransform);
		}
		else
		{
			SetInstanceTransformInternal<float>(InstanceIndex, InstanceTransform);
		}

		SetInstanceLightMapDataInternal(InstanceIndex, FVector4(LightmapUVBias.X, LightmapUVBias.Y, ShadowmapUVBias.X, ShadowmapUVBias.Y));

		InstancesUsage[InstanceIndex] = true;
	}

	FORCEINLINE void SetInstance(int32 InstanceIndex, const FMatrix& Transform, const FVector2D& LightmapUVBias, const FVector2D& ShadowmapUVBias)
	{
		FVector4 OldOrigin;
		GetInstanceOriginInternal(InstanceIndex, OldOrigin);

		FVector4 NewOrigin(Transform.M[3][0], Transform.M[3][1], Transform.M[3][2], OldOrigin.Component(3));
		SetInstanceOriginInternal(InstanceIndex, NewOrigin);

		FVector4 InstanceTransform[3];
		InstanceTransform[0] = FVector4(Transform.M[0][0], Transform.M[0][1], Transform.M[0][2], 0.0f);
		InstanceTransform[1] = FVector4(Transform.M[1][0], Transform.M[1][1], Transform.M[1][2], 0.0f);
		InstanceTransform[2] = FVector4(Transform.M[2][0], Transform.M[2][1], Transform.M[2][2], 0.0f);

		if (bUseHalfFloat)
		{
			SetInstanceTransformInternal<FFloat16>(InstanceIndex, InstanceTransform);
		}
		else
		{
			SetInstanceTransformInternal<float>(InstanceIndex, InstanceTransform);
		}

		SetInstanceLightMapDataInternal(InstanceIndex, FVector4(LightmapUVBias.X, LightmapUVBias.Y, ShadowmapUVBias.X, ShadowmapUVBias.Y));

		InstancesUsage[InstanceIndex] = true;
	}
	
	FORCEINLINE_DEBUGGABLE void NullifyInstance(int32 InstanceIndex)
	{
		SetInstanceOriginInternal(InstanceIndex, FVector4(0, 0, 0, 0));

		FVector4 InstanceTransform[3];
		InstanceTransform[0] = FVector4(0, 0, 0, 0);
		InstanceTransform[1] = FVector4(0, 0, 0, 0);
		InstanceTransform[2] = FVector4(0, 0, 0, 0);

		if (bUseHalfFloat)
		{
			SetInstanceTransformInternal<FFloat16>(InstanceIndex, InstanceTransform);
		}
		else
		{
			SetInstanceTransformInternal<float>(InstanceIndex, InstanceTransform);
		}

		SetInstanceLightMapDataInternal(InstanceIndex, FVector4(0, 0, 0, 0));

		InstancesUsage[InstanceIndex] = false;
	}

	FORCEINLINE_DEBUGGABLE void SetInstanceEditorData(int32 InstanceIndex, FColor HitProxyColor, bool bSelected)
	{
		FVector4 InstanceTransform[3];
		if (bUseHalfFloat)
		{
			GetInstanceTransformInternal<FFloat16>(InstanceIndex, InstanceTransform);
			InstanceTransform[0][3] = ((float)HitProxyColor.R) + (bSelected ? 256.f : 0.0f);
			InstanceTransform[1][3] = (float)HitProxyColor.G;
			InstanceTransform[2][3] = (float)HitProxyColor.B;
			SetInstanceTransformInternal<FFloat16>(InstanceIndex, InstanceTransform);
		}
		else
		{
			GetInstanceTransformInternal<float>(InstanceIndex, InstanceTransform);
			InstanceTransform[0][3] = ((float)HitProxyColor.R) + (bSelected ? 256.f : 0.0f);
			InstanceTransform[1][3] = (float)HitProxyColor.G;
			InstanceTransform[2][3] = (float)HitProxyColor.B;
			SetInstanceTransformInternal<float>(InstanceIndex, InstanceTransform);
		}

		InstancesUsage[InstanceIndex] = true;
	}

	FORCEINLINE_DEBUGGABLE void SwapInstance(int32 Index1, int32 Index2)
	{
		if (bUseHalfFloat)
		{
			FInstanceTransformMatrix<FFloat16>* ElementData = reinterpret_cast<FInstanceTransformMatrix<FFloat16>*>(InstanceTransformDataPtr);
			check((void*)((&ElementData[Index1]) + 1) <= (void*)(InstanceTransformDataPtr + InstanceTransformData->GetResourceSize()));
			check((void*)((&ElementData[Index1]) + 0) >= (void*)(InstanceTransformDataPtr));
			check((void*)((&ElementData[Index2]) + 1) <= (void*)(InstanceTransformDataPtr + InstanceTransformData->GetResourceSize()));
			check((void*)((&ElementData[Index2]) + 0) >= (void*)(InstanceTransformDataPtr));

			FInstanceTransformMatrix<FFloat16> TempStore = ElementData[Index1];
			ElementData[Index1] = ElementData[Index2];
			ElementData[Index2] = TempStore;
		}
		else
		{
			FInstanceTransformMatrix<float>* ElementData = reinterpret_cast<FInstanceTransformMatrix<float>*>(InstanceTransformDataPtr);
			check((void*)((&ElementData[Index1]) + 1) <= (void*)(InstanceTransformDataPtr + InstanceTransformData->GetResourceSize()));
			check((void*)((&ElementData[Index1]) + 0) >= (void*)(InstanceTransformDataPtr));
			check((void*)((&ElementData[Index2]) + 1) <= (void*)(InstanceTransformDataPtr + InstanceTransformData->GetResourceSize()));
			check((void*)((&ElementData[Index2]) + 0) >= (void*)(InstanceTransformDataPtr));
			
			FInstanceTransformMatrix<float> TempStore = ElementData[Index1];
			ElementData[Index1] = ElementData[Index2];
			ElementData[Index2] = TempStore;
		}
		{

			FVector4* ElementData = reinterpret_cast<FVector4*>(InstanceOriginDataPtr);
			check((void*)((&ElementData[Index1]) + 1) <= (void*)(InstanceOriginDataPtr + InstanceOriginData->GetResourceSize()));
			check((void*)((&ElementData[Index1]) + 0) >= (void*)(InstanceOriginDataPtr));
			check((void*)((&ElementData[Index2]) + 1) <= (void*)(InstanceOriginDataPtr + InstanceOriginData->GetResourceSize()));
			check((void*)((&ElementData[Index2]) + 0) >= (void*)(InstanceOriginDataPtr));

			FVector4 TempStore = ElementData[Index1];
			ElementData[Index1] = ElementData[Index2];
			ElementData[Index2] = TempStore;
		}
		{
			FInstanceLightMapVector* ElementData = reinterpret_cast<FInstanceLightMapVector*>(InstanceLightmapDataPtr);
			check((void*)((&ElementData[Index1]) + 1) <= (void*)(InstanceLightmapDataPtr + InstanceLightmapData->GetResourceSize()));
			check((void*)((&ElementData[Index1]) + 0) >= (void*)(InstanceLightmapDataPtr));
			check((void*)((&ElementData[Index2]) + 1) <= (void*)(InstanceLightmapDataPtr + InstanceLightmapData->GetResourceSize()));
			check((void*)((&ElementData[Index2]) + 0) >= (void*)(InstanceLightmapDataPtr));
			
			FInstanceLightMapVector TempStore = ElementData[Index1];
			ElementData[Index1] = ElementData[Index2];
			ElementData[Index2] = TempStore;
		}
	}

	FORCEINLINE_DEBUGGABLE int32 GetNumInstances() const
	{
		return NumInstances;
	}

	FORCEINLINE_DEBUGGABLE size_t GetResourceSize() const
	{
		return (size_t)InstanceOriginData->GetResourceSize() + (size_t)InstanceTransformData->GetResourceSize() + (size_t)InstanceLightmapData->GetResourceSize();
	}

	FORCEINLINE_DEBUGGABLE bool GetAllowCPUAccess() const
	{
		return bNeedsCPUAccess;
	}

	FORCEINLINE_DEBUGGABLE bool GetTranslationUsesHalfs() const
	{
		return bUseHalfFloat;
	}

	FORCEINLINE_DEBUGGABLE FResourceArrayInterface* GetOriginResourceArray()
	{
		return InstanceOriginData->GetResourceArray();
	}

	FORCEINLINE_DEBUGGABLE FResourceArrayInterface* GetTransformResourceArray()
	{
		return InstanceTransformData->GetResourceArray();
	}

	FORCEINLINE_DEBUGGABLE FResourceArrayInterface* GetLightMapResourceArray()
	{
		return InstanceLightmapData->GetResourceArray();
	}

	FORCEINLINE_DEBUGGABLE uint32 GetOriginStride()
	{
		return InstanceOriginData->GetStride();
	}

	FORCEINLINE_DEBUGGABLE uint32 GetTransformStride()
	{
		return InstanceTransformData->GetStride();
	}

	FORCEINLINE_DEBUGGABLE uint32 GetLightMapStride()
	{
		return InstanceLightmapData->GetStride();
	}

private:
	template<typename T>
	FORCEINLINE_DEBUGGABLE void GetInstanceTransformInternal(int32 InstanceIndex, FVector4 (&Transform)[3]) const
	{
		FInstanceTransformMatrix<T>* ElementData = reinterpret_cast<FInstanceTransformMatrix<T>*>(InstanceTransformDataPtr);
		check((void*)((&ElementData[InstanceIndex]) + 1) <= (void*)(InstanceTransformDataPtr + InstanceTransformData->GetResourceSize()));
		check((void*)((&ElementData[InstanceIndex]) + 0) >= (void*)(InstanceTransformDataPtr));
		
		Transform[0][0] = ElementData[InstanceIndex].InstanceTransform1[0];
		Transform[0][1] = ElementData[InstanceIndex].InstanceTransform1[1];
		Transform[0][2] = ElementData[InstanceIndex].InstanceTransform1[2];
		Transform[0][3] = ElementData[InstanceIndex].InstanceTransform1[3];
		
		Transform[1][0] = ElementData[InstanceIndex].InstanceTransform2[0];
		Transform[1][1] = ElementData[InstanceIndex].InstanceTransform2[1];
		Transform[1][2] = ElementData[InstanceIndex].InstanceTransform2[2];
		Transform[1][3] = ElementData[InstanceIndex].InstanceTransform2[3];
		
		Transform[2][0] = ElementData[InstanceIndex].InstanceTransform3[0];
		Transform[2][1] = ElementData[InstanceIndex].InstanceTransform3[1];
		Transform[2][2] = ElementData[InstanceIndex].InstanceTransform3[2];
		Transform[2][3] = ElementData[InstanceIndex].InstanceTransform3[3];
	}

	FORCEINLINE_DEBUGGABLE void GetInstanceOriginInternal(int32 InstanceIndex, FVector4 &Origin) const
	{
		FVector4* ElementData = reinterpret_cast<FVector4*>(InstanceOriginDataPtr);
		check((void*)((&ElementData[InstanceIndex]) + 1) <= (void*)(InstanceOriginDataPtr + InstanceOriginData->GetResourceSize()));
		check((void*)((&ElementData[InstanceIndex]) + 0) >= (void*)(InstanceOriginDataPtr));

		Origin = ElementData[InstanceIndex];
	}

	FORCEINLINE_DEBUGGABLE void GetInstanceLightMapDataInternal(int32 InstanceIndex, FVector4 &LightmapData) const
	{
		FInstanceLightMapVector* ElementData = reinterpret_cast<FInstanceLightMapVector*>(InstanceLightmapDataPtr);
		check((void*)((&ElementData[InstanceIndex]) + 1) <= (void*)(InstanceLightmapDataPtr + InstanceLightmapData->GetResourceSize()));
		check((void*)((&ElementData[InstanceIndex]) + 0) >= (void*)(InstanceLightmapDataPtr));

		LightmapData = FVector4
		(
			float(ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[0]) / 32767.0f, 
			float(ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[1]) / 32767.0f,
			float(ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[2]) / 32767.0f,
			float(ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[3]) / 32767.0f
		);
	}

	template<typename T>
	FORCEINLINE_DEBUGGABLE void SetInstanceTransformInternal(int32 InstanceIndex, FVector4(Transform)[3]) const
	{
		FInstanceTransformMatrix<T>* ElementData = reinterpret_cast<FInstanceTransformMatrix<T>*>(InstanceTransformDataPtr);
		check((void*)((&ElementData[InstanceIndex]) + 1) <= (void*)(InstanceTransformDataPtr + InstanceTransformData->GetResourceSize()));
		check((void*)((&ElementData[InstanceIndex]) + 0) >= (void*)(InstanceTransformDataPtr));

		ElementData[InstanceIndex].InstanceTransform1[0] = Transform[0][0];
		ElementData[InstanceIndex].InstanceTransform1[1] = Transform[0][1];
		ElementData[InstanceIndex].InstanceTransform1[2] = Transform[0][2];
		ElementData[InstanceIndex].InstanceTransform1[3] = Transform[0][3];

		ElementData[InstanceIndex].InstanceTransform2[0] = Transform[1][0];
		ElementData[InstanceIndex].InstanceTransform2[1] = Transform[1][1];
		ElementData[InstanceIndex].InstanceTransform2[2] = Transform[1][2];
		ElementData[InstanceIndex].InstanceTransform2[3] = Transform[1][3];

		ElementData[InstanceIndex].InstanceTransform3[0] = Transform[2][0];
		ElementData[InstanceIndex].InstanceTransform3[1] = Transform[2][1];
		ElementData[InstanceIndex].InstanceTransform3[2] = Transform[2][2];
		ElementData[InstanceIndex].InstanceTransform3[3] = Transform[2][3];
	}

	FORCEINLINE_DEBUGGABLE void SetInstanceOriginInternal(int32 InstanceIndex, const FVector4& Origin) const
	{
		FVector4* ElementData = reinterpret_cast<FVector4*>(InstanceOriginDataPtr);
		check((void*)((&ElementData[InstanceIndex]) + 1) <= (void*)(InstanceOriginDataPtr + InstanceOriginData->GetResourceSize()));
		check((void*)((&ElementData[InstanceIndex]) + 0) >= (void*)(InstanceOriginDataPtr));

		ElementData[InstanceIndex] = Origin;
	}

	FORCEINLINE_DEBUGGABLE void SetInstanceLightMapDataInternal(int32 InstanceIndex, const FVector4& LightmapData) const
	{
		FInstanceLightMapVector* ElementData = reinterpret_cast<FInstanceLightMapVector*>(InstanceLightmapDataPtr);
		check((void*)((&ElementData[InstanceIndex]) + 1) <= (void*)(InstanceLightmapDataPtr + InstanceLightmapData->GetResourceSize()));
		check((void*)((&ElementData[InstanceIndex]) + 0) >= (void*)(InstanceLightmapDataPtr));

		ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[0] = FMath::Clamp<int32>(FMath::TruncToInt(LightmapData.X * 32767.0f), MIN_int16, MAX_int16);
		ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[1] = FMath::Clamp<int32>(FMath::TruncToInt(LightmapData.Y * 32767.0f), MIN_int16, MAX_int16);
		ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[2] = FMath::Clamp<int32>(FMath::TruncToInt(LightmapData.Z * 32767.0f), MIN_int16, MAX_int16);
		ElementData[InstanceIndex].InstanceLightmapAndShadowMapUVBias[3] = FMath::Clamp<int32>(FMath::TruncToInt(LightmapData.W * 32767.0f), MIN_int16, MAX_int16);
	}

	FStaticMeshVertexDataInterface* InstanceOriginData = nullptr;
	uint8* InstanceOriginDataPtr = nullptr;

	FStaticMeshVertexDataInterface* InstanceTransformData = nullptr;
	uint8* InstanceTransformDataPtr = nullptr;

	FStaticMeshVertexDataInterface* InstanceLightmapData = nullptr;
	uint8* InstanceLightmapDataPtr = nullptr;	

	TBitArray<> InstancesUsage;

	int32 NumInstances = 0;
	const bool bUseHalfFloat = false;
	const bool bNeedsCPUAccess = false;
};
	
#if WITH_EDITOR
/**
 * Remaps painted vertex colors when the renderable mesh has changed.
 * @param InPaintedVertices - The original position and normal for each painted vertex.
 * @param InOverrideColors - The painted vertex colors.
 * @param NewPositions - Positions of the new renderable mesh on which colors are to be mapped.
 * @param OptionalVertexBuffer - [optional] Vertex buffer containing vertex normals for the new mesh.
 * @param OutOverrideColors - Will contain vertex colors for the new mesh.
 */
ENGINE_API void RemapPaintedVertexColors(
	const TArray<FPaintedVertex>& InPaintedVertices,
	const FColorVertexBuffer* InOverrideColors,
	const FPositionVertexBuffer& OldPositions,
	const FStaticMeshVertexBuffer& OldVertexBuffer,
	const FPositionVertexBuffer& NewPositions,	
	const FStaticMeshVertexBuffer* OptionalVertexBuffer,
	TArray<FColor>& OutOverrideColors
	);
#endif // #if WITH_EDITOR
