// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Contains the shared data that is used by all SkeletalMeshComponents (instances).
 */

#include "SkeletalMeshTypes.h"
#include "Animation/PreviewAssetAttachComponent.h"
#include "SkeletalMesh.generated.h"

class UMorphTarget;

#if WITH_APEX_CLOTHING
struct FApexClothCollisionVolumeData;

namespace physx
{
	namespace apex
	{
		class NxClothingAsset;
	}
}
#endif

/** Enum specifying the importance of properties when simplifying skeletal meshes. */
UENUM()
enum SkeletalMeshOptimizationImportance
{	
	SMOI_Off,
	SMOI_Lowest,
	SMOI_Low,
	SMOI_Normal,
	SMOI_High,
	SMOI_Highest,
	SMOI_MAX,
};

/** Enum specifying the reduction type to use when simplifying skeletal meshes. */
UENUM()
enum SkeletalMeshOptimizationType
{
	SMOT_NumOfTriangles,
	SMOT_MaxDeviation,
	SMOT_MAX,
};

USTRUCT()
struct FBoneMirrorInfo
{
	GENERATED_USTRUCT_BODY()

	/** The bone to mirror. */
	UPROPERTY(EditAnywhere, Category=BoneMirrorInfo, meta=(ArrayClamp = "RefSkeleton"))
	int32 SourceIndex;

	/** Axis the bone is mirrored across. */
	UPROPERTY(EditAnywhere, Category=BoneMirrorInfo)
	TEnumAsByte<EAxis::Type> BoneFlipAxis;

	FBoneMirrorInfo()
		: SourceIndex(0)
		, BoneFlipAxis(0)
	{
	}

};

/** Structure to export/import bone mirroring information */
USTRUCT()
struct FBoneMirrorExport
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=BoneMirrorExport)
	FName BoneName;

	UPROPERTY(EditAnywhere, Category=BoneMirrorExport)
	FName SourceBoneName;

	UPROPERTY(EditAnywhere, Category=BoneMirrorExport)
	TEnumAsByte<EAxis::Type> BoneFlipAxis;


	FBoneMirrorExport()
		: BoneFlipAxis(0)
	{
	}

};

/** Struct containing triangle sort settings for a particular section */
USTRUCT()
struct FTriangleSortSettings
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=TriangleSortSettings)
	TEnumAsByte<enum ETriangleSortOption> TriangleSorting;

	UPROPERTY(EditAnywhere, Category=TriangleSortSettings)
	TEnumAsByte<enum ETriangleSortAxis> CustomLeftRightAxis;

	UPROPERTY(EditAnywhere, Category=TriangleSortSettings)
	FName CustomLeftRightBoneName;


	FTriangleSortSettings()
		: TriangleSorting(0)
		, CustomLeftRightAxis(0)
	{
	}

};

/** Struct containing information for a particular LOD level, such as materials and info for when to use it. */
USTRUCT()
struct FSkeletalMeshLODInfo
{
	GENERATED_USTRUCT_BODY()

	/**	Indicates when to use this LOD. A smaller number means use this LOD when further away. */
	UPROPERTY(EditAnywhere, Category=SkeletalMeshLODInfo)
	float DisplayFactor;

	/**	Used to avoid 'flickering' when on LOD boundary. Only taken into account when moving from complex->simple. */
	UPROPERTY(EditAnywhere, Category=SkeletalMeshLODInfo)
	float LODHysteresis;

	/** Mapping table from this LOD's materials to the USkeletalMesh materials array. */
	UPROPERTY(EditAnywhere, editfixedsize, Category=SkeletalMeshLODInfo)
	TArray<int32> LODMaterialMap;

	/** Per-section control over whether to enable shadow casting. */
	UPROPERTY()
	TArray<bool> bEnableShadowCasting_DEPRECATED;

	/** Per-section sorting options */
	UPROPERTY()
	TArray<TEnumAsByte<enum ETriangleSortOption> > TriangleSorting_DEPRECATED;

