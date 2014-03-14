// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "RHI.h"

/** Encapsulates a GPU read/write buffer with its UAV and SRV. */
struct FRWBuffer
{
	FVertexBufferRHIRef Buffer;
	FUnorderedAccessViewRHIRef UAV;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FRWBuffer(): NumBytes(0) {}

	void Initialize(uint32 BytesPerElement,uint32 NumElements,EPixelFormat Format,uint32 AdditionalUsage = 0)
	{
		check(GRHIFeatureLevel == ERHIFeatureLevel::SM5);
		NumBytes = BytesPerElement * NumElements;
		Buffer = RHICreateVertexBuffer(NumBytes,NULL,BUF_UnorderedAccess | BUF_ShaderResource | AdditionalUsage);
		UAV = RHICreateUnorderedAccessView(Buffer,Format);
		SRV = RHICreateShaderResourceView(Buffer,BytesPerElement,Format);
	}

	void Release()
	{
		NumBytes = 0;
		Buffer.SafeRelease();
		UAV.SafeRelease();
		SRV.SafeRelease();
	}
};

/** Encapsulates a GPU read buffer with its SRV. */
struct FReadBuffer
{
	FVertexBufferRHIRef Buffer;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FReadBuffer(): NumBytes(0) {}

	void Initialize(uint32 BytesPerElement,uint32 NumElements,EPixelFormat Format,uint32 AdditionalUsage = 0)
	{
		check(GRHIFeatureLevel >= ERHIFeatureLevel::SM4);
		NumBytes = BytesPerElement * NumElements;
		Buffer = RHICreateVertexBuffer(NumBytes,NULL,BUF_ShaderResource | AdditionalUsage);
		SRV = RHICreateShaderResourceView(Buffer,BytesPerElement,Format);
	}

	void Release()
	{
		NumBytes = 0;
		Buffer.SafeRelease();
		SRV.SafeRelease();
	}
};

/** Encapsulates a GPU read/write structured buffer with its UAV and SRV. */
struct FRWBufferStructured
{
	FStructuredBufferRHIRef Buffer;
	FUnorderedAccessViewRHIRef UAV;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FRWBufferStructured(): NumBytes(0) {}

	void Initialize( uint32 BytesPerElement, uint32 NumElements, uint32 AdditionalUsage = 0, bool bUseUavCounter = false, bool bAppendBuffer = false )
	{
		check(GRHIFeatureLevel == ERHIFeatureLevel::SM5);
		NumBytes = BytesPerElement * NumElements;
		Buffer = RHICreateStructuredBuffer(BytesPerElement, NumBytes, NULL, BUF_UnorderedAccess | BUF_ShaderResource | AdditionalUsage );
		UAV = RHICreateUnorderedAccessView(Buffer,bUseUavCounter, bAppendBuffer);
		SRV = RHICreateShaderResourceView(Buffer);
	}

	void Release()
	{
		NumBytes = 0;
		Buffer.SafeRelease();
		UAV.SafeRelease();
		SRV.SafeRelease();
	}
};

/** Encapsulates a GPU read/write ByteAddress buffer with its UAV and SRV. */
struct FRWBufferByteAddress
{
	FStructuredBufferRHIRef Buffer;
	FUnorderedAccessViewRHIRef UAV;
	FShaderResourceViewRHIRef SRV;
	uint32 NumBytes;

	FRWBufferByteAddress(): NumBytes(0) {}

	void Initialize( uint32 InNumBytes, uint32 AdditionalUsage = 0 )
	{
		NumBytes = InNumBytes;
		check( GRHIFeatureLevel == ERHIFeatureLevel::SM5 );
		check( NumBytes % 4 == 0 );
		Buffer = RHICreateStructuredBuffer(4, NumBytes, NULL, BUF_UnorderedAccess | BUF_ShaderResource | BUF_ByteAddressBuffer | AdditionalUsage );
		UAV = RHICreateUnorderedAccessView(Buffer, false, false );
		SRV = RHICreateShaderResourceView(Buffer);
	}

