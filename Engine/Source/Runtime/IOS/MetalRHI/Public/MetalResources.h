// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalResources.h: Metal resource RHI definitions.
=============================================================================*/

#pragma once

#include "BoundShaderStateCache.h"
#include "MetalShaderResources.h"

/** This represents a vertex declaration that hasn't been combined with a specific shader to create a bound shader. */
class FMetalVertexDeclaration : public FRHIVertexDeclaration
{
public:

	/** Initialization constructor. */
	FMetalVertexDeclaration(const FVertexDeclarationElementList& InElements);
	~FMetalVertexDeclaration();
	
	/** Cached element info array (offset, stream index, etc) */
	FVertexDeclarationElementList Elements;

	/** This is the layout for the vertex elements */
 	MTLVertexDescriptor* Layout;

protected:
	void GenerateLayout(const FVertexDeclarationElementList& Elements);

};



/** This represents a vertex shader that hasn't been combined with a specific declaration to create a bound shader. */
template<typename BaseResourceType /*, typename ShaderType */>
class TMetalBaseShader : public BaseResourceType, public IRefCountedObject
{
public:

	/** Initialization constructor. */
	TMetalBaseShader()
	{
	}
	TMetalBaseShader(const TArray<uint8>& InCode);

	/** Destructor */
	virtual ~TMetalBaseShader();


	// IRefCountedObject interface.
	virtual uint32 AddRef() const
	{
		return FRefCountedObject::AddRef();
	}
	virtual uint32 Release() const
	{
		return FRefCountedObject::Release();
	}
	virtual uint32 GetRefCount() const
	{
		return FRefCountedObject::GetRefCount();
	}
	
	// this is the compiler shader
	id<MTLFunction> Function;

 	/** External bindings for this shader. */
	FMetalShaderBindings Bindings;
	TArray< TRefCountPtr<FRHIUniformBuffer> > BoundUniformBuffers;
	// List of memory copies from RHIUniformBuffer to packed uniforms
	TArray<FMetalUniformBufferCopyInfo> UniformBuffersCopyInfo;
    
	TArray<ANSICHAR> GlslCode;
    NSString* GlslCodeNSString;
	const ANSICHAR*  GlslCodeString; // make it easier in VS to see shader code in debug mode; points to begin of GlslCode
};

typedef TMetalBaseShader<FRHIVertexShader> FMetalVertexShader;
typedef TMetalBaseShader<FRHIPixelShader> FMetalPixelShader;
typedef TMetalBaseShader<FRHIHullShader> FMetalHullShader;
typedef TMetalBaseShader<FRHIDomainShader> FMetalDomainShader;
typedef TMetalBaseShader<FRHIComputeShader> FMetalComputeShader;
typedef TMetalBaseShader<FRHIGeometryShader> FMetalGeometryShader;


/**
 * Combined shader state and vertex definition for rendering geometry. 
 * Each unique instance consists of a vertex decl, vertex shader, and pixel shader.
 */
class FMetalBoundShaderState : public FRHIBoundShaderState
{
public:

	FCachedBoundShaderStateLink CacheLink;

	/** Cached vertex structure */
	TRefCountPtr<FMetalVertexDeclaration> VertexDeclaration;

	/** Cached shaders */
	TRefCountPtr<FMetalVertexShader> VertexShader;
	TRefCountPtr<FMetalPixelShader> PixelShader;
	TRefCountPtr<FMetalHullShader> HullShader;
	TRefCountPtr<FMetalDomainShader> DomainShader;
	TRefCountPtr<FMetalGeometryShader> GeometryShader;

	/** Initialization constructor. */
	FMetalBoundShaderState(
		FVertexDeclarationRHIParamRef InVertexDeclarationRHI,
		FVertexShaderRHIParamRef InVertexShaderRHI,
		FPixelShaderRHIParamRef InPixelShaderRHI,
		FHullShaderRHIParamRef InHullShaderRHI,
		FDomainShaderRHIParamRef InDomainShaderRHI,
		FGeometryShaderRHIParamRef InGeometryShaderRHI);

	/**
	 *Destructor
	 */
	~FMetalBoundShaderState();
	
	/**
	 * Prepare a pipeline state object for the current state right before drawing
	 */
	void PrepareToDraw(const struct FPipelineShadow& Pipeline);

protected:
	TMap<uint64, id<MTLRenderPipelineState> > PipelineStates;
};


/** Texture/RT wrapper. */
class FMetalSurface
{
public:

	/** 
	 * Constructor that will create Texture and Color/DepthBuffers as needed
	 */
	FMetalSurface(ERHIResourceType ResourceType, EPixelFormat Format, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint32 NumSamples, bool bArray, uint32 ArraySize, uint32 NumMips, uint32 Flags, FResourceBulkDataInterface* BulkData);