	UPROPERTY(EditAnywhere, editfixedsize, Category=SkeletalMeshLODInfo)
	TArray<struct FTriangleSortSettings> TriangleSortSettings;

	/** Whether to disable morph targets for this LOD. */
	UPROPERTY()
	uint32 bHasBeenSimplified:1;


	FSkeletalMeshLODInfo()
		: DisplayFactor(0)
		, LODHysteresis(0)
		, bHasBeenSimplified(false)
	{
	}

};

/**
 * FSkeletalMeshOptimizationSettings - The settings used to optimize a skeletal mesh LOD.
 */
USTRUCT()
struct FSkeletalMeshOptimizationSettings
{
	GENERATED_USTRUCT_BODY()

	/** The method to use when optimizing the skeletal mesh LOD */
	UPROPERTY()
	TEnumAsByte<enum SkeletalMeshOptimizationType> ReductionMethod;

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

	/** How important the shape of the geometry is. */
	UPROPERTY()
	TEnumAsByte<enum SkeletalMeshOptimizationImportance> SilhouetteImportance;

	/** How important texture density is. */
	UPROPERTY()
	TEnumAsByte<enum SkeletalMeshOptimizationImportance> TextureImportance;

	/** How important shading quality is. */
	UPROPERTY()
	TEnumAsByte<enum SkeletalMeshOptimizationImportance> ShadingImportance;

	/** How important skinning quality is. */
	UPROPERTY()
	TEnumAsByte<enum SkeletalMeshOptimizationImportance> SkinningImportance;

	/** The ratio of bones that will be removed from the mesh */
	UPROPERTY()
	float BoneReductionRatio;

	/** Maximum number of bones that can be assigned to each vertex. */
	UPROPERTY()
	int32 MaxBonesPerVertex;

	FSkeletalMeshOptimizationSettings()
		: ReductionMethod(SMOT_MaxDeviation)
		, NumOfTrianglesPercentage(1.0f)
		, MaxDeviationPercentage(0)
		, WeldingThreshold(0.1f)
		, bRecalcNormals(true)
		, NormalsThreshold(60.0f)
		, SilhouetteImportance(SMOI_Normal)
		, TextureImportance(SMOI_Normal)
		, ShadingImportance(SMOI_Normal)
		, SkinningImportance(SMOI_Normal)
		, BoneReductionRatio(100.0f)
		, MaxBonesPerVertex(4)
	{
	}

};

USTRUCT()
struct FMorphTargetMap
{
	GENERATED_USTRUCT_BODY()

	/** The bone to mirror. */
	UPROPERTY(EditAnywhere, Category=MorphTargetMap)
	FName Name;

	/** Axis the bone is mirrored across. */
	UPROPERTY(EditAnywhere, Category=MorphTargetMap)
	UMorphTarget* MorphTarget;


	FMorphTargetMap()
		: Name(NAME_None)
		, MorphTarget(NULL)
	{
	}

	FMorphTargetMap( FName InName, UMorphTarget * InMorphTarget )
		: Name(InName)
		, MorphTarget(InMorphTarget)
	{
	}

	bool operator== (const FMorphTargetMap& Other) const
	{
		return (Name==Other.Name && MorphTarget == Other.MorphTarget);
	}
};

#if WITH_APEX_CLOTHING
class FClothingAssetWrapper
{
public:

	FClothingAssetWrapper(physx::apex::NxClothingAsset* InApexClothingAsset)
	:	ApexClothingAsset(InApexClothingAsset),bValid(true)
	{}

	ENGINE_API ~FClothingAssetWrapper();

	physx::apex::NxClothingAsset* GetAsset()
	{
		return ApexClothingAsset;
	}

	//returns bone name converted to fbx style
	FName GetConvertedBoneName(int32 BoneIndex);


	bool	IsValid()
	{ 
		return bValid;
	}

	void	SetValid(bool valid)
	{
		bValid = valid;
	}

private:
	physx::apex::NxClothingAsset* ApexClothingAsset;
	bool						  bValid;
};
#endif // #if WITH_APEX_CLOTHING

