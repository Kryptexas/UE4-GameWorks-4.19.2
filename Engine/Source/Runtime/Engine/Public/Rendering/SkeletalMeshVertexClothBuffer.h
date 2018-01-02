// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RenderResource.h"

class FSkeletalMeshVertexDataInterface;
struct FMeshToMeshVertData;

/**
* A vertex buffer for holding skeletal mesh per APEX cloth information only.
* This buffer sits along side FSkeletalMeshVertexBuffer in each skeletal mesh lod
*/
class FSkeletalMeshVertexClothBuffer : public FVertexBuffer
{
public:
	/**
	* Constructor
	*/
	ENGINE_API FSkeletalMeshVertexClothBuffer();

	/**
	* Destructor
	*/
	ENGINE_API virtual ~FSkeletalMeshVertexClothBuffer();

	/**
	* Assignment. Assumes that vertex buffer will be rebuilt
	*/
	ENGINE_API FSkeletalMeshVertexClothBuffer& operator=(const FSkeletalMeshVertexClothBuffer& Other);

	/**
	* Constructor (copy)
	*/
	ENGINE_API FSkeletalMeshVertexClothBuffer(const FSkeletalMeshVertexClothBuffer& Other);

	/**
	* Delete existing resources
	*/
	void CleanUp();

	/**
	* Initializes the buffer with the given vertices.
	* @param InVertices - The vertices to initialize the buffer with.
	* @param InClothIndexMapping - Packed Map: u32 Key, u32 Value.
	*/
	void Init(const TArray<FMeshToMeshVertData>& InMappingData, const TArray<uint64>& InClothIndexMapping);


	/**
	* Serializer for this class
	* @param Ar - archive to serialize to
	* @param B - data to serialize
	*/
	friend FArchive& operator<<(FArchive& Ar, FSkeletalMeshVertexClothBuffer& VertexBuffer);

	//~ Begin FRenderResource interface.

	/**
	* Initialize the RHI resource for this vertex buffer
	*/
	virtual void InitRHI() override;

	/**
	* @return text description for the resource type
	*/
	virtual FString GetFriendlyName() const override;

	//~ End FRenderResource interface.

	//~ Vertex data accessors.

	FORCEINLINE FMeshToMeshVertData& MappingData(uint32 VertexIndex)
	{
		checkSlow(VertexIndex < GetNumVertices());
		return *((FMeshToMeshVertData*)(Data + VertexIndex * Stride));
	}
	FORCEINLINE const FMeshToMeshVertData& MappingData(uint32 VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		return *((FMeshToMeshVertData*)(Data + VertexIndex * Stride));
	}

	/**
	* @return number of vertices in this vertex buffer
	*/
	FORCEINLINE uint32 GetNumVertices() const
	{
		return NumVertices;
	}

	/**
	* @return cached stride for vertex data type for this vertex buffer
	*/
	FORCEINLINE uint32 GetStride() const
	{
		return Stride;
	}
	/**
	* @return total size of data in resource array
	*/
	FORCEINLINE uint32 GetVertexDataSize() const
	{
		return NumVertices * Stride;
	}

	inline FShaderResourceViewRHIRef GetSRV() const
	{
		return VertexBufferSRV;
	}

	inline const TArray<uint64>& GetClothIndexMapping() const
	{
		return ClothIndexMapping;
	}

private:
	FShaderResourceViewRHIRef VertexBufferSRV;

	// Packed Map: u32 Key, u32 Value
	TArray<uint64> ClothIndexMapping;

	/** The vertex data storage type */
	FSkeletalMeshVertexDataInterface* VertexData;
	/** The cached vertex data pointer. */
	uint8* Data;
	/** The cached vertex stride. */
	uint32 Stride;
	/** The cached number of vertices. */
	uint32 NumVertices;

	/**
	* Allocates the vertex data storage type
	*/
	void AllocateData();
};