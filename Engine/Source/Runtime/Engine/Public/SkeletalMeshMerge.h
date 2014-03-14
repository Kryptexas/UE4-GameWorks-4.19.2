// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SkeletalMeshMerge.h: Merging of unreal skeletal mesh objects.
=============================================================================*/

/*-----------------------------------------------------------------------------
FSkeletalMeshMerge
-----------------------------------------------------------------------------*/

#pragma once

/** 
* Info to map all the sections from a single source skeletal mesh to 
* a final section entry int he merged skeletal mesh
*/
struct FSkelMeshMergeSectionMapping
{
	/** indices to final section entries of the merged skel mesh */
	TArray<int32> SectionIDs;
};

/** 
* Utility for merging a list of skeletal meshes into a single mesh
*/
class ENGINE_API FSkeletalMeshMerge
{
public:
	/**
	* Constructor
	* @param InMergeMesh - destination mesh to merge to
	* @param InSrcMeshList - array of source meshes to merge
	* @param InForceSectionMapping - optional array to map sections from the source meshes to merged section entries
	* @param StripTopLODs - number of high LODs to remove from input meshes
	*/
	FSkeletalMeshMerge( 
		USkeletalMesh* InMergeMesh, 
		const TArray<USkeletalMesh*>& InSrcMeshList, 
		const TArray<FSkelMeshMergeSectionMapping>& InForceSectionMapping,
		int32 StripTopLODs
		);

	/**
	* Merge/Composite the list of source meshes onto the merge one
	* @return true if succeeded
	*/
	bool DoMerge();

private:
	/** Destination merged mesh */
	USkeletalMesh* MergeMesh;
	/** Array of source skeletal meshes  */
	const TArray<USkeletalMesh*>& SrcMeshList;

	/** Number of high LODs to remove from input meshes. */
	int32 StripTopLODs;

	/** Info about source mesh used in merge. */
	struct FMergeMeshInfo
	{
		/** Mapping from RefSkeleton bone index in source mesh to output bone index. */
		TArray<int32> SrcToDestRefSkeletonMap;
	};

	/** Array of source mesh info structs. */
	TArray<FMergeMeshInfo> SrcMeshInfo;

	/** New reference skeleton, made from creating union of each part's skeleton. */
	FReferenceSkeleton NewRefSkeleton;

	/** array to map sections from the source meshes to merged section entries */
	const TArray<FSkelMeshMergeSectionMapping>& ForceSectionMapping;

	/** Matches the Materials array in the final mesh - used for creating the right number of Material slots. */
	TArray<int32>	MaterialIds;

	/** keeps track of an existing section that need to be merged with another */
	struct FMergeSectionInfo
	{
		/** ptr to source skeletal mesh for this section */
		const USkeletalMesh* SkelMesh;
		/** ptr to source section for merging */
		const FSkelMeshSection* Section;
		/** ptr to source chunk for this section */
		const FSkelMeshChunk* Chunk;
		/** mapping from the original BoneMap for this sections chunk to the new MergedBoneMap */
		TArray<FBoneIndexType> BoneMapToMergedBoneMap;

		FMergeSectionInfo( const USkeletalMesh* InSkelMesh,const FSkelMeshSection* InSection, const FSkelMeshChunk* InChunk )
			:	SkelMesh(InSkelMesh)
			,	Section(InSection)
			,	Chunk(InChunk)
		{}
	};

	/** info needed to create a new merged section */
	struct FNewSectionInfo
	{
		/** array of existing sections to merge */
		TArray<FMergeSectionInfo> MergeSections;
		/** merged bonemap */
		TArray<FBoneIndexType> MergedBoneMap;
		/** material for use by this section */
		UMaterialInterface* Material;
		/** 
		* if -1 then we use the Material* to match new section entries
		* otherwise the MaterialId is used to find new section entries
		*/
		int32 MaterialId;

		FNewSectionInfo( UMaterialInterface* InMaterial, int32 InMaterialId )
			:	Material(InMaterial)
			,	MaterialId(InMaterialId)
		{}
	};

	/**
	* Merge a bonemap with an existing bonemap and keep track of remapping
	* (a bonemap is a list of indices of bones in the USkeletalMesh::RefSkeleton array)
	* @param MergedBoneMap - out merged bonemap
	* @param BoneMapToMergedBoneMap - out of mapping from original bonemap to new merged bonemap 
	* @param BoneMap - input bonemap to merge
	*/
	void MergeBoneMap( TArray<FBoneIndexType>& MergedBoneMap, TArray<FBoneIndexType>& BoneMapToMergedBoneMap, const TArray<FBoneIndexType>& BoneMap );

	/**
	* Creates a new LOD model and adds the new merged sections to it. Modifies the MergedMesh.
	* @param LODIdx - current LOD to process
	*/
	template<typename VertexDataType, bool bExtraBoneInfluencesT>
	void GenerateLODModel( int32 LODIdx );

	/**
	* Generate the list of sections that need to be created along with info needed to merge sections
	* @param NewSectionArray - out array to populate
	* @param LODIdx - current LOD to process
	*/
	void GenerateNewSectionArray( TArray<FNewSectionInfo>& NewSectionArray, int32 LODIdx );

	/**
	* (Re)initialize and merge skeletal mesh info from the list of source meshes to the merge mesh
	* @return true if succeeded
	*/
	bool ProcessMergeMesh();
};
