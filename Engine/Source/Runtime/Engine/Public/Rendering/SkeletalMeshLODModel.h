// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "PackedNormal.h"
#include "Components.h"
#include "GPUSkinPublicDefs.h"
#include "BoneIndices.h"
#include "Serialization/BulkData.h"
#include "SkeletalMeshTypes.h"


//
//	FSoftSkinVertex
//

struct FSoftSkinVertex
{
	FVector			Position;

	// Tangent, U-direction
	FPackedNormal	TangentX;
	// Binormal, V-direction
	FPackedNormal	TangentY;
	// Normal
	FPackedNormal	TangentZ;

	// UVs
	FVector2D		UVs[MAX_TEXCOORDS];
	// VertexColor
	FColor			Color;
	uint8			InfluenceBones[MAX_TOTAL_INFLUENCES];
	uint8			InfluenceWeights[MAX_TOTAL_INFLUENCES];

	/** If this vert is rigidly weighted to a bone, return true and the bone index. Otherwise return false. */
	ENGINE_API bool GetRigidWeightBone(uint8& OutBoneIndex) const;

	/** Returns the maximum weight of any bone that influences this vertex. */
	ENGINE_API uint8 GetMaximumWeight() const;

	/**
	* Serializer
	*
	* @param Ar - archive to serialize with
	* @param V - vertex to serialize
	* @return archive that was used
	*/
	friend FArchive& operator<<(FArchive& Ar, FSoftSkinVertex& V);
};



/**
* A set of skeletal mesh triangles which use the same material
*/
struct FSkelMeshSection
{
	/** Material (texture) used for this section. */
	uint16 MaterialIndex;

	/** The offset of this section's indices in the LOD's index buffer. */
	uint32 BaseIndex;

	/** The number of triangles in this section. */
	uint32 NumTriangles;

	/** Is this mesh selected? */
	uint8 bSelected : 1;

	/** This section will recompute tangent in runtime */
	bool bRecomputeTangent;

	/** This section will cast shadow */
	bool bCastShadow;

	/** This is set for old 'duplicate' sections that need to get removed on load */
	bool bLegacyClothingSection_DEPRECATED;

	/** Corresponding Section Index will be enabled when this section is disabled
	because corresponding cloth section will be shown instead of this
	or disabled section index when this section is enabled for cloth simulation
	*/
	int16 CorrespondClothSectionIndex_DEPRECATED;

	/** Decide whether enabling clothing LOD for this section or not, just using skelmesh LOD_0's one to decide */
	/** no need anymore because each clothing LOD will be assigned to each mesh LOD  */
	uint8 bEnableClothLOD_DEPRECATED;

	/** The offset into the LOD's vertex buffer of this section's vertices. */
	uint32 BaseVertexIndex;

	/** The soft vertices of this section. */
	TArray<FSoftSkinVertex> SoftVertices;

	/** The extra vertex data for mapping to an APEX clothing simulation mesh. */
	TArray<FMeshToMeshVertData> ClothMappingData;

	/** The bones which are used by the vertices of this section. Indices of bones in the USkeletalMesh::RefSkeleton array */
	TArray<FBoneIndexType> BoneMap;

	/** Number of vertices in this section (size of SoftVertices array). Available in non-editor builds. */
	int32 NumVertices;

	/** max # of bones used to skin the vertices in this section */
	int32 MaxBoneInfluences;

	// INDEX_NONE if not set
	int16 CorrespondClothAssetIndex;

	/** Clothing data for this section, clothing is only present if ClothingData.IsValid() returns true */
	FClothingSectionData ClothingData;

	/** Map between a vertex index and all vertices that share the same position **/
	TMap<int32, TArray<int32>> OverlappingVertices;

	/** If disabled, we won't render this section */
	bool bDisabled;

	FSkelMeshSection()
		: MaterialIndex(0)
		, BaseIndex(0)
		, NumTriangles(0)
		, bSelected(false)
		, bRecomputeTangent(false)
		, bCastShadow(true)
		, bLegacyClothingSection_DEPRECATED(false)
		, CorrespondClothSectionIndex_DEPRECATED(-1)
		, BaseVertexIndex(0)
		, NumVertices(0)
		, MaxBoneInfluences(4)
		, CorrespondClothAssetIndex(INDEX_NONE)
		, bDisabled(false)
	{}