	void Release()
	{
		NumBytes = 0;
		Buffer.SafeRelease();
		UAV.SafeRelease();
		SRV.SafeRelease();
	}
};

/** Helper for the common case of using a single color and depth render target. */
inline void RHISetRenderTarget(FTextureRHIParamRef NewRenderTarget, FTextureRHIParamRef NewDepthStencilTarget)
{
	FRHIRenderTargetView RTV(NewRenderTarget);
	RHISetRenderTargets(1, &RTV, NewDepthStencilTarget, 0, NULL);
}

/** Helper for the common case of using a single color and depth render target, with a mip index for the color target. */
inline void RHISetRenderTarget(FTextureRHIParamRef NewRenderTarget, int32 MipIndex, FTextureRHIParamRef NewDepthStencilTarget)
{
	FRHIRenderTargetView RTV(NewRenderTarget, MipIndex, 0);
	RHISetRenderTargets(1, &RTV, NewDepthStencilTarget, 0, NULL);
}

/** Helper for the common case of using a single color and depth render target, with a mip index for the color target. */
inline void RHISetRenderTarget(FTextureRHIParamRef NewRenderTarget, int32 MipIndex, int32 ArraySliceIndex, FTextureRHIParamRef NewDepthStencilTarget)
{
	FRHIRenderTargetView RTV(NewRenderTarget, MipIndex, ArraySliceIndex);
	RHISetRenderTargets(1, &RTV, NewDepthStencilTarget, 0, NULL);
}

/** Helper that converts FTextureRHIParamRef's into FRHIRenderTargetView's. */
inline void RHISetRenderTargets(
	uint32 NewNumSimultaneousRenderTargets, 
	const FTextureRHIParamRef* NewRenderTargetsRHI,
	FTextureRHIParamRef NewDepthStencilTargetRHI,
	uint32 NewNumUAVs,
	const FUnorderedAccessViewRHIParamRef* UAVs
	)
{
	FRHIRenderTargetView RTVs[MaxSimultaneousRenderTargets];

	for (uint32 Index = 0; Index < NewNumSimultaneousRenderTargets; Index++)
	{
		RTVs[Index] = FRHIRenderTargetView(NewRenderTargetsRHI[Index]);
	}

	RHISetRenderTargets(NewNumSimultaneousRenderTargets, RTVs, NewDepthStencilTargetRHI, NewNumUAVs, UAVs);
}

/** Sets a depth-stencil state with a default stencil ref value of 0. */
inline void RHISetDepthStencilState(FDepthStencilStateRHIParamRef NewState)
{
	RHISetDepthStencilState(NewState,0);
}

/** Sets a blend state with a default blend factor of white. */
inline void RHISetBlendState(FBlendStateRHIParamRef NewState)
{
	RHISetBlendState(NewState,FLinearColor::White);
}

/**
 * Creates 1 or 2 textures with the same dimensions/format.
 * If the RHI supports textures that can be used as both shader resources and render targets,
 * and bForceSeparateTargetAndShaderResource=false, then a single texture is created.
 * Otherwise two textures are created, one of them usable as a shader resource and resolve target, and one of them usable as a render target.
 * Two texture references are always returned, but they may reference the same texture.
 * If two different textures are returned, the render-target texture must be manually copied to the shader-resource texture.
 */
