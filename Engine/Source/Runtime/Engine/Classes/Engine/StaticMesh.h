// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "StaticMesh.generated.h"

/** The maximum number of static mesh LODs allowed. */
#define MAX_STATIC_MESH_LODS 4

/*-----------------------------------------------------------------------------
	Legacy mesh optimization settings.
-----------------------------------------------------------------------------*/

/** Optimization settings used to simplify mesh LODs. */
UENUM()
enum ENormalMode
{
	NM_PreserveSmoothingGroups,
	NM_RecalculateNormals,
	NM_RecalculateNormalsSmooth,
	NM_RecalculateNormalsHard,
	TEMP_BROKEN,
	ENormalMode_MAX,
};

UENUM()
enum EImportanceLevel
{
	IL_Off,
	IL_Lowest,
	IL_Low,
	IL_Normal,
	IL_High,
	IL_Highest,
	TEMP_BROKEN2,
	EImportanceLevel_MAX,
};

/** Enum specifying the reduction type to use when simplifying static meshes. */
UENUM()
enum EOptimizationType
{
	OT_NumOfTriangles,
	OT_MaxDeviation,
	OT_MAX,
};

/** Old optimization settings. */
USTRUCT()
struct FStaticMeshOptimizationSettings
{
	GENERATED_USTRUCT_BODY()

	/** The method to use when optimizing the skeletal mesh LOD */
	UPROPERTY()
	TEnumAsByte<enum EOptimizationType> ReductionMethod;

	/** If ReductionMethod equals SMOT_NumOfTriangles this value is the ratio of triangles [0-1] to remove from the mesh */
	UPROPERTY()
	float NumOfTrianglesPercentage;

	/**If ReductionMethod equals SMOT_MaxDeviation this value is the maximum deviation from the base mesh as a percentage of the bounding sphere. */
	UPROPERTY()
	float MaxDeviationPercentage;

	/** The welding threshold distance. Vertices under this distance will be welded. */
	UPROPERTY()
	float WeldingThreshold;

	/** Whether Normal smoothing groups should be preserved. If false then NormalsThreshold is used **/
	UPROPERTY()
	bool bRecalcNormals;

	/** If the angle between two triangles are above this value, the normals will not be
	smooth over the edge between those two triangles. Set in degrees. This is only used when PreserveNormals is set to false*/
	UPROPERTY()
	float NormalsThreshold;

	/** How important the shape of the geometry is (EImportanceLevel). */
	UPROPERTY()
	uint8 SilhouetteImportance;

	/** How important texture density is (EImportanceLevel). */
	UPROPERTY()
	uint8 TextureImportance;

	/** How important shading quality is. */
	UPROPERTY()
	uint8 ShadingImportance;


		FStaticMeshOptimizationSettings()
		: ReductionMethod( OT_MaxDeviation )
		, NumOfTrianglesPercentage( 1.0f )
		, MaxDeviationPercentage( 0.0f )
		, WeldingThreshold( 0.1f )
		, bRecalcNormals( true )
		, NormalsThreshold( 60.0f )
		, SilhouetteImportance( IL_Normal )
		, TextureImportance( IL_Normal )
		, ShadingImportance( IL_Normal )
		{
		}

		/** Serialization for FStaticMeshOptimizationSettings. */
		inline friend FArchive& operator<<( FArchive& Ar, FStaticMeshOptimizationSettings& Settings )
		{
			if( Ar.UE4Ver() < VER_UE4_ADDED_EXTRA_MESH_OPTIMIZATION_SETTINGS )
			{
				Ar << Settings.MaxDeviationPercentage;

				//Remap Importance Settings
				Ar << Settings.SilhouetteImportance;
				Ar << Settings.TextureImportance;

				//IL_Normal was previously the first enum value. We add the new index of IL_Normal to correctly offset the old values.
				Settings.SilhouetteImportance += IL_Normal;
				Settings.TextureImportance += IL_Normal;

				//Remap NormalMode enum value to new threshold variable.
				uint8 NormalMode;
				Ar << NormalMode;

				const float NormalThresholdTable[] =
				{
					60.0f, // Recompute
					80.0f, // Recompute (Smooth)
					45.0f  // Recompute (Hard)
				};

				if( NormalMode == NM_PreserveSmoothingGroups)
				{
					Settings.bRecalcNormals = false;
				}
				else
				{
					Settings.bRecalcNormals = true;
					Settings.NormalsThreshold = NormalThresholdTable[NormalMode];
				}
			}
			else
			{
				Ar << Settings.ReductionMethod;
			Ar << Settings.MaxDeviationPercentage;
				Ar << Settings.NumOfTrianglesPercentage;
			Ar << Settings.SilhouetteImportance;
			Ar << Settings.TextureImportance;
				Ar << Settings.ShadingImportance;
				Ar << Settings.bRecalcNormals;
				Ar << Settings.NormalsThreshold;
				Ar << Settings.WeldingThreshold;
			}

			return Ar;
		}

};