	/**
	* @return total num rigid verts for this section
	*/
	FORCEINLINE int32 GetNumVertices() const
	{
		// Either SoftVertices should be empty, or size should match NumVertices
		check(SoftVertices.Num() == 0 || SoftVertices.Num() == NumVertices);
		return NumVertices;
	}

	/**
	* @return starting index for rigid verts for this section in the LOD vertex buffer
	*/
	FORCEINLINE int32 GetVertexBufferIndex() const
	{
		return BaseVertexIndex;
	}

	/**
	* @return TRUE if we have cloth data for this section
	*/
	FORCEINLINE bool HasClothingData() const
	{
		return (ClothMappingData.Num() > 0);
	}

	/**
	* Calculate max # of bone influences used by this skel mesh section
	*/
	ENGINE_API void CalcMaxBoneInfluences();

	FORCEINLINE bool HasExtraBoneInfluences() const
	{
		return MaxBoneInfluences > MAX_INFLUENCES_PER_STREAM;
	}

	// Serialization.
	friend FArchive& operator<<(FArchive& Ar, FSkelMeshSection& S);
};

/**
* All data to define a certain LOD model for a skeletal mesh.
*/
class FSkeletalMeshLODModel
{
public:
	/** Sections. */
	TArray<FSkelMeshSection> Sections;

	uint32						NumVertices;
	/** The number of unique texture coordinate sets in this lod */
	uint32						NumTexCoords;

	/** Index buffer, covering all sections */
	TArray<uint32> IndexBuffer;

	/**
	* Bone hierarchy subset active for this LOD.
	* This is a map between the bones index of this LOD (as used by the vertex structs) and the bone index in the reference skeleton of this SkeletalMesh.
	*/
	TArray<FBoneIndexType> ActiveBoneIndices;

	/**
	* Bones that should be updated when rendering this LOD. This may include bones that are not required for rendering.
	* All parents for bones in this array should be present as well - that is, a complete path from the root to each bone.
	* For bone LOD code to work, this array must be in strictly increasing order, to allow easy merging of other required bones.
	*/
	TArray<FBoneIndexType> RequiredBones;

	/** Mapping from final mesh vertex index to raw import vertex index. Needed for vertex animation, which only stores positions for import verts. */
	TArray<int32>				MeshToImportVertexMap;
	/** The max index in MeshToImportVertexMap, ie. the number of imported (raw) verts. */
	int32						MaxImportVertex;

	/** Editor only data: array of the original point (wedge) indices for each of the vertices in a FSkeletalMeshLODModel */
	FIntBulkData				RawPointIndices;
	FWordBulkData				LegacyRawPointIndices;


	/** Constructor (default) */
	FSkeletalMeshLODModel()
		: NumVertices(0)
		, NumTexCoords(0)
		, MaxImportVertex(-1)
	{
	}

	/**
	* Special serialize function passing the owning UObject along as required by FUnytpedBulkData
	* serialization.
	*
	* @param	Ar		Archive to serialize with
	* @param	Owner	UObject this structure is serialized within
	* @param	Idx		Index of current array entry being serialized
	*/
	void Serialize(FArchive& Ar, UObject* Owner, int32 Idx);

	/**
	* Fill array with vertex position and tangent data from skel mesh chunks.
	*
	* @param Vertices Array to fill.
	*/
	ENGINE_API void GetVertices(TArray<FSoftSkinVertex>& Vertices) const;

	/**
	* Fill array with APEX cloth mapping data.
	*
	* @param MappingData Array to fill.
	*/
	void GetClothMappingData(TArray<FMeshToMeshVertData>& MappingData, TArray<uint64>& OutClothIndexMapping) const;



	/** Utility for finding the section that a particular vertex is in. */
	ENGINE_API void GetSectionFromVertexIndex(int32 InVertIndex, int32& OutSectionIndex, int32& OutVertIndex) const;

	/**
	* @return true if any chunks have cloth data.
	*/
	bool HasClothData() const;

	ENGINE_API int32 NumNonClothingSections() const;

	ENGINE_API int32 GetNumNonClothingVertices() const;

	/**
	* Similar to GetVertices but ignores vertices from clothing sections
	* to avoid getting duplicate vertices from clothing sections if not needed
	*
	* @param OutVertices Array to fill
	*/
	ENGINE_API void GetNonClothVertices(TArray<FSoftSkinVertex>& OutVertices) const;

	bool DoSectionsNeedExtraBoneInfluences() const;

	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize) const;
};

#endif // WITH_EDITOR