USTRUCT()
struct FClothingAssetData
{
	GENERATED_USTRUCT_BODY()

	/* User-defined asset name */
	UPROPERTY(EditAnywhere, Category=ClothingAssetData)
	FName AssetName;

	UPROPERTY(EditAnywhere, Category=ClothingAssetData)
	FString	ApexFileName;

#if WITH_APEX_CLOTHING
	TSharedPtr<FClothingAssetWrapper> ApexClothingAsset;

	/* Collision volume data for showing to the users whether collision shape is correct or not */
	TArray<FApexClothCollisionVolumeData> ClothCollisionVolumes;
	TArray<uint32> ClothCollisionConvexPlaneIndices;
	TArray<FPlane> ClothCollisionVolumePlanes;
	TArray<FApexClothBoneSphereData> ClothBoneSpheres;
	TArray<uint16> BoneSphereConnections;
#endif// #if WITH_APEX_CLOTHING

	// serialization
	friend FArchive& operator<<(FArchive& Ar, FClothingAssetData& A);
};

// Material interface for USkeletalMesh - contains a material and a shadow casting flag
USTRUCT()
struct FSkeletalMaterial
{
	GENERATED_USTRUCT_BODY()

	FSkeletalMaterial()
		: MaterialInterface( NULL )
		, bEnableShadowCasting( true )
	{

	}

	FSkeletalMaterial( class UMaterialInterface* InMaterialInterface, bool bInEnableShadowCasting = true )
		: MaterialInterface( InMaterialInterface )
		, bEnableShadowCasting( bInEnableShadowCasting )
	{

	}

	friend FArchive& operator<<( FArchive& Ar, FSkeletalMaterial& Elem );

	ENGINE_API friend bool operator==( const FSkeletalMaterial& LHS, const FSkeletalMaterial& RHS );
	ENGINE_API friend bool operator==( const FSkeletalMaterial& LHS, const UMaterialInterface& RHS );
	ENGINE_API friend bool operator==( const UMaterialInterface& LHS, const FSkeletalMaterial& RHS );

	UPROPERTY(EditAnywhere, BlueprintReadOnly, transient, Category=SkeletalMesh)
	class UMaterialInterface *	MaterialInterface;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=SkeletalMesh, Category=SkeletalMesh)
	bool						bEnableShadowCasting;
};

class FSkeletalMeshResource;

UCLASS(HeaderGroup=SkeletalMesh, hidecategories=Object, MinimalAPI, BlueprintType)
class USkeletalMesh : public UObject
{
	GENERATED_UCLASS_BODY()

private:
	/** Rendering resources created at import time. */
	TSharedPtr<FSkeletalMeshResource> ImportedResource;

public:
	/** Get the default resource for this skeletal mesh. */
	FORCEINLINE FSkeletalMeshResource* GetImportedResource() const { return ImportedResource.Get(); }

	/** Get the resource to use for rendering. */
	FORCEINLINE FSkeletalMeshResource* GetResourceForRendering(ERHIFeatureLevel::Type FeatureLevel) const { return GetImportedResource(); }

	/** Skeleton of this skeletal mesh **/
	UPROPERTY(Category=Mesh, AssetRegistrySearchable, VisibleAnywhere, BlueprintReadOnly)
	class USkeleton* Skeleton;

	UPROPERTY(transient, duplicatetransient)
	FBoxSphereBounds Bounds;

	/** List of materials applied to this mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, transient, duplicatetransient, Category=SkeletalMesh)
	TArray<FSkeletalMaterial> Materials;

	/** List of bones that should be mirrored. */
	UPROPERTY(EditAnywhere, editfixedsize, Category=Mirroring)
	TArray<struct FBoneMirrorInfo> SkelMirrorTable;

	UPROPERTY(EditAnywhere, Category=Mirroring)
	TEnumAsByte<EAxis::Type> SkelMirrorAxis;

