// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RenderResource.h"
#include "RenderUtils.h"
#include "StaticMeshVertexData.h"
#include "PackedNormal.h"
#include "Components.h"

class FStaticMeshVertexDataInterface;

template<typename TangentTypeT>
struct TStaticMeshVertexTangentDatum
{
	TangentTypeT TangentX;
	TangentTypeT TangentZ;

	FORCEINLINE FVector GetTangentX() const
	{
		return TangentX;
	}

	FORCEINLINE FVector4 GetTangentZ() const
	{
		return TangentZ;
	}

	FORCEINLINE FVector GetTangentY() const
	{
		FVector  TanX = GetTangentX();
		FVector4 TanZ = GetTangentZ();

		return (FVector(TanZ) ^ TanX) * TanZ.W;
	}

	FORCEINLINE void SetTangents(FVector X, FVector Y, FVector Z)
	{
		TangentX = X;
		TangentZ = FVector4(Z, GetBasisDeterminantSign(X, Y, Z));
	}

	/**
	* Serializer
	*
	* @param Ar - archive to serialize with
	* @param V - vertex to serialize
	* @return archive that was used
	*/
	friend FArchive& operator<<(FArchive& Ar, TStaticMeshVertexTangentDatum& Vertex)
	{
		Ar << Vertex.TangentX;
		Ar << Vertex.TangentZ;
		return Ar;
	}
};

template<typename UVTypeT>
struct TStaticMeshVertexUVsDatum
{
	UVTypeT UVs;

	FORCEINLINE FVector2D GetUV() const
	{
		return UVs;
	}

	FORCEINLINE void SetUV(FVector2D UV)
	{
		UVs = UV;
	}

	/**
	* Serializer
	*
	* @param Ar - archive to serialize with
	* @param V - vertex to serialize
	* @return archive that was used
	*/
	friend FArchive& operator<<(FArchive& Ar, TStaticMeshVertexUVsDatum& Vertex)
	{
		Ar << Vertex.UVs;
		return Ar;
	}
};

enum class EStaticMeshVertexTangentBasisType
{
	Default,
	HighPrecision,
};

enum class EStaticMeshVertexUVType
{
	Default,
	HighPrecision,
};

template<EStaticMeshVertexTangentBasisType TangentBasisType>
struct TStaticMeshVertexTangentTypeSelector
{
};

template<>
struct TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::Default>
{
	typedef FPackedNormal TangentTypeT;
	static const EVertexElementType VertexElementType = VET_PackedNormal;
};

template<>
struct TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::HighPrecision>
{
	typedef FPackedRGBA16N TangentTypeT;
	static const EVertexElementType VertexElementType = VET_UShort4N;
};

template<EStaticMeshVertexUVType UVType>
struct TStaticMeshVertexUVsTypeSelector
{
};

template<>
struct TStaticMeshVertexUVsTypeSelector<EStaticMeshVertexUVType::Default>
{
	typedef FVector2DHalf UVsTypeT;
};

template<>
struct TStaticMeshVertexUVsTypeSelector<EStaticMeshVertexUVType::HighPrecision>
{
	typedef FVector2D UVsTypeT;
};

/** Vertex buffer for a static mesh LOD */
class FStaticMeshVertexBuffer : public FRenderResource
{
	friend class FStaticMeshVertexBuffer;

public:

	/** Default constructor. */
	ENGINE_API FStaticMeshVertexBuffer();

	/** Destructor. */
	ENGINE_API ~FStaticMeshVertexBuffer();

	/** Delete existing resources */
	ENGINE_API void CleanUp();

	ENGINE_API void Init(uint32 InNumVertices, uint32 InNumTexCoords, bool bNeedsCPUAccess = true);

	/**
	* Initializes the buffer with the given vertices.
	* @param InVertices - The vertices to initialize the buffer with.
	* @param InNumTexCoords - The number of texture coordinate to store in the buffer.
	*/
	ENGINE_API void Init(const TArray<FStaticMeshBuildVertex>& InVertices, uint32 InNumTexCoords);

	/**
	* Initializes this vertex buffer with the contents of the given vertex buffer.
	* @param InVertexBuffer - The vertex buffer to initialize from.
	*/
	void Init(const FStaticMeshVertexBuffer& InVertexBuffer);

	/**
	* Removes the cloned vertices used for extruding shadow volumes.
	* @param NumVertices - The real number of static mesh vertices which should remain in the buffer upon return.
	*/
	void RemoveLegacyShadowVolumeVertices(uint32 NumVertices);

	/**
	* Serializer
	*
	* @param	Ar				Archive to serialize with
	* @param	bNeedsCPUAccess	Whether the elements need to be accessed by the CPU
	*/
	void Serialize(FArchive& Ar, bool bNeedsCPUAccess);