	/**
	 * Destructor
	 */
	~FMetalSurface();

	/**
	 * Locks one of the texture's mip-maps.
	 * @param ArrayIndex Index of the texture array/face in the form Index*6+Face
	 * @return A pointer to the specified texture data.
	 */
	void* Lock(uint32 MipIndex, uint32 ArrayIndex, EResourceLockMode LockMode, uint32& DestStride);

	/** Unlocks a previously locked mip-map.
	 * @param ArrayIndex Index of the texture array/face in the form Index*6+Face
	 */
	void Unlock(uint32 MipIndex, uint32 ArrayIndex);

	/**
	 * Returns how much memory is used by the surface
	 */
	uint32 GetMemorySize();

	EPixelFormat PixelFormat;
	id<MTLTexture> Texture;
    id<MTLTexture> MSAATexture;
	uint32 SizeX, SizeY, SizeZ;
	bool bIsCubemap;
	
	uint32 Flags;
	void* LockedMemory;
};

class FMetalTexture2D : public FRHITexture2D
{
public:
	/** The surface info */
	FMetalSurface Surface;

	// Constructor, just calls base and Surface constructor
	FMetalTexture2D(EPixelFormat Format, uint32 SizeX, uint32 SizeY, uint32 NumMips, uint32 NumSamples, uint32 Flags, FResourceBulkDataInterface* BulkData)
		: FRHITexture2D(SizeX, SizeY, NumMips, NumSamples, Format, Flags)
		, Surface(RRT_Texture2D, Format, SizeX, SizeY, 1, NumSamples, /*bArray=*/ false, 1, NumMips, Flags, BulkData)
	{
	}
    
    
};

class FMetalTexture2DArray : public FRHITexture2DArray
{
public:
	/** The surface info */
	FMetalSurface Surface;

	// Constructor, just calls base and Surface constructor
	FMetalTexture2DArray(EPixelFormat Format, uint32 SizeX, uint32 SizeY, uint32 ArraySize, uint32 NumMips, uint32 Flags, FResourceBulkDataInterface* BulkData)
		: FRHITexture2DArray(SizeX, SizeY, ArraySize, NumMips, Format, Flags)
		, Surface(RRT_Texture2DArray, Format, SizeX, SizeY, 1, /*NumSamples=*/1, /*bArray=*/ true, ArraySize, NumMips, Flags, BulkData)
	{
	}
};

class FMetalTexture3D : public FRHITexture3D
{
public:
	/** The surface info */
	FMetalSurface Surface;

	// Constructor, just calls base and Surface constructor
	FMetalTexture3D(EPixelFormat Format, uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint32 NumMips, uint32 Flags, FResourceBulkDataInterface* BulkData)
		: FRHITexture3D(SizeX, SizeY, SizeZ, NumMips, Format, Flags)
		, Surface(RRT_Texture3D, Format, SizeX, SizeY, SizeZ, /*NumSamples=*/1, /*bArray=*/ false, 1, NumMips, Flags, BulkData)
	{
	}
};

class FMetalTextureCube : public FRHITextureCube
{
public:
	/** The surface info */
	FMetalSurface Surface;

	// Constructor, just calls base and Surface constructor
	FMetalTextureCube(EPixelFormat Format, uint32 Size, bool bArray, uint32 ArraySize, uint32 NumMips, uint32 Flags, FResourceBulkDataInterface* BulkData)
		: FRHITextureCube(Size, NumMips, Format, Flags)
		, Surface(RRT_TextureCube, Format, Size, Size, 6, /*NumSamples=*/1, bArray, ArraySize, NumMips, Flags, BulkData)
	{
	}
};

/** Given a pointer to a RHI texture that was created by the Metal RHI, returns a pointer to the FMetalTextureBase it encapsulates. */
FMetalSurface& GetSurfaceFromRHITexture(FRHITexture* Texture);





/** Metal occlusion query */
class FMetalRenderQuery : public FRHIRenderQuery
{
public:

	/** Initialization constructor. */
	FMetalRenderQuery(ERenderQueryType InQueryType);

	~FMetalRenderQuery();

	/**
	 * Kick off an occlusion test 
	 */
	void Begin();

	/**
	 * Finish up an occlusion test 
	 */
	void End();

	// Where in the ring buffer to store the result
	uint32 Offset;

	// The command buffer this query is being rendered in - used to make sure the command buffer is complete whewn we read the result
	uint64 CommandBufferIndex;
};

/** Index buffer resource class that stores stride information. */
class FMetalIndexBuffer : public FRHIIndexBuffer
{
public:

