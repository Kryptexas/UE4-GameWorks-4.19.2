// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalShaderResources.h: Metal shader resource RHI definitions.
=============================================================================*/

#pragma once

enum
{
	METAL_SHADER_STAGE_VERTEX = 0,
	METAL_SHADER_STAGE_PIXEL,
	METAL_SHADER_STAGE_GEOMETRY,
	METAL_SHADER_STAGE_HULL,
	METAL_SHADER_STAGE_DOMAIN,
	METAL_NUM_NON_COMPUTE_SHADER_STAGES,
	METAL_SHADER_STAGE_COMPUTE = METAL_NUM_NON_COMPUTE_SHADER_STAGES,
	METAL_NUM_SHADER_STAGES
};

/**
* Shader related constants.
*/
enum
{
	METAL_PACKED_TYPENAME_HIGHP = 'h',	// Make sure these enums match hlslcc
	METAL_PACKED_TYPENAME_MEDIUMP = 'm',
	METAL_PACKED_TYPENAME_LOWP = 'l',
	METAL_PACKED_TYPENAME_INT = 'i',
	METAL_PACKED_TYPENAME_UINT = 'u',
	METAL_PACKED_TYPENAME_SAMPLER = 's',
	METAL_PACKED_TYPENAME_IMAGE = 'g',

	METAL_PACKED_TYPEINDEX_HIGHP = 0,
	METAL_PACKED_TYPEINDEX_MEDIUMP = 1,
	METAL_PACKED_TYPEINDEX_LOWP = 2,
	METAL_PACKED_TYPEINDEX_INT = 3,
	METAL_PACKED_TYPEINDEX_UINT = 4,
	METAL_PACKED_TYPEINDEX_MAX = 5,

	METAL_MAX_UNIFORM_BUFFER_BINDINGS = 12,	// @todo-mobile: Remove me
	METAL_FIRST_UNIFORM_BUFFER = 0,			// @todo-mobile: Remove me
	METAL_MAX_COMPUTE_STAGE_UAV_UNITS = 8,	// @todo-mobile: Remove me
	METAL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT = -1, // for now, only CS supports UAVs/ images
};

static FORCEINLINE uint8 MetalPackedTypeNameToTypeIndex(uint8 ArrayName)
{
	switch (ArrayName)
	{
	case METAL_PACKED_TYPENAME_HIGHP:		return METAL_PACKED_TYPEINDEX_HIGHP;
	case METAL_PACKED_TYPENAME_MEDIUMP:	return METAL_PACKED_TYPEINDEX_MEDIUMP;
	case METAL_PACKED_TYPENAME_LOWP:		return METAL_PACKED_TYPEINDEX_LOWP;
	case METAL_PACKED_TYPENAME_INT:		return METAL_PACKED_TYPEINDEX_INT;
	case METAL_PACKED_TYPENAME_UINT:		return METAL_PACKED_TYPEINDEX_UINT;
	}
	check(0);
	return 0;
}

struct FMetalPackedArrayInfo
{
	uint16	Size;		// Bytes
	uint8	TypeName;	// OGL_PACKED_TYPENAME
	uint8	TypeIndex;	// OGL_PACKED_TYPE
};

inline FArchive& operator<<(FArchive& Ar, FMetalPackedArrayInfo& Info)
{
	Ar << Info.Size;
	Ar << Info.TypeName;
	Ar << Info.TypeIndex;
	return Ar;
}

struct FMetalShaderResourceTable : public FBaseShaderResourceTable
{
	/** Mapping of bound Textures to their location in resource tables. */
	TArray<uint32> TextureMap;
	friend bool operator==(const FMetalShaderResourceTable &A, const FMetalShaderResourceTable& B)
	{
		if (!(((FBaseShaderResourceTable&)A) == ((FBaseShaderResourceTable&)B)))
		{
			return false;
		}
		if (A.TextureMap.Num() != B.TextureMap.Num())
		{
			return false;
		}
		if (FMemory::Memcmp(A.TextureMap.GetTypedData(), B.TextureMap.GetTypedData(), A.TextureMap.GetTypeSize()*A.TextureMap.Num()) != 0)
		{
			return false;
		}
		return true;
	}
};

inline FArchive& operator<<(FArchive& Ar, FMetalShaderResourceTable& SRT)
{
	Ar << ((FBaseShaderResourceTable&)SRT);
	Ar << SRT.TextureMap;
	return Ar;
}


struct FMetalShaderBindings
{
	TArray<TArray<FMetalPackedArrayInfo>>	PackedUniformBuffers;
	TArray<FMetalPackedArrayInfo>			PackedGlobalArrays;
	FMetalShaderResourceTable				ShaderResourceTable;

	uint16	InOutMask;
	uint8	NumSamplers;
	uint8	NumUniformBuffers;
	uint8	NumUAVs;
	bool	bHasRegularUniformBuffers;

	FMetalShaderBindings() :
		InOutMask(0),
		NumSamplers(0),
		NumUniformBuffers(0),
		NumUAVs(0),
		bHasRegularUniformBuffers(false)
	{
	}

