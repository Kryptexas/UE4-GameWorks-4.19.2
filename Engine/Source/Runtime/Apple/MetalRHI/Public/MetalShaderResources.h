// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalShaderResources.h: Metal shader resource RHI definitions.
=============================================================================*/

#pragma once

#include "CrossCompilerCommon.h"

/**
* Shader related constants.
*/
enum
{
	METAL_MAX_UNIFORM_BUFFER_BINDINGS = 12,	// @todo-mobile: Remove me
	METAL_FIRST_UNIFORM_BUFFER = 0,			// @todo-mobile: Remove me
	METAL_MAX_COMPUTE_STAGE_UAV_UNITS = 8,	// @todo-mobile: Remove me
	METAL_UAV_NOT_SUPPORTED_FOR_GRAPHICS_UNIT = -1, // for now, only CS supports UAVs/ images
	METAL_MAX_BUFFERS = 31,
};

/**
* Buffer data-types for MetalRHI & MetalSL
*/
enum EMetalBufferFormat
{
	Unknown					=0,
	
	R8Sint					=1,
	R8Uint					=2,
	R8Snorm					=3,
	R8Unorm					=4,
	
	R16Sint					=5,
	R16Uint					=6,
	R16Snorm				=7,
	R16Unorm				=8,
	R16Half					=9,
	
	R32Sint					=10,
	R32Uint					=11,
	R32Float				=12,
	
	RG8Sint					=13,
	RG8Uint					=14,
	RG8Snorm				=15,
	RG8Unorm				=16,
	
	RG16Sint				=17,
	RG16Uint				=18,
	RG16Snorm				=19,
	RG16Unorm				=20,
	RG16Half				=21,
	
	RG32Sint				=22,
	RG32Uint				=23,
	RG32Float				=24,
	
	RGB8Sint				=25,
	RGB8Uint				=26,
	RGB8Snorm				=27,
	RGB8Unorm				=28,
	
	RGB16Sint				=29,
	RGB16Uint				=30,
	RGB16Snorm				=31,
	RGB16Unorm				=32,
	RGB16Half				=33,
	
	RGB32Sint				=34,
	RGB32Uint				=35,
	RGB32Float				=36,
	
	RGBA8Sint				=37,
	RGBA8Uint				=38,
	RGBA8Snorm				=39,
	RGBA8Unorm				=40,
	
	BGRA8Unorm				=41,
	
	RGBA16Sint				=42,
	RGBA16Uint				=43,
	RGBA16Snorm				=44,
	RGBA16Unorm				=45,
	RGBA16Half				=46,
	
	RGBA32Sint				=47,
	RGBA32Uint				=48,
	RGBA32Float				=49,
	
	RGB10A2Unorm			=50,
	
	RG11B10Half 			=51,
	
	R5G6B5Unorm         	=52,
	
	Max						=53
};

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
		if (FMemory::Memcmp(A.TextureMap.GetData(), B.TextureMap.GetData(), A.TextureMap.GetTypeSize()*A.TextureMap.Num()) != 0)
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
	TArray<TArray<CrossCompiler::FPackedArrayInfo>>	PackedUniformBuffers;
	TArray<CrossCompiler::FPackedArrayInfo>			PackedGlobalArrays;
	FMetalShaderResourceTable				ShaderResourceTable;
	TArray<uint8> 							TypedBufferFormats;

    uint32  LinearBuffer;
	uint32	TypedBuffers;
	uint32 	InvariantBuffers;
	uint16	InOutMask;
	uint8	NumSamplers;
	uint8	NumUniformBuffers;
	uint8	NumUAVs;
	bool	bHasRegularUniformBuffers;
	bool	bDiscards;

	FMetalShaderBindings() :
		LinearBuffer(0),
        TypedBuffers(0),
		InvariantBuffers(0),
		InOutMask(0),
		NumSamplers(0),
		NumUniformBuffers(0),
		NumUAVs(0),
		bHasRegularUniformBuffers(false),
		bDiscards(false)
	{
	}
};

inline FArchive& operator<<(FArchive& Ar, FMetalShaderBindings& Bindings)
{
	Ar << Bindings.PackedUniformBuffers;
	Ar << Bindings.PackedGlobalArrays;
	Ar << Bindings.ShaderResourceTable;
	Ar << Bindings.TypedBufferFormats;
    Ar << Bindings.LinearBuffer;
    Ar << Bindings.TypedBuffers;
	Ar << Bindings.InvariantBuffers;
	Ar << Bindings.InOutMask;
	Ar << Bindings.NumSamplers;
	Ar << Bindings.NumUniformBuffers;
	Ar << Bindings.NumUAVs;
	Ar << Bindings.bHasRegularUniformBuffers;
	Ar << Bindings.bDiscards;
	return Ar;
}