inline void RHICreateTargetableShaderResource2D(
	uint32 SizeX,
	uint32 SizeY,
	uint8 Format,
	uint32 NumMips,
	uint32 Flags,
	uint32 TargetableTextureFlags,
	bool bForceSeparateTargetAndShaderResource,
	FTexture2DRHIRef& OutTargetableTexture,
	FTexture2DRHIRef& OutShaderResourceTexture,
	uint32 NumSamples=1
	)
{
	// Ensure none of the usage flags are passed in.
	check(!(Flags & TexCreate_RenderTargetable));
	check(!(Flags & TexCreate_ResolveTargetable));
	check(!(Flags & TexCreate_ShaderResource));

	// Ensure that all of the flags provided for the targetable texture are not already passed in Flags.
	check(!(Flags & TargetableTextureFlags));

	// Ensure that the targetable texture is either render or depth-stencil targetable.
	check(TargetableTextureFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable | TexCreate_UAV));

	if(NumSamples > 1) 
	{
		bForceSeparateTargetAndShaderResource = true;
	}

	// ES2 doesn't support resolve operations.
	if (GRHIFeatureLevel <= ERHIFeatureLevel::ES2)
	{
		bForceSeparateTargetAndShaderResource = false;
	}

	if(!bForceSeparateTargetAndShaderResource/* && GSupportsRenderDepthTargetableShaderResources*/)
	{
		// Create a single texture that has both TargetableTextureFlags and TexCreate_ShaderResource set.
		OutTargetableTexture = OutShaderResourceTexture = RHICreateTexture2D(SizeX,SizeY,Format,NumMips,NumSamples,Flags | TargetableTextureFlags | TexCreate_ShaderResource,NULL);
	}
	else
	{
		// Create a texture that has TargetableTextureFlags set, and a second texture that has TexCreate_ResolveTargetable and TexCreate_ShaderResource set.
		OutTargetableTexture = RHICreateTexture2D(SizeX,SizeY,Format,NumMips,NumSamples,Flags | TargetableTextureFlags,NULL);
		OutShaderResourceTexture = RHICreateTexture2D(SizeX,SizeY,Format,NumMips,1,Flags | TexCreate_ResolveTargetable | TexCreate_ShaderResource,NULL);
	}
}

/**
 * Creates 1 or 2 textures with the same dimensions/format.
 * If the RHI supports textures that can be used as both shader resources and render targets,
 * and bForceSeparateTargetAndShaderResource=false, then a single texture is created.
 * Otherwise two textures are created, one of them usable as a shader resource and resolve target, and one of them usable as a render target.
 * Two texture references are always returned, but they may reference the same texture.
 * If two different textures are returned, the render-target texture must be manually copied to the shader-resource texture.
 */
inline void RHICreateTargetableShaderResourceCube(
	uint32 LinearSize,
	uint8 Format,
	uint32 NumMips,
	uint32 Flags,
	uint32 TargetableTextureFlags,
	bool bForceSeparateTargetAndShaderResource,
	FTextureCubeRHIRef& OutTargetableTexture,
	FTextureCubeRHIRef& OutShaderResourceTexture
	)
{
	// Ensure none of the usage flags are passed in.
	check(!(Flags & TexCreate_RenderTargetable));
	check(!(Flags & TexCreate_ResolveTargetable));
	check(!(Flags & TexCreate_ShaderResource));

	// Ensure that all of the flags provided for the targetable texture are not already passed in Flags.
	check(!(Flags & TargetableTextureFlags));

	// Ensure that the targetable texture is either render or depth-stencil targetable.
	check(TargetableTextureFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable));

	// ES2 doesn't support resolve operations.
	if (GRHIFeatureLevel <= ERHIFeatureLevel::ES2)
	{
		bForceSeparateTargetAndShaderResource = false;
	}

	if(!bForceSeparateTargetAndShaderResource/* && GSupportsRenderDepthTargetableShaderResources*/)
	{
		// Create a single texture that has both TargetableTextureFlags and TexCreate_ShaderResource set.
		OutTargetableTexture = OutShaderResourceTexture = RHICreateTextureCube(LinearSize,Format,NumMips,Flags | TargetableTextureFlags | TexCreate_ShaderResource,NULL);
	}
	else
	{
		// Create a texture that has TargetableTextureFlags set, and a second texture that has TexCreate_ResolveTargetable and TexCreate_ShaderResource set.
		OutTargetableTexture = RHICreateTextureCube(LinearSize,Format,NumMips,Flags | TargetableTextureFlags,NULL);
		OutShaderResourceTexture = RHICreateTextureCube(LinearSize,Format,NumMips,Flags | TexCreate_ResolveTargetable | TexCreate_ShaderResource,NULL);
	}
}

