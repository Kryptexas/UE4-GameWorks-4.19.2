// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/DynamicRHIResourceArray.h"
#include "Rendering/StaticMeshVertexDataInterface.h"

/** The implementation of the static mesh vertex data storage type. */
template<typename VertexDataType>
class TStaticMeshVertexData :
	public FStaticMeshVertexDataInterface
{
	TResourceArray<VertexDataType, VERTEXBUFFER_ALIGNMENT> Data;

public:

	/**
	* Constructor
	* @param InNeedsCPUAccess - true if resource array data should be CPU accessible
	*/
	TStaticMeshVertexData(bool InNeedsCPUAccess=false)
		: Data(InNeedsCPUAccess)
	{
	}

	/**
	* Resizes the vertex data buffer, discarding any data which no longer fits.
	*
	* @param NumVertices - The number of vertices to allocate the buffer for.
	*/
	void ResizeBuffer(uint32 NumVertices) override
	{
		if((uint32)Data.Num() < NumVertices)
		{
			// Enlarge the array.
			Data.AddUninitialized(NumVertices - Data.Num());
		}
		else if((uint32)Data.Num() > NumVertices)
		{
			// Shrink the array.
			Data.RemoveAt(NumVertices, Data.Num() - NumVertices);
		}
	}

	void Empty(uint32 NumVertices) override
	{
		Data.Empty(NumVertices);
	}

	bool IsValidIndex(uint32 Index) override
	{
		return Data.IsValidIndex(Index);
	}

	/**
	* @return stride of the vertex type stored in the resource data array
	*/
	uint32 GetStride() const override
	{
		return sizeof(VertexDataType);
	}
	/**
	* @return uint8 pointer to the resource data array
	*/
	uint8* GetDataPointer() override
	{
		return (uint8*)&Data[0];
	}

	/**
	* @return resource array interface access
	*/
	FResourceArrayInterface* GetResourceArray() override
	{
		return &Data;
	}
	/**
	* Serializer for this class
	*
	* @param Ar - archive to serialize to
	* @param B - data to serialize
	*/
	void Serialize(FArchive& Ar) override
	{
		Data.TResourceArray<VertexDataType, VERTEXBUFFER_ALIGNMENT>::BulkSerialize(Ar);
	}
	/**
	* Assignment. This is currently the only method which allows for 
	* modifying an existing resource array
	*/
	void Assign(const TArray<VertexDataType>& Other)
	{
		ResizeBuffer(Other.Num());
		if (Other.Num())
		{
			memcpy(GetDataPointer(), &Other[0], Other.Num() * sizeof(VertexDataType));
		}
	}

	/**
	* Helper function to return the amount of memory allocated by this
	* container.
	*
	* @returns Number of bytes allocated by this container.
	*/
	SIZE_T GetResourceSize() const override
	{
		return Data.GetAllocatedSize();
	}

	/**
	* Helper function to return the number of elements by this
	* container.
	*
	* @returns Number of elements allocated by this container.
	*/
	FORCEINLINE int32 Num() const
	{
		return Data.Num();
	}

	bool GetAllowCPUAccess() const override
	{
		return Data.GetAllowCPUAccess();
	}
};