	/**
	* Specialized assignment operator, only used when importing LOD's.
	*/
	ENGINE_API void operator=(const FStaticMeshVertexBuffer &Other);

	template<EStaticMeshVertexTangentBasisType TangentBasisTypeT>
	FORCEINLINE_DEBUGGABLE FVector4 VertexTangentX_Typed(uint32 VertexIndex)const
	{
		typedef TStaticMeshVertexTangentDatum<typename TStaticMeshVertexTangentTypeSelector<TangentBasisTypeT>::TangentTypeT> TangentType;
		TangentType* ElementData = reinterpret_cast<TangentType*>(TangentsDataPtr);
		check((void*)((&ElementData[VertexIndex]) + 1) <= (void*)(TangentsDataPtr + TangentsData->GetResourceSize()));
		check((void*)((&ElementData[VertexIndex]) + 0) >= (void*)(TangentsDataPtr));
		return ElementData[VertexIndex].GetTangentX();
	}

	FORCEINLINE_DEBUGGABLE FVector4 VertexTangentX(uint32 VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());

		if (GetUseHighPrecisionTangentBasis())
		{
			return VertexTangentX_Typed<EStaticMeshVertexTangentBasisType::HighPrecision>(VertexIndex);
		}
		else
		{
			return VertexTangentX_Typed<EStaticMeshVertexTangentBasisType::Default>(VertexIndex);
		}
	}

	template<EStaticMeshVertexTangentBasisType TangentBasisTypeT>
	FORCEINLINE_DEBUGGABLE FVector4 VertexTangentZ_Typed(uint32 VertexIndex)const
	{
		typedef TStaticMeshVertexTangentDatum<typename TStaticMeshVertexTangentTypeSelector<TangentBasisTypeT>::TangentTypeT> TangentType;
		TangentType* ElementData = reinterpret_cast<TangentType*>(TangentsDataPtr);
		check((void*)((&ElementData[VertexIndex]) + 1) <= (void*)(TangentsDataPtr + TangentsData->GetResourceSize()));
		check((void*)((&ElementData[VertexIndex]) + 0) >= (void*)(TangentsDataPtr));
		return ElementData[VertexIndex].GetTangentZ();
	}

	FORCEINLINE_DEBUGGABLE FVector VertexTangentZ(uint32 VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());

		if (GetUseHighPrecisionTangentBasis())
		{
			return VertexTangentZ_Typed<EStaticMeshVertexTangentBasisType::HighPrecision>(VertexIndex);
		}
		else
		{
			return VertexTangentZ_Typed<EStaticMeshVertexTangentBasisType::Default>(VertexIndex);
		}
	}

	template<EStaticMeshVertexTangentBasisType TangentBasisTypeT>
	FORCEINLINE_DEBUGGABLE FVector4 VertexTangentY_Typed(uint32 VertexIndex)const
	{
		typedef TStaticMeshVertexTangentDatum<typename TStaticMeshVertexTangentTypeSelector<TangentBasisTypeT>::TangentTypeT> TangentType;
		TangentType* ElementData = reinterpret_cast<TangentType*>(TangentsDataPtr);
		check((void*)((&ElementData[VertexIndex]) + 1) <= (void*)(TangentsDataPtr + TangentsData->GetResourceSize()));
		check((void*)((&ElementData[VertexIndex]) + 0) >= (void*)(TangentsDataPtr));
		return ElementData[VertexIndex].GetTangentY();
	}

	/**
	* Calculate the binormal (TangentY) vector using the normal,tangent vectors
	*
	* @param VertexIndex - index into the vertex buffer
	* @return binormal (TangentY) vector
	*/
	FORCEINLINE_DEBUGGABLE FVector VertexTangentY(uint32 VertexIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());

		if (GetUseHighPrecisionTangentBasis())
		{
			return VertexTangentY_Typed<EStaticMeshVertexTangentBasisType::HighPrecision>(VertexIndex);
		}
		else
		{
			return VertexTangentY_Typed<EStaticMeshVertexTangentBasisType::Default>(VertexIndex);
		}
	}

	FORCEINLINE_DEBUGGABLE void SetVertexTangents(uint32 VertexIndex, FVector X, FVector Y, FVector Z)
	{
		checkSlow(VertexIndex < GetNumVertices());

		if (GetUseHighPrecisionTangentBasis())
		{
			typedef TStaticMeshVertexTangentDatum<typename TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::HighPrecision>::TangentTypeT> TangentType;
			TangentType* ElementData = reinterpret_cast<TangentType*>(TangentsDataPtr);
			check((void*)((&ElementData[VertexIndex]) + 1) <= (void*)(TangentsDataPtr + TangentsData->GetResourceSize()));
			check((void*)((&ElementData[VertexIndex]) + 0) >= (void*)(TangentsDataPtr));
			ElementData[VertexIndex].SetTangents(X, Y, Z);
		}
		else
		{
			typedef TStaticMeshVertexTangentDatum<typename TStaticMeshVertexTangentTypeSelector<EStaticMeshVertexTangentBasisType::Default>::TangentTypeT> TangentType;
			TangentType* ElementData = reinterpret_cast<TangentType*>(TangentsDataPtr);
			check((void*)((&ElementData[VertexIndex]) + 1) <= (void*)(TangentsDataPtr + TangentsData->GetResourceSize()));
			check((void*)((&ElementData[VertexIndex]) + 0) >= (void*)(TangentsDataPtr));
			ElementData[VertexIndex].SetTangents(X, Y, Z);
		}
	}

	/**
	* Set the vertex UV values at the given index in the vertex buffer
	*
	* @param VertexIndex - index into the vertex buffer
	* @param UVIndex - [0,MAX_STATIC_TEXCOORDS] value to index into UVs array
	* @param Vec2D - UV values to set
	*/
	FORCEINLINE_DEBUGGABLE void SetVertexUV(uint32 VertexIndex, uint32 UVIndex, const FVector2D& Vec2D)
	{
		checkSlow(VertexIndex < GetNumVertices());
		checkSlow(UVIndex < GetNumTexCoords());

		if (GetUseFullPrecisionUVs())
		{
			typedef TStaticMeshVertexUVsDatum<typename TStaticMeshVertexUVsTypeSelector<EStaticMeshVertexUVType::HighPrecision>::UVsTypeT> UVType;
			size_t UvStride = sizeof(UVType) * GetNumTexCoords();

			UVType* ElementData = reinterpret_cast<UVType*>(TexcoordDataPtr + (VertexIndex * UvStride));
			check((void*)((&ElementData[UVIndex]) + 1) <= (void*)(TexcoordDataPtr + TexcoordData->GetResourceSize()));
			check((void*)((&ElementData[UVIndex]) + 0) >= (void*)(TexcoordDataPtr));
			ElementData[UVIndex].SetUV(Vec2D);
		}
		else
		{
			typedef TStaticMeshVertexUVsDatum<typename TStaticMeshVertexUVsTypeSelector<EStaticMeshVertexUVType::Default>::UVsTypeT> UVType;
			size_t UvStride = sizeof(UVType) * GetNumTexCoords();

			UVType* ElementData = reinterpret_cast<UVType*>(TexcoordDataPtr + (VertexIndex * UvStride));
			check((void*)((&ElementData[UVIndex]) + 1) <= (void*)(TexcoordDataPtr + TexcoordData->GetResourceSize()));
			check((void*)((&ElementData[UVIndex]) + 0) >= (void*)(TexcoordDataPtr));
			ElementData[UVIndex].SetUV(Vec2D);
		}
	}

	template<EStaticMeshVertexUVType UVTypeT>
	FORCEINLINE_DEBUGGABLE FVector2D GetVertexUV_Typed(uint32 VertexIndex, uint32 UVIndex)const
	{
		typedef TStaticMeshVertexUVsDatum<typename TStaticMeshVertexUVsTypeSelector<UVTypeT>::UVsTypeT> UVType;
		size_t UvStride = sizeof(UVType) * GetNumTexCoords();

		UVType* ElementData = reinterpret_cast<UVType*>(TexcoordDataPtr + (VertexIndex * UvStride));
		check((void*)((&ElementData[UVIndex]) + 1) <= (void*)(TexcoordDataPtr + TexcoordData->GetResourceSize()));
		check((void*)((&ElementData[UVIndex]) + 0) >= (void*)(TexcoordDataPtr));
		return ElementData[UVIndex].GetUV();
	}

	/**
	* Set the vertex UV values at the given index in the vertex buffer
	*
	* @param VertexIndex - index into the vertex buffer
	* @param UVIndex - [0,MAX_STATIC_TEXCOORDS] value to index into UVs array
	* @param 2D UV values
	*/
	FORCEINLINE_DEBUGGABLE FVector2D GetVertexUV(uint32 VertexIndex, uint32 UVIndex) const
	{
		checkSlow(VertexIndex < GetNumVertices());
		checkSlow(UVIndex < GetNumTexCoords());

		if (GetUseFullPrecisionUVs())
		{
			return GetVertexUV_Typed<EStaticMeshVertexUVType::HighPrecision>(VertexIndex, UVIndex);
		}
		else
		{
			return GetVertexUV_Typed<EStaticMeshVertexUVType::Default>(VertexIndex, UVIndex);
		}
	}

	FORCEINLINE_DEBUGGABLE uint32 GetNumVertices() const
	{
		return NumVertices;
	}

	FORCEINLINE_DEBUGGABLE uint32 GetNumTexCoords() const
	{
		return NumTexCoords;
	}

	FORCEINLINE_DEBUGGABLE bool GetUseFullPrecisionUVs() const
	{
		return bUseFullPrecisionUVs;
	}

	FORCEINLINE_DEBUGGABLE void SetUseFullPrecisionUVs(bool UseFull)
	{
		bUseFullPrecisionUVs = UseFull;
	}

	FORCEINLINE_DEBUGGABLE bool GetUseHighPrecisionTangentBasis() const
	{
		return bUseHighPrecisionTangentBasis;
	}

	FORCEINLINE_DEBUGGABLE void SetUseHighPrecisionTangentBasis(bool bUseHighPrecision)
	{
		bUseHighPrecisionTangentBasis = bUseHighPrecision;
	}

	FORCEINLINE_DEBUGGABLE uint32 GetResourceSize() const
	{
		return (TangentsStride + (TexcoordStride * GetNumTexCoords())) * NumVertices;
	}

	// FRenderResource interface.
	ENGINE_API virtual void InitRHI() override;
	ENGINE_API virtual void ReleaseRHI() override;
	ENGINE_API virtual void InitResource() override;
	ENGINE_API virtual void ReleaseResource() override;
	virtual FString GetFriendlyName() const override { return TEXT("Static-mesh vertices"); }

	ENGINE_API void BindTangentVertexBuffer(const FVertexFactory* VertexFactory, struct FStaticMeshDataType& Data) const;
	ENGINE_API void BindTexCoordVertexBuffer(const FVertexFactory* VertexFactory, struct FStaticMeshDataType& Data, int ClampedNumTexCoords = -1) const;
	ENGINE_API void BindPackedTexCoordVertexBuffer(const FVertexFactory* VertexFactory, struct FStaticMeshDataType& Data) const;
	ENGINE_API void BindLightMapVertexBuffer(const FVertexFactory* VertexFactory, struct FStaticMeshDataType& Data, int LightMapCoordinateIndex) const;

	FORCEINLINE_DEBUGGABLE void* GetTangentData()
	{
		return TangentsDataPtr;
	}

	FORCEINLINE_DEBUGGABLE void* GetTexCoordData()
	{
		return TexcoordDataPtr;
	}

	ENGINE_API int GetTangentSize();

	ENGINE_API int GetTexCoordSize();

	FORCEINLINE_DEBUGGABLE bool GetAllowCPUAccess()
	{
		if (!TangentsData || !TexcoordData)
			return false;

		return TangentsData->GetAllowCPUAccess() && TexcoordData->GetAllowCPUAccess();
	}

	class FTangentsVertexBuffer : public FVertexBuffer
	{
		virtual FString GetFriendlyName() const override { return TEXT("FTangentsVertexBuffer"); }
	} TangentsVertexBuffer;

	class FTexcoordVertexBuffer : public FVertexBuffer
	{
		virtual FString GetFriendlyName() const override { return TEXT("FTexcoordVertexBuffer"); }
	} TexCoordVertexBuffer;

	inline bool IsValid()
	{
		return IsValidRef(TangentsVertexBuffer.VertexBufferRHI) && IsValidRef(TexCoordVertexBuffer.VertexBufferRHI);
	}