	UPROPERTY(EditAnywhere, Category=Mirroring)
	TEnumAsByte<EAxis::Type> SkelMirrorFlipAxis;

	/** Struct containing information for each LOD level, such as materials to use, and when use the LOD. */
	UPROPERTY(EditAnywhere, EditFixedSize, Category=LevelOfDetail)
	TArray<struct FSkeletalMeshLODInfo> LODInfo;

	/** If true, use 32 bit UVs. If false, use 16 bit UVs to save memory */
	UPROPERTY(EditAnywhere, Category=SkeletalMesh)
	uint32 bUseFullPrecisionUVs:1;

	/** true if this mesh has ever been simplified with Simplygon. */
	UPROPERTY()
	uint32 bHasBeenSimplified:1;

	/** Whether or not the mesh has vertex colors */
	UPROPERTY()
	uint32 bHasVertexColors:1;

	/**
	 *	Physics and collision information used for this USkeletalMesh, set up in PhAT.
	 *	This is used for per-bone hit detection, accurate bounding box calculation and ragdoll physics for example.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Physics)
	class UPhysicsAsset* PhysicsAsset;

#if WITH_EDITORONLY_DATA
	/** Asset used for previewing bounds in AnimSetViewer. Makes setting up LOD distance factors more reliable. 
	 *  Removing this and use PhysicsAsset for preview from now on, but meanwhile, we will copy this info to PhysicsAsset (below)
	 */
	UPROPERTY()
	class UPhysicsAsset* BoundsPreviewAsset_DEPRECATED;

	/** Importing data and options used for this mesh */
	UPROPERTY(EditAnywhere, editinline, Category=Reimport)
	class UAssetImportData* AssetImportData;

	/** Path to the resource used to construct this skeletal mesh */
	UPROPERTY()
	FString SourceFilePath_DEPRECATED;

	/** Date/Time-stamp of the file from the last import */
	UPROPERTY()
	FString SourceFileTimestamp_DEPRECATED;

	/** Information for thumbnail rendering */
	UPROPERTY()
	class UThumbnailInfo* ThumbnailInfo;

	/** Optimization settings used to simplify LODs of this mesh. */
	UPROPERTY()
	TArray<struct FSkeletalMeshOptimizationSettings> OptimizationSettings;

	/* Attached assets component for this mesh */
	UPROPERTY()
	FPreviewAssetAttachContainer PreviewAttachedAssetContainer;
#endif // WITH_EDITORONLY_DATA

	/**
	 * Allows artists to adjust the distance where textures using UV 0 are streamed in/out.
	 * 1.0 is the default, whereas a higher value increases the streamed-in resolution.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=TextureStreaming)
	float StreamingDistanceMultiplier;

	/** This is serialized right now with strong pointer to Morph Target
	 * Later on when memory is concerned, this can be transient array that is built based on MorphTarget loaded 
	 */
	UPROPERTY()
	TArray<FMorphTargetMap> MorphTargetTable_DEPRECATED;

	UPROPERTY(Category=Mesh, BlueprintReadWrite)
	TArray<UMorphTarget*> MorphTargets;

	/** A fence which is used to keep track of the rendering thread releasing the static mesh resources. */
	FRenderCommandFence ReleaseResourcesFence;

	/** New Reference skeleton type **/
	FReferenceSkeleton RefSkeleton;

	/** Map of morph target to name **/
	TMap<FName, UMorphTarget*> MorphTargetIndexMap;

	/** Reference skeleton precomputed bases. */
	TArray<FMatrix> RefBasesInvMatrix;    // @todo: wasteful ?!

#if WITH_EDITORONLY_DATA
	/** The section currently selected in the Editor. */
	UPROPERTY(transient)
	int32 SelectedEditorSection;
	/** The section currently selected for clothing. need to remember this index for reimporting cloth */
	UPROPERTY(transient)
	int32 SelectedClothingSection;

	/** Height offset for the floor mesh in the editor */
	UPROPERTY()
	float FloorOffset;

#endif

