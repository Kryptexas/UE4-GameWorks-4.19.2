// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Rendering/SkeletalMeshVertexClothBuffer.h"
#include "Rendering/SkeletalMeshVertexBuffer.h"
#include "EngineUtils.h"
#include "SkeletalMeshTypes.h"

/**
* Constructor
*/
FSkeletalMeshVertexClothBuffer::FSkeletalMeshVertexClothBuffer()
	: VertexData(nullptr),
	Data(nullptr),
	Stride(0),
	NumVertices(0)
{

}

/**
* Destructor
*/
FSkeletalMeshVertexClothBuffer::~FSkeletalMeshVertexClothBuffer()
{
	// clean up everything
	CleanUp();
}

/**
* Assignment. Assumes that vertex buffer will be rebuilt
*/

FSkeletalMeshVertexClothBuffer& FSkeletalMeshVertexClothBuffer::operator=(const FSkeletalMeshVertexClothBuffer& Other)
{
	CleanUp();
	return *this;
}

/**
* Copy Constructor
*/
FSkeletalMeshVertexClothBuffer::FSkeletalMeshVertexClothBuffer(const FSkeletalMeshVertexClothBuffer& Other)
	: VertexData(nullptr),
	Data(nullptr),
	Stride(0),
	NumVertices(0)
{

}

/**
* @return text description for the resource type
*/
FString FSkeletalMeshVertexClothBuffer::GetFriendlyName() const
{
	return TEXT("Skeletal-mesh vertex APEX cloth mesh-mesh mapping buffer");
}

/**
* Delete existing resources
*/
void FSkeletalMeshVertexClothBuffer::CleanUp()
{
	delete VertexData;
	VertexData = nullptr;
}

/**
* Initialize the RHI resource for this vertex buffer
*/
void FSkeletalMeshVertexClothBuffer::InitRHI()
{
	check(VertexData);
	FResourceArrayInterface* ResourceArray = VertexData->GetResourceArray();
	if (ResourceArray->GetResourceDataSize() > 0)
	{
		FRHIResourceCreateInfo CreateInfo(ResourceArray);
		VertexBufferRHI = RHICreateVertexBuffer(ResourceArray->GetResourceDataSize(), BUF_Static | BUF_ShaderResource, CreateInfo);
		VertexBufferSRV = RHICreateShaderResourceView(VertexBufferRHI, sizeof(FVector4), PF_R32G32B32A32_UINT);
	}
}

/**
* Serializer for this class
* @param Ar - archive to serialize to
* @param B - data to serialize
*/
FArchive& operator<<(FArchive& Ar, FSkeletalMeshVertexClothBuffer& VertexBuffer)
{
	FStripDataFlags StripFlags(Ar, 0, VER_UE4_STATIC_SKELETAL_MESH_SERIALIZATION_FIX);

	if (Ar.IsLoading())
	{
		VertexBuffer.AllocateData();
	}

	if (!StripFlags.IsDataStrippedForServer() || Ar.IsCountingMemory())
	{
		if (VertexBuffer.VertexData != NULL)
		{
			VertexBuffer.VertexData->Serialize(Ar);

			// update cached buffer info
			VertexBuffer.NumVertices = VertexBuffer.VertexData->GetNumVertices();
			VertexBuffer.Data = (VertexBuffer.NumVertices > 0) ? VertexBuffer.VertexData->GetDataPointer() : nullptr;
			VertexBuffer.Stride = VertexBuffer.VertexData->GetStride();
		}

		Ar << VertexBuffer.ClothIndexMapping;
	}

	return Ar;
}


/**
* Initializes the buffer with the given vertices.
* @param InVertices - The vertices to initialize the buffer with.
*/
void FSkeletalMeshVertexClothBuffer::Init(const TArray<FMeshToMeshVertData>& InMappingData, const TArray<uint64>& InClothIndexMapping)
{
	// Allocate new data
	AllocateData();

	// Resize the buffer to hold enough data for all passed in vertices
	VertexData->ResizeBuffer(InMappingData.Num());

	Data = VertexData->GetDataPointer();
	Stride = VertexData->GetStride();
	NumVertices = VertexData->GetNumVertices();

	// Copy the vertices into the buffer.
	checkSlow(Stride*NumVertices == sizeof(FMeshToMeshVertData) * InMappingData.Num());
	//appMemcpy(Data, &InMappingData(0), Stride*NumVertices);
	for (int32 Index = 0; Index < InMappingData.Num(); Index++)
	{
		const FMeshToMeshVertData& SourceMapping = InMappingData[Index];
		const int32 DestVertexIndex = Index;
		MappingData(DestVertexIndex) = SourceMapping;
	}
	ClothIndexMapping = InClothIndexMapping;
}

/**
* Allocates the vertex data storage type.
*/
void FSkeletalMeshVertexClothBuffer::AllocateData()
{
	CleanUp();

	VertexData = new TSkeletalMeshVertexData<FMeshToMeshVertData>(true);
}