/*-----------------------------------------------------------------------------
	UStaticMesh
-----------------------------------------------------------------------------*/

/**
 * Source model from which a renderable static mesh is built.
 */
USTRUCT()
struct FStaticMeshSourceModel
{
	GENERATED_USTRUCT_BODY()

#if WITH_EDITOR
	/** Imported raw mesh data. Optional for all but the first LOD. */
	class FRawMeshBulkData* RawMeshBulkData;
#endif // #if WITH_EDITORONLY_DATA

	/** Settings applied when building the mesh. */
	UPROPERTY(EditAnywhere, Category=BuildSettings)
	FMeshBuildSettings BuildSettings;

	/** Reduction settings to apply when building render data. */
	UPROPERTY(EditAnywhere, Category=ReductionSettings)
	FMeshReductionSettings ReductionSettings;

	/** The distance at which this LOD swaps in. */
	UPROPERTY(EditAnywhere, Category=ReductionSettings)
	float LODDistance;

	/** Default constructor. */
	ENGINE_API FStaticMeshSourceModel();

	/** Destructor. */
	ENGINE_API ~FStaticMeshSourceModel();

	/** Serializes bulk data. */
	void SerializeBulkData(FArchive& Ar, UObject* Owner);
};

/**
 * Per-section settings.
 */
USTRUCT()
struct FMeshSectionInfo
{
	GENERATED_USTRUCT_BODY()

	/** Index in to the Materials array on UStaticMesh. */
	UPROPERTY()
	int32 MaterialIndex;

	/** If true, collision is enabled for this section. */
	UPROPERTY()
	bool bEnableCollision;

	/** If true, this section will cast shadows. */
	UPROPERTY()
	bool bCastShadow;

	/** Default values. */
	FMeshSectionInfo()
		: MaterialIndex(0)
		, bEnableCollision(true)
		, bCastShadow(true)
	{
	}

	/** Default values with an explicit material index. */
	explicit FMeshSectionInfo(int32 InMaterialIndex)
		: MaterialIndex(InMaterialIndex)
		, bEnableCollision(true)
		, bCastShadow(true)
	{
	}
};

/** Comparison for mesh section info. */
bool operator==(const FMeshSectionInfo& A, const FMeshSectionInfo& B);
bool operator!=(const FMeshSectionInfo& A, const FMeshSectionInfo& B);

/**
 * Map containing per-section settings for each section of each LOD.
 */
USTRUCT()
struct FMeshSectionInfoMap
{
	GENERATED_USTRUCT_BODY()

	/** Maps an LOD+Section to the material it should render with. */
	TMap<uint32,FMeshSectionInfo> Map;

	/** Serialize. */
	void Serialize(FArchive& Ar);

	/** Clears all entries in the map resetting everything to default. */
	ENGINE_API void Clear();

	/** Gets per-section settings for the specified LOD + section. */
	ENGINE_API FMeshSectionInfo Get(int32 LODIndex, int32 SectionIndex) const;

	/** Sets per-section settings for the specified LOD + section. */
	ENGINE_API void Set(int32 LODIndex, int32 SectionIndex, FMeshSectionInfo Info);

	/** Resets per-section settings for the specified LOD + section to defaults. */
	ENGINE_API void Remove(int32 LODIndex, int32 SectionIndex);

	/** Copies per-section settings from the specified section info map. */
	ENGINE_API void CopyFrom(const FMeshSectionInfoMap& Other);

	/** Returns true if any section has collision enabled. */
	bool AnySectionHasCollision() const;
};