/**
 * Creates 1 or 2 textures with the same dimensions/format.
 * If the RHI supports textures that can be used as both shader resources and render targets,
 * and bForceSeparateTargetAndShaderResource=false, then a single texture is created.
 * Otherwise two textures are created, one of them usable as a shader resource and resolve target, and one of them usable as a render target.
 * Two texture references are always returned, but they may reference the same texture.
 * If two different textures are returned, the render-target texture must be manually copied to the shader-resource texture.
 */
inline void RHICreateTargetableShaderResourceCubeArray(
	uint32 LinearSize,
	uint32 ArraySize,
	uint8 Format,
	uint32 NumMips,
	uint32 Flags,
	uint32 TargetableTextureFlags,
	bool bForceSeparateTargetAndShaderResource,
	FTextureCubeRHIRef& OutTargetableTexture,
	FTextureCubeRHIRef& OutShaderResourceTexture
	)
{
	// Ensure none of the usage flags are passed in.
	check(!(Flags & TexCreate_RenderTargetable));
	check(!(Flags & TexCreate_ResolveTargetable));
	check(!(Flags & TexCreate_ShaderResource));

	// Ensure that all of the flags provided for the targetable texture are not already passed in Flags.
	check(!(Flags & TargetableTextureFlags));

	// Ensure that the targetable texture is either render or depth-stencil targetable.
	check(TargetableTextureFlags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable));

	if(!bForceSeparateTargetAndShaderResource/* && GSupportsRenderDepthTargetableShaderResources*/)
	{
		// Create a single texture that has both TargetableTextureFlags and TexCreate_ShaderResource set.
		OutTargetableTexture = OutShaderResourceTexture = RHICreateTextureCubeArray(LinearSize,ArraySize,Format,NumMips,Flags | TargetableTextureFlags | TexCreate_ShaderResource,NULL);
	}
	else
	{
		// Create a texture that has TargetableTextureFlags set, and a second texture that has TexCreate_ResolveTargetable and TexCreate_ShaderResource set.
		OutTargetableTexture = RHICreateTextureCubeArray(LinearSize,ArraySize,Format,NumMips,Flags | TargetableTextureFlags,NULL);
		OutShaderResourceTexture = RHICreateTextureCubeArray(LinearSize,ArraySize,Format,NumMips,Flags | TexCreate_ResolveTargetable | TexCreate_ShaderResource,NULL);
	}
}
/**
 * Computes the vertex count for a given number of primitives of the specified type.
 * @param NumPrimitives The number of primitives.
 * @param PrimitiveType The type of primitives.
 * @returns The number of vertices.
 */