enum class EMetalOutputWindingMode : uint8
{
	Clockwise = 0,
	CounterClockwise = 1,
};

enum class EMetalPartitionMode : uint8
{
	Pow2 = 0,
	Integer = 1,
	FractionalOdd = 2,
	FractionalEven = 3,
};

enum class EMetalComponentType : uint8
{
	Uint = 0,
	Int,
	Half,
	Float,
	Bool,
	Max
};

struct FMetalAttribute
{
	uint32 Index;
	EMetalComponentType Type;
	uint32 Components;
	uint32 Offset;
	
	friend FArchive& operator<<(FArchive& Ar, FMetalAttribute& Attr)
	{
		Ar << Attr.Index;
		Ar << Attr.Type;
		Ar << Attr.Components;
		Ar << Attr.Offset;
		return Ar;
	}
};

struct FMetalTessellationOutputs
{
	uint32 HSOutSize;
	uint32 HSTFOutSize;
	uint32 PatchControlPointOutSize;
	TArray<FMetalAttribute> HSOut;
	TArray<FMetalAttribute> PatchControlPointOut;
	
	friend FArchive& operator<<(FArchive& Ar, FMetalTessellationOutputs& Attrs)
	{
		Ar << Attrs.HSOutSize;
		Ar << Attrs.HSTFOutSize;
		Ar << Attrs.PatchControlPointOutSize;
		Ar << Attrs.HSOut;
		Ar << Attrs.PatchControlPointOut;
		return Ar;
	}
};

struct FMetalCodeHeader
{
	uint32 Frequency;
	FMetalShaderBindings Bindings;
	TArray<CrossCompiler::FUniformBufferCopyInfo> UniformBuffersCopyInfo;
    FString ShaderName;
    
    FMetalTessellationOutputs TessellationOutputAttribs;

	uint64 CompilerBuild;
	uint32 CompilerVersion;

	uint32 TessellationOutputControlPoints;
	uint32 TessellationDomain; // 3 = tri, 4 = quad // TODO unused
	uint32 TessellationInputControlPoints; // TODO unused
	uint32 TessellationPatchesPerThreadGroup;
    uint32 TessellationPatchCountBuffer;
    uint32 TessellationIndexBuffer;
    uint32 TessellationHSOutBuffer;
    uint32 TessellationHSTFOutBuffer;
    uint32 TessellationControlPointOutBuffer;
    uint32 TessellationControlPointIndexBuffer;
	float  TessellationMaxTessFactor;

	uint32 SourceLen;
	uint32 SourceCRC;
	
	uint16 CompileFlags;
	
	uint32 NumThreadsX;
	uint32 NumThreadsY;
	uint32 NumThreadsZ;
	
	uint8 Version;
	int8 SideTable;
	
	EMetalOutputWindingMode TessellationOutputWinding;
	EMetalPartitionMode TessellationPartitioning;
	
	bool bTessFunctionConstants;
	bool bDeviceFunctionConstants;
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
			CrossCompiler::FUniformBufferCopyInfo Info;
			Ar << Info;
			Header.UniformBuffersCopyInfo.Add(Info);
		}
	}
	
	Ar << Header.ShaderName;

    Ar << Header.TessellationOutputAttribs;

	Ar << Header.CompilerVersion;
	Ar << Header.CompilerBuild;
    
	Ar << Header.TessellationOutputControlPoints;
	Ar << Header.TessellationDomain;
	Ar << Header.TessellationInputControlPoints;
	Ar << Header.TessellationPatchesPerThreadGroup;
	Ar << Header.TessellationMaxTessFactor;
    
    Ar << Header.TessellationPatchCountBuffer;
    Ar << Header.TessellationIndexBuffer;
    Ar << Header.TessellationHSOutBuffer;
    Ar << Header.TessellationHSTFOutBuffer;
    Ar << Header.TessellationControlPointOutBuffer;
    Ar << Header.TessellationControlPointIndexBuffer;
	
	Ar << Header.SourceLen;
	Ar << Header.SourceCRC;
	
	Ar << Header.CompileFlags;
	
	Ar << Header.NumThreadsX;
	Ar << Header.NumThreadsY;
	Ar << Header.NumThreadsZ;
	
	Ar << Header.Version;
	Ar << Header.SideTable;
	
	Ar << Header.TessellationOutputWinding;
	Ar << Header.TessellationPartitioning;
	Ar << Header.bTessFunctionConstants;
	Ar << Header.bDeviceFunctionConstants;

    return Ar;
}

struct FMetalShaderMap
{
	FString Format;
	TMap<FSHAHash, TPair<uint8, TArray<uint8>>> HashMap;
	
	friend FArchive& operator<<(FArchive& Ar, FMetalShaderMap& Header)
	{
		return Ar << Header.Format << Header.HashMap;
	}
};