UCLASS(HeaderGroup=StaticMesh, collapsecategories, hidecategories=Object, customconstructor, MinimalAPI, BlueprintType, config=Engine)
class UStaticMesh : public UObject, public IInterface_CollisionDataProvider, public IInterface_AssetUserData
{
	GENERATED_UCLASS_BODY()

	/** Pointer to the data used to render this static mesh. */
	TScopedPointer<class FStaticMeshRenderData> RenderData;

#if WITH_EDITORONLY_DATA
	/** Imported raw mesh bulk data. */
	UPROPERTY()
	TArray<FStaticMeshSourceModel> SourceModels;

	/** Map of LOD+Section index to per-section info. */
	FMeshSectionInfoMap SectionInfoMap;

	/** The pixel error allowed when computing auto LOD distances. */
	UPROPERTY()
	float AutoLODPixelError;

	/** The LOD group to which this mesh belongs. */
	UPROPERTY()
	FName LODGroup;

	/** If true, the distances at which LODs swap are computed automatically. */
	UPROPERTY()
	uint32 bAutoComputeLODDistance:1;
#endif // #if WITH_EDITORONLY_DATA

	/** Materials used by this static mesh. Individual sections index in to this array. */
	UPROPERTY()
	TArray<UMaterialInterface*> Materials;

	UPROPERTY(EditAnywhere, Category=StaticMesh, meta=(ToolTip="The light map resolution", FixedIncrement="4.0"))
	int32 LightMapResolution;

	/** The light map coordinate index */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=StaticMesh, meta=(ToolTip="The light map coordinate index"))
	int32 LightMapCoordinateIndex;

	// Physics data.
	UPROPERTY(EditAnywhere, transient, duplicatetransient, editinline, Category=StaticMesh)
	class UBodySetup* BodySetup;

	// Artist-accessible options.
	UPROPERTY()
	uint32 UseFullPrecisionUVs_DEPRECATED:1;

	/** True if mesh should use a less-conservative method of mip LOD texture factor computation.
		requires mesh to be resaved to take effect as algorithm is applied on save. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=StaticMesh, meta=(ToolTip="If true, use a less-conservative method of mip LOD texture factor computation.  Requires mesh to be resaved to take effect as algorithm is applied on save"))
	uint32 bUseMaximumStreamingTexelRatio:1;

	/** If true, strips unwanted complex collision data aka kDOP tree when cooking for consoles.
		On the Playstation 3 data of this mesh will be stored in video memory. */
	UPROPERTY()
	uint32 bStripComplexCollisionForConsole_DEPRECATED:1;

	/** If true, mesh will have NavCollision property with additional data for navmesh generation and usage.
	    Set to false for distant meshes (always outside navigation bounds) to save memory on collision data. */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=Navigation)
	uint32 bHasNavigationData:1;

	/**
	 * Allows artists to adjust the distance where textures using UV 0 are streamed in/out.
	 * 1.0 is the default, whereas a higher value increases the streamed-in resolution.
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category=StaticMesh)
	float StreamingDistanceMultiplier;

	/** Bias multiplier for Light Propagation Volume lighting */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=StaticMesh, meta=(UIMin = "0.0", UIMax = "3.0"))
	float LpvBiasMultiplier;

	/** A fence which is used to keep track of the rendering thread releasing the static mesh resources. */
	FRenderCommandFence ReleaseResourcesFence;

	/**
	 * For simplified meshes, this is the fully qualified path and name of the static mesh object we were
	 * originally duplicated from.  This is serialized to disk, but is discarded when cooking for consoles.
	 */
	FString HighResSourceMeshName;

#if WITH_EDITORONLY_DATA
	/** Default settings when using this mesh for instanced foliage */
	UPROPERTY(EditAnywhere, AdvancedDisplay, editinline, Category=StaticMesh)
	class UInstancedFoliageSettings* FoliageDefaultSettings;

	/** Importing data and options used for this mesh */
	UPROPERTY(VisibleAnywhere, editinline, Category=ImportSettings)
	class UAssetImportData* AssetImportData;

	/** Path to the resource used to construct this static mesh */
	UPROPERTY()
	FString SourceFilePath_DEPRECATED;

	/** Date/Time-stamp of the file from the last import */
	UPROPERTY()
	FString SourceFileTimestamp_DEPRECATED;

	/** Information for thumbnail rendering */
	UPROPERTY()
	class UThumbnailInfo* ThumbnailInfo;