inline uint32 RHIGetVertexCountForPrimitiveCount(uint32 NumPrimitives, uint32 PrimitiveType)
{
	uint32 VertexCount = 0;
	switch(PrimitiveType)
	{
	case PT_TriangleList: VertexCount = NumPrimitives*3; break;
	case PT_TriangleStrip: VertexCount = NumPrimitives+2; break;
	case PT_LineList: VertexCount = NumPrimitives*2; break;
	case PT_PointList: VertexCount = NumPrimitives; break;
	case PT_1_ControlPointPatchList:
	case PT_2_ControlPointPatchList:
	case PT_3_ControlPointPatchList: 
	case PT_4_ControlPointPatchList: 
	case PT_5_ControlPointPatchList:
	case PT_6_ControlPointPatchList:
	case PT_7_ControlPointPatchList: 
	case PT_8_ControlPointPatchList: 
	case PT_9_ControlPointPatchList: 
	case PT_10_ControlPointPatchList: 
	case PT_11_ControlPointPatchList: 
	case PT_12_ControlPointPatchList: 
	case PT_13_ControlPointPatchList: 
	case PT_14_ControlPointPatchList: 
	case PT_15_ControlPointPatchList: 
	case PT_16_ControlPointPatchList: 
	case PT_17_ControlPointPatchList: 
	case PT_18_ControlPointPatchList: 
	case PT_19_ControlPointPatchList: 
	case PT_20_ControlPointPatchList: 
	case PT_21_ControlPointPatchList: 
	case PT_22_ControlPointPatchList: 
	case PT_23_ControlPointPatchList: 
	case PT_24_ControlPointPatchList: 
	case PT_25_ControlPointPatchList: 
	case PT_26_ControlPointPatchList: 
	case PT_27_ControlPointPatchList: 
	case PT_28_ControlPointPatchList: 
	case PT_29_ControlPointPatchList: 
	case PT_30_ControlPointPatchList: 
	case PT_31_ControlPointPatchList: 
	case PT_32_ControlPointPatchList: 
		VertexCount = (PrimitiveType - PT_1_ControlPointPatchList + 1) * NumPrimitives;
		break;
	default: UE_LOG(LogRHI, Fatal,TEXT("Unknown primitive type: %u"),PrimitiveType);
	};

	return VertexCount;
}

/**
 * Draw a primitive using the vertices passed in.
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param NumPrimitives The number of primitives in the VertexData buffer
 * @param VertexData A reference to memory preallocate in RHIBeginDrawPrimitiveUP
 * @param VertexDataStride Size of each vertex
 */
inline void RHIDrawPrimitiveUP( uint32 PrimitiveType, uint32 NumPrimitives, const void* VertexData, uint32 VertexDataStride )
{
	void* Buffer = NULL;
	const uint32 VertexCount = RHIGetVertexCountForPrimitiveCount( NumPrimitives, PrimitiveType );
	RHIBeginDrawPrimitiveUP( PrimitiveType, NumPrimitives, VertexCount, VertexDataStride, Buffer );
	FMemory::Memcpy( Buffer, VertexData, VertexCount * VertexDataStride );
	RHIEndDrawPrimitiveUP();
}

/**
 * Draw a primitive using the vertices passed in as described the passed in indices. 
 * @param PrimitiveType The type (triangles, lineloop, etc) of primitive to draw
 * @param MinVertexIndex The lowest vertex index used by the index buffer
 * @param NumVertices The number of vertices in the vertex buffer
 * @param NumPrimitives THe number of primitives described by the index buffer
 * @param IndexData The memory preallocated in RHIBeginDrawIndexedPrimitiveUP
 * @param IndexDataStride The size of one index
 * @param VertexData The memory preallocate in RHIBeginDrawIndexedPrimitiveUP
 * @param VertexDataStride The size of one vertex
 */
inline void RHIDrawIndexedPrimitiveUP(
	uint32 PrimitiveType,
	uint32 MinVertexIndex,
	uint32 NumVertices,
	uint32 NumPrimitives,
	const void* IndexData,
	uint32 IndexDataStride,
	const void* VertexData,
	uint32 VertexDataStride )
{
	void* VertexBuffer = NULL;
	void* IndexBuffer = NULL;
	const uint32 NumIndices = RHIGetVertexCountForPrimitiveCount( NumPrimitives, PrimitiveType );
	RHIBeginDrawIndexedPrimitiveUP(
		PrimitiveType,
		NumPrimitives,
		NumVertices,
		VertexDataStride,
		VertexBuffer,
		MinVertexIndex,
		NumIndices,
		IndexDataStride,
		IndexBuffer );
	FMemory::Memcpy( VertexBuffer, VertexData, NumVertices * VertexDataStride );
	FMemory::Memcpy( IndexBuffer, IndexData, NumIndices * IndexDataStride );
	RHIEndDrawIndexedPrimitiveUP();
}
