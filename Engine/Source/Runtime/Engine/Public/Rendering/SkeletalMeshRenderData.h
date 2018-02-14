// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RHIDefinitions.h"
#include "Containers/IndirectArray.h"
#include "Rendering/SkeletalMeshLODRenderData.h"

struct FMeshUVChannelInfo;
class USkeletalMesh;
struct FSkeletalMaterial;
class UMorphTarget;
struct FResourceSizeEx;

class FSkeletalMeshRenderData
{
public:
	/** Per-LOD render data. */
	TIndirectArray<FSkeletalMeshLODRenderData> LODRenderData;

#if WITH_EDITORONLY_DATA
	/** UV data used for streaming accuracy debug view modes. In sync for rendering thread */
	TArray<FMeshUVChannelInfo> UVChannelDataPerMaterial;
#endif

#if WITH_EDITOR
	void Cache(USkeletalMesh* Owner);

	void SyncUVChannelData(const TArray<FSkeletalMaterial>& ObjectData);
#endif

	/** Serialize to/from the specified archive.. */
	void Serialize(FArchive& Ar, USkeletalMesh* Owner);

	/** Initializes rendering resources. */
	void InitResources(bool bNeedsVertexColors, TArray<UMorphTarget*>& InMorphTargets);

	/** Releases rendering resources. */
	ENGINE_API void ReleaseResources();

	/** Return the resource size */
	void GetResourceSizeEx(FResourceSizeEx& CumulativeResourceSize);

	/** Returns true if this resource must be skinned on the CPU for the given feature level. */
	bool RequiresCPUSkinning(ERHIFeatureLevel::Type FeatureLevel) const;

	/** Returns true if there are more than MAX_INFLUENCES_PER_STREAM influences per vertex. */
	bool HasExtraBoneInfluences() const;

	/**
	* Computes the maximum number of bones per section used to render this mesh.
	*/
	ENGINE_API int32 GetMaxBonesPerSection() const;

private:
	/** True if the resource has been initialized. */
	bool bInitialized;
};