	friend bool operator==(const FMetalShaderBindings &A, const FMetalShaderBindings &B)
	{
		bool bEqual = true;

		bEqual &= A.InOutMask == B.InOutMask;
		bEqual &= A.NumSamplers == B.NumSamplers;
		bEqual &= A.NumUniformBuffers == B.NumUniformBuffers;
		bEqual &= A.NumUAVs == B.NumUAVs;
		bEqual &= A.bHasRegularUniformBuffers == B.bHasRegularUniformBuffers;
		bEqual &= A.PackedGlobalArrays.Num() == B.PackedGlobalArrays.Num();
		bEqual &= A.PackedUniformBuffers.Num() == B.PackedUniformBuffers.Num();
		bEqual &= A.ShaderResourceTable == B.ShaderResourceTable;

		if (!bEqual)
		{
			return bEqual;
		}

		bEqual &= FMemory::Memcmp(A.PackedGlobalArrays.GetTypedData(), B.PackedGlobalArrays.GetTypedData(), A.PackedGlobalArrays.GetTypeSize()*A.PackedGlobalArrays.Num()) == 0;

		for (int32 Item = 0; Item < A.PackedUniformBuffers.Num(); Item++)
		{
			const TArray<FMetalPackedArrayInfo> &ArrayA = A.PackedUniformBuffers[Item];
			const TArray<FMetalPackedArrayInfo> &ArrayB = B.PackedUniformBuffers[Item];

			bEqual &= FMemory::Memcmp(ArrayA.GetTypedData(), ArrayB.GetTypedData(), ArrayA.GetTypeSize()*ArrayA.Num()) == 0;
		}

		return bEqual;
	}

	friend uint32 GetTypeHash(const FMetalShaderBindings &Binding)
	{
		uint32 Hash = 0;
		Hash = Binding.InOutMask;
		Hash |= Binding.NumSamplers << 16;
		Hash |= Binding.NumUniformBuffers << 24;
		Hash ^= Binding.NumUAVs;
		Hash ^= Binding.bHasRegularUniformBuffers << 8;
		Hash ^= FCrc::MemCrc_DEPRECATED(Binding.PackedGlobalArrays.GetTypedData(), Binding.PackedGlobalArrays.GetTypeSize()*Binding.PackedGlobalArrays.Num());

		for (int32 Item = 0; Item < Binding.PackedUniformBuffers.Num(); Item++)
		{
			const TArray<FMetalPackedArrayInfo> &Array = Binding.PackedUniformBuffers[Item];
			Hash ^= FCrc::MemCrc_DEPRECATED(Array.GetTypedData(), Array.GetTypeSize()* Array.Num());
		}
		return Hash;
	}
};

inline FArchive& operator<<(FArchive& Ar, FMetalShaderBindings& Bindings)
{
	Ar << Bindings.PackedUniformBuffers;
	Ar << Bindings.PackedGlobalArrays;
	Ar << Bindings.ShaderResourceTable;
	Ar << Bindings.InOutMask;
	Ar << Bindings.NumSamplers;
	Ar << Bindings.NumUniformBuffers;
	Ar << Bindings.NumUAVs;
	Ar << Bindings.bHasRegularUniformBuffers;
	return Ar;
}

// Information for copying members from uniform buffers to packed
struct FMetalUniformBufferCopyInfo
{
	uint16 SourceOffsetInFloats;
	uint8 SourceUBIndex;
	uint8 DestUBIndex;
	uint8 DestUBTypeName;
	uint8 DestUBTypeIndex;
	uint16 DestOffsetInFloats;
	uint16 SizeInFloats;
};

inline FArchive& operator<<(FArchive& Ar, FMetalUniformBufferCopyInfo& Info)
{
	Ar << Info.SourceOffsetInFloats;
	Ar << Info.SourceUBIndex;
	Ar << Info.DestUBIndex;
	Ar << Info.DestUBTypeName;
	if (Ar.IsLoading())
	{
		Info.DestUBTypeIndex = MetalPackedTypeNameToTypeIndex(Info.DestUBTypeName);
	}
	Ar << Info.DestOffsetInFloats;
	Ar << Info.SizeInFloats;
	return Ar;
}

struct FMetalCodeHeader
{
	uint32 Frequency;
	FMetalShaderBindings Bindings;
	TArray<FMetalUniformBufferCopyInfo> UniformBuffersCopyInfo;
};

inline FArchive& operator<<(FArchive& Ar, FMetalCodeHeader& Header)
{
	Ar << Header.Frequency;
	Ar << Header.Bindings;

	int32 NumInfos = Header.UniformBuffersCopyInfo.Num();
	Ar << NumInfos;
	if (Ar.IsSaving())
	{
		for (int32 Index = 0; Index < NumInfos; ++Index)
		{
			Ar << Header.UniformBuffersCopyInfo[Index];
		}
	}
	else if (Ar.IsLoading())
	{
		Header.UniformBuffersCopyInfo.Empty(NumInfos);
		for (int32 Index = 0; Index < NumInfos; ++Index)
		{
			FMetalUniformBufferCopyInfo Info;
			Ar << Info;
			Header.UniformBuffersCopyInfo.Add(Info);
		}
	}

	return Ar;
}