#endif // WITH_EDITORONLY_DATA

	/** For simplified meshes, this is the CRC of the high res mesh we were originally duplicated from. */
	uint32 HighResSourceMeshCRC;

	/** Unique ID for tracking/caching this mesh during distributed lighting */
	FGuid LightingGuid;

	/**
	 *	Array of named socket locations, set up in editor and used as a shortcut instead of specifying
	 *	everything explicitly to AttachComponent in the StaticMeshComponent.
	 */
	UPROPERTY()
	TArray<class UStaticMeshSocket*> Sockets;

	/** Data that is only available if this static mesh is an imported SpeedTree */
	TSharedPtr<class FSpeedTreeWind> SpeedTreeWind;

protected:
	/**
	 * Index of an element to ignore while gathering streaming texture factors.
	 * This is useful to disregard automatically generated vertex data which breaks texture factor heuristics.
	 */
	UPROPERTY()
	int32 ElementToIgnoreForTexFactor;

	/** Array of user data stored with the asset */
	UPROPERTY(EditAnywhere, AdvancedDisplay, editinline, Category=StaticMesh)
	TArray<UAssetUserData*> AssetUserData;

public:
	/** Pre-build navigation collision */
	UPROPERTY(VisibleAnywhere, transient, duplicatetransient, editinline, Category=Navigation)
	class UNavCollision* NavCollision;
	
public:
	/**
	 * Default constructor
	 */
	ENGINE_API UStaticMesh(const class FPostConstructInitializeProperties& PCIP);

	// Begin UObject interface.