	/** Clothing asset data */
	UPROPERTY(EditAnywhere, editfixedsize, BlueprintReadOnly, Category=Clothing)
	TArray<FClothingAssetData>		ClothingAssets;

private:
	/** Skeletal mesh source data */
	class FSkeletalMeshSourceData* SourceData;

	/** The cached streaming texture factors.  If the array doesn't have MAX_TEXCOORDS entries in it, the cache is outdated. */
	TArray<float> CachedStreamingTextureFactors;

	/** 
	 *	Array of named socket locations, set up in editor and used as a shortcut instead of specifying 
	 *	everything explicitly to AttachComponent in the SkeletalMeshComponent. 
	 */
	UPROPERTY()
	TArray<class USkeletalMeshSocket*> Sockets;

public:
	/**
	* Initialize the mesh's render resources.
	*/
	ENGINE_API void InitResources();

	/**
	* Releases the mesh's render resources.
	*/
	ENGINE_API void ReleaseResources();


	/** Release CPU access version of buffer */
	void ReleaseCPUResources();

	/**
	 * Returns the scale dependent texture factor used by the texture streaming code.	
	 *
	 * @param RequestedUVIndex UVIndex to look at
	 * @return scale dependent texture factor
	 */
	float GetStreamingTextureFactor( int32 RequestedUVIndex );

	/**
	 * Gets the center point from which triangles should be sorted, if any.
	 */
	ENGINE_API bool GetSortCenterPoint(FVector& OutSortCenter) const;

	/**
	 * Computes flags for building vertex buffers.
	 */
	uint32 GetVertexBufferFlags() const;

	// Begin UObject interface.
#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void PostEditUndo() OVERRIDE;
#endif // WITH_EDITOR
	virtual void BeginDestroy() OVERRIDE;
	virtual bool IsReadyForFinishDestroy() OVERRIDE;
	virtual void PreSave() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;	
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
	virtual FString GetDetailedInfoInternal() const OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject interface.

	/** Setup-only routines - not concerned with the instance. */

	ENGINE_API void CalculateInvRefMatrices();

	/** Calculate the required bones for a Skeletal Mesh LOD, including possible extra influences */
	ENGINE_API static void CalculateRequiredBones(class FStaticLODModel& LODModel, const struct FReferenceSkeleton& RefSkeleton, const TMap<FBoneIndexType, FBoneIndexType> * BonesToRemove);

	/** 
	 *	Find a socket object in this SkeletalMesh by name. 
	 *	Entering NAME_None will return NULL. If there are multiple sockets with the same name, will return the first one.
	 */
	ENGINE_API class USkeletalMeshSocket const* FindSocket(FName InSocketName) const;

	// @todo document
	ENGINE_API FMatrix GetRefPoseMatrix( int32 BoneIndex ) const;

	/** 
	 *	Get the component orientation of a bone or socket. Transforms by parent bones.
	 */
	ENGINE_API FMatrix GetComposedRefPoseMatrix( FName InBoneName ) const;

	/** Allocate and initialise bone mirroring table for this skeletal mesh. Default is source = destination for each bone. */
	void InitBoneMirrorInfo();

	/** Utility for copying and converting a mirroring table from another USkeletalMesh. */
	ENGINE_API void CopyMirrorTableFrom(USkeletalMesh* SrcMesh);
	ENGINE_API void ExportMirrorTable(TArray<FBoneMirrorExport> &MirrorExportInfo);
	ENGINE_API void ImportMirrorTable(TArray<FBoneMirrorExport> &MirrorExportInfo);

	/** 
	 *	Utility for checking that the bone mirroring table of this mesh is good.
	 *	Return true if mirror table is OK, false if there are problems.
	 *	@param	ProblemBones	Output string containing information on bones that are currently bad.
	 */
	ENGINE_API bool MirrorTableIsGood(FString& ProblemBones);