private:

	/** The vertex data storage type */
	FStaticMeshVertexDataInterface* TangentsData;
	FShaderResourceViewRHIRef TangentsSRV;

	FStaticMeshVertexDataInterface* TexcoordData;
	FShaderResourceViewRHIRef TextureCoordinatesSRV;

	/** The cached vertex data pointer. */
	uint8* TangentsDataPtr;
	uint8* TexcoordDataPtr;

	/** The cached Tangent stride. */
	uint32 TangentsStride;

	/** The cached Texcoord stride. */
	uint32 TexcoordStride;

	/** The number of texcoords/vertex in the buffer. */
	uint32 NumTexCoords;

	/** The cached number of vertices. */
	uint32 NumVertices;

	/** Corresponds to UStaticMesh::UseFullPrecisionUVs. if true then 32 bit UVs are used */
	bool bUseFullPrecisionUVs;

	/** If true then RGB10A2 is used to store tangent else RGBA8 */
	bool bUseHighPrecisionTangentBasis;

	/** Allocates the vertex data storage type. */
	void AllocateData(bool bNeedsCPUAccess = true);

	/** Convert half float data to full float if the HW requires it.
	* @param InData - optional half float source data to convert into full float texture coordinate buffer. if null, convert existing half float texture coordinates to a new float buffer.
	*/
	void ConvertHalfTexcoordsToFloat(const uint8* InData);
};