#if WITH_EDITOR
	ENGINE_API virtual void PreEditChange(UProperty* PropertyAboutToChange) OVERRIDE;
	ENGINE_API virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	ENGINE_API virtual void Serialize(FArchive& Ar) OVERRIDE;
	ENGINE_API virtual void PostLoad() OVERRIDE;
	ENGINE_API virtual void BeginDestroy() OVERRIDE;
	ENGINE_API virtual bool IsReadyForFinishDestroy() OVERRIDE;
	ENGINE_API virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;
	ENGINE_API virtual FString GetDesc() OVERRIDE;
	ENGINE_API virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface.

	/**
	 * Rebuilds renderable data for this static mesh.
	 * @param bSilent - If true will not popup a progress dialog.
	 */
	ENGINE_API void Build(bool bSilent = false);

	/**
	 * Initialize the static mesh's render resources.
	 */
	ENGINE_API virtual void InitResources();

	/**
	 * Releases the static mesh's render resources.
	 */
	ENGINE_API virtual void ReleaseResources();

	/**
	 * Returns the scale dependent texture factor used by the texture streaming code.
	 *
	 * @param RequestedUVIndex UVIndex to look at
	 * @return scale dependent texture factor
	 */
	float GetStreamingTextureFactor( int32 RequestedUVIndex );

	/**
	 * Returns the number of vertices for the specified LOD.
	 */
	ENGINE_API int32 GetNumVertices(int32 LODIndex) const;

	/**
	 * Returns the number of LODs used by the mesh.
	 */
	ENGINE_API int32 GetNumLODs() const;

	/**
	 * Returns the number of LODs used by the mesh.
	 */
	ENGINE_API bool HasValidRenderData() const;

	/**
	 * Returns the number of bounds of the mesh.
	 */
	ENGINE_API FBoxSphereBounds GetBounds() const;

	/**
	 * Gets a Material given a Material Index and an LOD number
	 *
	 * @return Requested material
	 */
	ENGINE_API UMaterialInterface* GetMaterial(int32 MaterialIndex) const;

	/**
	 * Returns the render data to use for exporting the specified LOD. This method should always
	 * be called when exporting a static mesh.
	 */
	ENGINE_API struct FStaticMeshLODResources& GetLODForExport( int32 LODIndex );

	/**
	 * Static: Processes the specified static mesh for light map UV problems
	 *
	 * @param	InStaticMesh					Static mesh to process
	 * @param	InOutAssetsWithMissingUVSets	Array of assets that we found with missing UV sets
	 * @param	InOutAssetsWithBadUVSets		Array of assets that we found with bad UV sets
	 * @param	InOutAssetsWithValidUVSets		Array of assets that we found with valid UV sets
	 * @param	bInVerbose						If true, log the items as they are found
	 */
	ENGINE_API static void CheckLightMapUVs( UStaticMesh* InStaticMesh, TArray< FString >& InOutAssetsWithMissingUVSets, TArray< FString >& InOutAssetsWithBadUVSets, TArray< FString >& InOutAssetsWithValidUVSets, bool bInVerbose = true );

	// Begin Interface_CollisionDataProvider Interface
	ENGINE_API virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) OVERRIDE;
	ENGINE_API virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const OVERRIDE;
	virtual bool WantsNegXTriMesh() OVERRIDE
	{
		return true;
	}
	ENGINE_API virtual void GetMeshId(FString& OutMeshId) OVERRIDE;
	// End Interface_CollisionDataProvider Interface

	// Begin IInterface_AssetUserData Interface
	virtual void AddAssetUserData(UAssetUserData* InUserData) OVERRIDE;
	virtual void RemoveUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass) OVERRIDE;
	virtual UAssetUserData* GetAssetUserDataOfClass(TSubclassOf<UAssetUserData> InUserDataClass) OVERRIDE;
	virtual const TArray<UAssetUserData*>* GetAssetUserDataArray() const OVERRIDE;
	// End IInterface_AssetUserData Interface


	/**
	 * Create BodySetup for this staticmesh if it doesn't have one
	 */
	ENGINE_API void CreateBodySetup();

	/**
	 * Calculates navigation collision for caching
	 */
	ENGINE_API void CreateNavCollision();

	const FGuid& GetLightingGuid() const
	{
#if WITH_EDITORONLY_DATA
		return LightingGuid;
#else
		static const FGuid NullGuid( 0, 0, 0, 0 );
		return NullGuid;
#endif // WITH_EDITORONLY_DATA
	}

	void SetLightingGuid()
	{
#if WITH_EDITORONLY_DATA
		LightingGuid = FGuid::NewGuid();
#endif // WITH_EDITORONLY_DATA
	}

	/**
	 *	Find a socket object in this StaticMesh by name.
	 *	Entering NAME_None will return NULL. If there are multiple sockets with the same name, will return the first one.
	 */
	ENGINE_API class UStaticMeshSocket* FindSocket(FName InSocketName);
	/**
	 * Returns vertex color data by position.
	 * For matching to reimported meshes that may have changed or copying vertex paint data from mesh to mesh.
	 *
	 *	@param	VertexColorData		(out)A map of vertex position data and its color. The method fills this map.
	 */
	ENGINE_API void GetVertexColorData(TMap<FVector, FColor>& VertexColorData);

	/**
	 * Sets vertex color data by position.
	 * Map of vertex color data by position is matched to the vertex position in the mesh
	 * and nearest matching vertex color is used.
	 *
	 *	@param	VertexColorData		A map of vertex position data and color.
	 */
	ENGINE_API void SetVertexColorData(const TMap<FVector, FColor>& VertexColorData);

	void EnforceLightmapRestrictions();

#if WITH_EDITORONLY_DATA

	/**
	 * Returns true if LODs of this static mesh may share texture lightmaps.
	 */
	bool CanLODsShareStaticLighting() const;

	/**
	 * Retrieves the names of all LOD groups.
	 */
	ENGINE_API static void GetLODGroups(TArray<FName>& OutLODGroups);

	/**
	 * Retrieves the localized display names of all LOD groups.
	 */
	ENGINE_API static void GetLODGroupsDisplayNames(TArray<FText>& OutLODGroupsDisplayNames);

private:
	/**
	 * Serializes in legacy source data and constructs the necessary source models.
	 */
	void SerializeLegacySouceData(class FArchive& Ar, const struct FBoxSphereBounds& LegacyBounds);

	/**
	 * Fixes up static meshes that were imported with sections that had zero triangles.
	 */
	void FixupZeroTriangleSections();

	/**
	 * Caches derived renderable data.
	 */
	void CacheDerivedData();
#endif // #if WITH_EDITORONLY_DATA
};