	/**
	 * Returns the mesh only socket list - this ignores any sockets in the skeleton
	 * Return value is a non-const reference so the socket list can be changed
	 */
	ENGINE_API TArray<USkeletalMeshSocket*>& GetMeshOnlySocketList();

#if WITH_EDITOR
	/** Retrieves the source model for this skeletal mesh. */
	ENGINE_API FStaticLODModel& GetSourceModel();

	/**
	 * Copies off the source model for this skeletal mesh if necessary and returns it. This function should always be called before
	 * making destructive changes to the mesh's geometry, e.g. simplification.
	 */
	ENGINE_API FStaticLODModel& PreModifyMesh();

	/**
	 * Returns the "active" socket list - all sockets from this mesh plus all non-duplicates from the skeleton
	 * Const ref return value as this cannot be modified externally
	 */
	ENGINE_API const TArray<USkeletalMeshSocket*>& GetActiveSocketList() const;

#endif // #if WITH_EDITOR

	/**
	* Verify SkeletalMeshLOD is set up correctly	
	*/
	void DebugVerifySkeletalMeshLOD();

	/**
	 * Find a named MorphTarget from the MorphSets array in the SkinnedMeshComponent.
	 * This searches the array in the same way as FindAnimSequence
	 *
	 * @param MorphTargetName Name of MorphTarget to look for.
	 *
	 * @return Pointer to found MorphTarget. Returns NULL if could not find target with that name.
	 */
	ENGINE_API UMorphTarget* FindMorphTarget( FName MorphTargetName );

	/** if name conflicts, it will overwrite the reference */
	ENGINE_API void RegisterMorphTarget(UMorphTarget* MorphTarget);

	ENGINE_API void UnregisterMorphTarget(UMorphTarget* MorphTarget);

	/** Initialize MorphSets look up table : MorphTargetIndexMap */
	ENGINE_API void InitMorphTargets();

#if WITH_APEX_CLOTHING
	ENGINE_API bool	 HasClothSections(int32 LODIndex,int AssetIndex);
	ENGINE_API void	 GetOriginSectionIndicesWithCloth(int32 LODIndex, TArray<uint32>& OutSectionIndices);
	ENGINE_API void	 GetOriginSectionIndicesWithCloth(int32 LODIndex, int32 AssetIndex, TArray<uint32>& OutSectionIndices);
	ENGINE_API void	 GetClothSectionIndices(int32 LODIndex, int32 AssetIndex, TArray<uint32>& OutSectionIndices);
	//moved from ApexClothingUtils because of compile issues
	ENGINE_API void  LoadClothCollisionVolumes(int32 AssetIndex, physx::apex::NxClothingAsset* ClothingAsset);
	ENGINE_API bool  IsEnabledClothLOD(int32 AssetIndex);
	ENGINE_API int32 GetClothAssetIndex(int32 SectionIndex);
#endif// #if WITH_APEX_CLOTHING

private:

#if WITH_EDITOR
	/** Utility function to help with building the combined socket list */
	bool IsSocketOnMesh( const FName& InSocketName ) const;
#endif

	/**
	* Flush current render state
	*/
	void FlushRenderState();
	/**
	* Restart render state. 
	*/
	void RestartRenderState();

	/**
	* In older data, the bEnableShadowCasting flag was stored in LODInfo
	* so it needs moving over to materials
	*/
	void MoveDeprecatedShadowFlagToMaterials();

	/**
	* Test whether all the flags in an array are identical (could be moved to Array.h?)
	*/
	bool AreAllFlagsIdentical( const TArray<bool>& BoolArray ) const;
};


/**
 * Refresh Physics Asset Change
 * 
 * Physics Asset has been changed, so it will need to recreate physics state to reflect it
 * Utilities functions to propagate new Physics Asset for InSkeletalMesh
 *
 * @param	InSkeletalMesh	SkeletalMesh that physics asset has been changed for
 */
ENGINE_API void RefreshSkelMeshOnPhysicsAssetChange(const USkeletalMesh * InSkeletalMesh);
