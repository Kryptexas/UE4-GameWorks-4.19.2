// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#if WITH_EDITOR

#include "CoreMinimal.h"
#include "Containers/IndirectArray.h"
#include "Rendering/SkeletalMeshLODModel.h"

class FSkeletalMeshLODModel;
class USkeletalMesh;
class FArchive;

/**
* Imported resources for a skeletal mesh.
*/
class FSkeletalMeshModel
{
public:
	/** GUID to indicate this version of data. Needs to change when modifications are made to model, so that derived data is regenerated. */
	FGuid SkeletalMeshModelGUID;
	/** If this GUID was generated from a hash of the data, or randomly generated when it was changed */
	bool bGuidIsHash;

	/** Per-LOD data. */
	TIndirectArray<FSkeletalMeshLODModel> LODModels;

	/** Default constructor. */
	FSkeletalMeshModel();

#if WITH_EDITOR
	/** Creates a new GUID for this Model */
	void GenerateNewGUID();

	/** 
	 *	Util to regenerate a GUID for this Model based on hashing its data 
	 *	Used by old content, rather than a random new GUID.
	 */
	void GenerateGUIDFromHash(USkeletalMesh* Owner);

	/** Get current GUID Id as a string, for DDC key */
	FString GetIdString() const;
#endif

	/** Serialize to/from the specified archive.. */
	void Serialize(FArchive& Ar, USkeletalMesh* Owner);

	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize);
};

#endif // WITH_EDITOR