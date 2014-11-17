// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PrimitiveUniformShaderParameters.h"

/**
 * A batch mesh element definition.
 */
struct FMeshBatchElement
{
	const TUniformBuffer<FPrimitiveUniformShaderParameters>* PrimitiveUniformBufferResource;
	TUniformBufferRef<FPrimitiveUniformShaderParameters> PrimitiveUniformBuffer;

	const FIndexBuffer* IndexBuffer;
	uint32 FirstIndex;
	uint32 NumPrimitives;
	uint32 NumInstances;
	uint32 MinVertexIndex;
	uint32 MaxVertexIndex;
	int32 UserIndex;
	void* UserData;
	float MinScreenSize;
	float MaxScreenSize;

	/** 
	 *	DynamicIndexData - pointer to user memory containing the index data.
	 *	Used for rendering dynamic data directly.
	 */
	const void* DynamicIndexData;
	uint16 DynamicIndexStride;
	
	FMeshBatchElement()
	:	PrimitiveUniformBufferResource(NULL)
	,	IndexBuffer(NULL)
	,	NumInstances(1)
	,	UserIndex(-1)
	,	UserData(NULL)
	,	MinScreenSize(0.0f)
	,	MaxScreenSize(1.0f)
	,	DynamicIndexData(NULL)
	{
	}
};

/**
 * A batch of mesh elements, all with the same material and vertex buffer
 */
struct FMeshBatch
{
	TArray<FMeshBatchElement,TInlineAllocator<1> > Elements;

	// used with DynamicVertexData
	uint16 DynamicVertexStride;

	/** LOD index of the mesh, used for fading LOD transitions. */
	int8 LODIndex;

	uint32 UseDynamicData : 1;
	uint32 ReverseCulling : 1;
	uint32 bDisableBackfaceCulling : 1;
	uint32 CastShadow : 1;
	uint32 bWireframe : 1;
	// e.g. PT_TriangleList(default), PT_LineList, ..
	uint32 Type : PT_NumBits;
	// e.g. SDPG_World (default), SDPG_Foreground
	uint32 DepthPriorityGroup : SDPG_NumBits;
	uint32 bUseAsOccluder : 1;

	// can be 0
	const FLightCacheInterface* LCI;

	/** 
	 *	DynamicVertexData - pointer to user memory containing the vertex data.
	 *	Used for rendering dynamic data directly.
	 *  used with DynamicVertexStride
	 */
	const void* DynamicVertexData;

	// can be 0
	const FVertexFactory* VertexFactory;
	// can be 0
	const FMaterialRenderProxy* MaterialRenderProxy;


	FORCEINLINE bool IsTranslucent(ERHIFeatureLevel::Type InFeatureLevel) const
	{
		// Note: blend mode does not depend on the feature level we are actually rendering in.
		return MaterialRenderProxy && IsTranslucentBlendMode(MaterialRenderProxy->GetMaterial(InFeatureLevel)->GetBlendMode());
	}

	FORCEINLINE bool IsMasked(ERHIFeatureLevel::Type InFeatureLevel) const
	{
		// Note: blend mode does not depend on the feature level we are actually rendering in.
		return MaterialRenderProxy && MaterialRenderProxy->GetMaterial(InFeatureLevel)->IsMasked();
	}

	/** Converts from an int32 index into a int8 */
	static int8 QuantizeLODIndex(int32 NewLODIndex)
	{
		checkSlow(NewLODIndex >= SCHAR_MIN && NewLODIndex <= SCHAR_MAX);
		return (int8)NewLODIndex;
	}

	/** 
	* @return vertex stride specified for the mesh. 0 if not dynamic
	*/
	FORCEINLINE uint32 GetDynamicVertexStride(ERHIFeatureLevel::Type /*InFeatureLevel*/) const
	{
		if (UseDynamicData && DynamicVertexData)
		{
			return DynamicVertexStride;
		}
		else
		{
			return 0;
		}
	}

	FORCEINLINE int32 GetNumPrimitives() const
	{
		int32 Count=0;
		for( int32 ElementIdx=0;ElementIdx<Elements.Num();ElementIdx++ )
		{
			Count += Elements[ElementIdx].NumPrimitives;
		}
		return Count;
	}

#if DO_CHECK
	FORCEINLINE void CheckUniformBuffers() const
	{
		for( int32 ElementIdx=0;ElementIdx<Elements.Num();ElementIdx++ )
		{
			check(IsValidRef(Elements[ElementIdx].PrimitiveUniformBuffer) || Elements[ElementIdx].PrimitiveUniformBufferResource != NULL);
		}
	}
#endif	

	/** Default constructor. */
	FMeshBatch()
	:	DynamicVertexStride(0)
	,	LODIndex(INDEX_NONE)
	,	UseDynamicData(false)
	,	ReverseCulling(false)
	,	bDisableBackfaceCulling(false)
	,	CastShadow(true)
	,	bWireframe(false)
	,	Type(PT_TriangleList)
	,	DepthPriorityGroup(SDPG_World)
	,	bUseAsOccluder(true)
	,	LCI(NULL)
	,	DynamicVertexData(NULL)
	,	VertexFactory(NULL)
	,	MaterialRenderProxy(NULL)
	{
		// By default always add the first element.
		new(Elements) FMeshBatchElement;
	}
};