	/** Constructor */
	FMetalIndexBuffer(uint32 InStride, uint32 InSize, uint32 InUsage);
	~FMetalIndexBuffer();

	/**
	 * Prepare a CPU accessible buffer for uploading to GPU memory
	 */
	void* Lock(EResourceLockMode LockMode, uint32 Size=0);

	/**
	 * Prepare a CPU accessible buffer for uploading to GPU memory
	 */
	void Unlock();
	
	// balsa buffer memory
	id<MTLBuffer> Buffer;

	// offet into the buffer (for ring buffer usage)
	uint32 Offset;
	
	// 16- or 32-bit
	MTLIndexType IndexType;
};


/** Vertex buffer resource class that stores usage type. */
class FMetalVertexBuffer : public FRHIVertexBuffer
{
public:

	/** Constructor */
	FMetalVertexBuffer(uint32 InSize, uint32 InUsage);
	~FMetalVertexBuffer();

	/**
	 * Prepare a CPU accessible buffer for uploading to GPU memory
	 */
	void* Lock(EResourceLockMode LockMode, uint32 Size=0);

	/**
	 * Prepare a CPU accessible buffer for uploading to GPU memory
	 */
	void Unlock();

	// balsa buffer memory
	id<MTLBuffer> Buffer;

	// offset into the buffer (for ring buffer usage)
	uint32 Offset;

	// if the buffer is a zero stride buffer, we need to duplicate and grow on demand, this is the size of one element to copy
	uint32 ZeroStrideElementSize;
};

class FMetalUniformBuffer : public FRHIUniformBuffer
{
public:

	// Constructor
	FMetalUniformBuffer(const void* Contents, const FRHIUniformBufferLayout& Layout, EUniformBufferUsage Usage);

	// Destructor 
	~FMetalUniformBuffer();
	
	/** The buffer containing the contents - either a fresh buffer or the ring buffer */
	id<MTLBuffer> Buffer;
	
	/** This offset is used when passing to setVertexBuffer, etc */
	uint32 Offset;
};


class FMetalStructuredBuffer : public FRHIStructuredBuffer
{
public:
	// Constructor
	FMetalStructuredBuffer(uint32 Stride, uint32 Size, FResourceArrayInterface* ResourceArray, uint32 InUsage);

	// Destructor
	~FMetalStructuredBuffer();

};



class FMetalUnorderedAccessView : public FRHIUnorderedAccessView
{
public:

	// the potential resources to refer to with the UAV object
	TRefCountPtr<FMetalStructuredBuffer> SourceStructuredBuffer;
	TRefCountPtr<FMetalVertexBuffer> SourceVertexBuffer;
	TRefCountPtr<FRHITexture> SourceTexture;
};


class FMetalShaderResourceView : public FRHIShaderResourceView
{
public:

	// The vertex buffer this SRV comes from (can be null)
	TRefCountPtr<FMetalVertexBuffer> SourceVertexBuffer;

	// The texture that this SRV come from
	TRefCountPtr<FRHITexture> SourceTexture;

	~FMetalShaderResourceView();
};

class FMetalShaderParameterCache
{
public:
	/** Constructor. */
	FMetalShaderParameterCache();

	/** Destructor. */
	~FMetalShaderParameterCache();

	void InitializeResources(int32 UniformArraySize);

	/**
	 * Marks all uniform arrays as dirty.
	 */
	void MarkAllDirty();

	/**
	 * Sets values directly into the packed uniform array
	 */
	void Set(uint32 BufferIndex, uint32 ByteOffset, uint32 NumBytes, const void* NewValues);

	/**
	 * Commit shader parameters to the currently bound program.
	 */
	void CommitPackedGlobals(int32 Stage, const FMetalShaderBindings& Bindings);
	void CommitPackedUniformBuffers(TRefCountPtr<FMetalBoundShaderState> BoundShaderState, int32 Stage, const TArray< TRefCountPtr<FRHIUniformBuffer> >& UniformBuffers, const TArray<FMetalUniformBufferCopyInfo>& UniformBuffersCopyInfo);

private:
	/** CPU memory block for storing uniform values. */
	uint8* PackedGlobalUniforms[METAL_PACKED_TYPEINDEX_MAX];

	struct FRange
	{
		uint32	LowVector;
		uint32	HighVector;
	};
	/** Dirty ranges for each uniform array. */
	FRange	PackedGlobalUniformDirty[METAL_PACKED_TYPEINDEX_MAX];

	/** Scratch CPU memory block for uploading packed uniforms. */
	uint8* PackedUniformsScratch[METAL_PACKED_TYPEINDEX_MAX];

	int32 GlobalUniformArraySize;
};
