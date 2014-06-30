// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "RHIDefinitions.h"
#include "RefCounting.h"
#include "Runtime/Engine/Public/PixelFormat.h" // for EPixelFormat

/** The base type of RHI resources. */
class RHI_API FRHIResource : public FRefCountedObject
{
public:
	virtual ~FRHIResource() {}
};

//
// State blocks
//

class FRHISamplerState : public FRHIResource {};
class FRHIRasterizerState : public FRHIResource {};
class FRHIDepthStencilState : public FRHIResource {};
class FRHIBlendState : public FRHIResource {};

//
// Shader bindings
//

class FRHIVertexDeclaration : public FRHIResource {};
class FRHIBoundShaderState : public FRHIResource {};

//
// Shaders
//

class FRHIVertexShader : public FRHIResource {};
class FRHIHullShader : public FRHIResource {};
class FRHIDomainShader : public FRHIResource {};
class FRHIPixelShader : public FRHIResource {};
class FRHIGeometryShader : public FRHIResource {};
class FRHIComputeShader : public FRHIResource {};

//
// Buffers
//

/** The layout of a uniform buffer in memory. */
struct FRHIUniformBufferLayout
{
	/** The size of the constant buffer in bytes. */
	uint32 ConstantBufferSize;
	/** The offset to the beginning of the resource table. */
	uint32 ResourceOffset;
	/** The type of each resource (EUniformBufferBaseType). */
	TArray<uint8> Resources;

	uint32 GetHash() const
	{
		if (!bComputedHash)
		{
			uint32 TmpHash = ConstantBufferSize;
			TmpHash ^= ResourceOffset;
			uint32 N = Resources.Num();
			while (N >= 4)
			{
				TmpHash ^= (Resources[--N] << 0);
				TmpHash ^= (Resources[--N] << 8);
				TmpHash ^= (Resources[--N] << 16);
				TmpHash ^= (Resources[--N] << 24);
			}
			while (N > 0)
			{
				TmpHash ^= Resources[--N];
			}
			Hash = TmpHash;
			bComputedHash = true;
		}
		return Hash;
	}

	FRHIUniformBufferLayout() :
		ConstantBufferSize(0),
		ResourceOffset(0),
		Hash(0),
		bComputedHash(false)
	{
	}

private:
	mutable uint32 Hash;
	mutable bool bComputedHash;
};

/** Compare two uniform buffer layouts. */
inline bool operator==(const FRHIUniformBufferLayout& A, const FRHIUniformBufferLayout& B)
{
	return A.ConstantBufferSize == B.ConstantBufferSize
		&& A.ResourceOffset == B.ResourceOffset
		&& A.Resources == B.Resources;
}

class FRHIUniformBuffer : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHIUniformBuffer(const FRHIUniformBufferLayout& InLayout)
	: Layout(&InLayout)
	{}

	/** @return The number of bytes in the uniform buffer. */
	uint32 GetSize() const { return Layout->ConstantBufferSize; }
	const FRHIUniformBufferLayout& GetLayout() const { return *Layout; }

private:
	/** Layout of the uniform buffer. */
	const FRHIUniformBufferLayout* Layout;
};

class FRHIIndexBuffer : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHIIndexBuffer(uint32 InStride,uint32 InSize,uint32 InUsage)
	: Stride(InStride)
	, Size(InSize)
	, Usage(InUsage)
	{}

	/** @return The stride in bytes of the index buffer; must be 2 or 4. */
	uint32 GetStride() const { return Stride; }

	/** @return The number of bytes in the index buffer. */
	uint32 GetSize() const { return Size; }

	/** @return The usage flags used to create the index buffer. */
	uint32 GetUsage() const { return Usage; }

private:
	uint32 Stride;
	uint32 Size;
	uint32 Usage;
};

class FRHIVertexBuffer : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHIVertexBuffer(uint32 InSize,uint32 InUsage)
	: Size(InSize)
	, Usage(InUsage)
	{}

	/** @return The number of bytes in the vertex buffer. */
	uint32 GetSize() const { return Size; }

	/** @return The usage flags used to create the vertex buffer. */
	uint32 GetUsage() const { return Usage; }

private:
	uint32 Size;
	uint32 Usage;
};
class FRHIStructuredBuffer : public FRHIResource
{
public:

	/** Initialization constructor. */
	FRHIStructuredBuffer(uint32 InStride,uint32 InSize,uint32 InUsage)
	: Stride(InStride)
	, Size(InSize)
	, Usage(InUsage)
	{}

	/** @return The stride in bytes of the structured buffer; must be 2 or 4. */
	uint32 GetStride() const { return Stride; }

	/** @return The number of bytes in the structured buffer. */
	uint32 GetSize() const { return Size; }

	/** @return The usage flags used to create the structured buffer. */
	uint32 GetUsage() const { return Usage; }

private:
	uint32 Stride;
	uint32 Size;
	uint32 Usage;
};

//
// Textures
//

class RHI_API FRHITexture : public FRHIResource
{
public:
	
	/** Initialization constructor. */
	FRHITexture(uint32 InNumMips,uint32 InNumSamples,EPixelFormat InFormat,uint32 InFlags)
	: NumMips(InNumMips)
	, NumSamples(InNumSamples)
	, Format(InFormat)
	, Flags(InFlags)
	, LastCachedTime(-FLT_MAX)
	{}

	// Dynamic cast methods.
	virtual class FRHITexture2D* GetTexture2D() { return NULL; }
	virtual class FRHITexture2DArray* GetTexture2DArray() { return NULL; }
	virtual class FRHITexture3D* GetTexture3D() { return NULL; }
	virtual class FRHITextureCube* GetTextureCube() { return NULL; }
	virtual class FRHITextureReference* GetTextureReference() { return NULL; }
	
	/**
	 * Returns access to the platform-specific native resource pointer.  This is designed to be used to provide plugins with access
	 * to the underlying resource and should be used very carefully or not at all.
	 *
	 * @return	The pointer to the native resource or NULL if it not initialized or not supported for this resource type for some reason
	 */
	virtual void* GetNativeResource() const
	{
		// Override this in derived classes to expose access to the native texture resource
		return nullptr;
	}

	/** @return The number of mip-maps in the texture. */
	uint32 GetNumMips() const { return NumMips; }

	/** @return The format of the pixels in the texture. */
	EPixelFormat GetFormat() const { return Format; }

	/** @return The flags used to create the texture. */
	uint32 GetFlags() const { return Flags; }

	/* @return the number of samples for multi-sampling. */
	uint32 GetNumSamples() const { return NumSamples; }

	/** @return Whether the texture is multi sampled. */
	bool IsMultisampled() const { return NumSamples > 1; }		

	FRHIResourceInfo ResourceInfo;

	/** @return the last time this texture was cached in a resource table. */
	inline float GetLastCachedTime() const { return LastCachedTime; }

	/** sets the last time this texture was cached in a resource table. */
	void SetLastCachedTime(float InLastCachedTime)
	{
		LastCachedTime = InLastCachedTime;
	}

private:
	uint32 NumMips;
	uint32 NumSamples;
	EPixelFormat Format;
	uint32 Flags;
	float LastCachedTime;
};

class RHI_API FRHITexture2D : public FRHITexture
{
public:
	
	/** Initialization constructor. */
	FRHITexture2D(uint32 InSizeX,uint32 InSizeY,uint32 InNumMips,uint32 InNumSamples,EPixelFormat InFormat,uint32 InFlags)
	: FRHITexture(InNumMips,InNumSamples,InFormat,InFlags)
	, SizeX(InSizeX)
	, SizeY(InSizeY)
	{}
	
	// Dynamic cast methods.
	virtual FRHITexture2D* GetTexture2D() { return this; }

	/** @return The width of the texture. */
	uint32 GetSizeX() const { return SizeX; }
	
	/** @return The height of the texture. */
	uint32 GetSizeY() const { return SizeY; }

private:

	uint32 SizeX;
	uint32 SizeY;
};

class RHI_API FRHITexture2DArray : public FRHITexture
{
public:
	
	/** Initialization constructor. */
	FRHITexture2DArray(uint32 InSizeX,uint32 InSizeY,uint32 InSizeZ,uint32 InNumMips,EPixelFormat InFormat,uint32 InFlags)
	: FRHITexture(InNumMips,1,InFormat,InFlags)
	, SizeX(InSizeX)
	, SizeY(InSizeY)
	, SizeZ(InSizeZ)
	{}
	
	// Dynamic cast methods.
	virtual FRHITexture2DArray* GetTexture2DArray() { return this; }
	
	/** @return The width of the textures in the array. */
	uint32 GetSizeX() const { return SizeX; }
	
	/** @return The height of the texture in the array. */
	uint32 GetSizeY() const { return SizeY; }

	/** @return The number of textures in the array. */
	uint32 GetSizeZ() const { return SizeZ; }

private:

	uint32 SizeX;
	uint32 SizeY;
	uint32 SizeZ;
};

class RHI_API FRHITexture3D : public FRHITexture
{
public:
	
	/** Initialization constructor. */
	FRHITexture3D(uint32 InSizeX,uint32 InSizeY,uint32 InSizeZ,uint32 InNumMips,EPixelFormat InFormat,uint32 InFlags)
	: FRHITexture(InNumMips,1,InFormat,InFlags)
	, SizeX(InSizeX)
	, SizeY(InSizeY)
	, SizeZ(InSizeZ)
	{}
	
	// Dynamic cast methods.
	virtual FRHITexture3D* GetTexture3D() { return this; }
	
	/** @return The width of the texture. */
	uint32 GetSizeX() const { return SizeX; }
	
	/** @return The height of the texture. */
	uint32 GetSizeY() const { return SizeY; }

	/** @return The depth of the texture. */
	uint32 GetSizeZ() const { return SizeZ; }

private:

	uint32 SizeX;
	uint32 SizeY;
	uint32 SizeZ;
};

class RHI_API FRHITextureCube : public FRHITexture
{
public:
	
	/** Initialization constructor. */
	FRHITextureCube(uint32 InSize,uint32 InNumMips,EPixelFormat InFormat,uint32 InFlags)
	: FRHITexture(InNumMips,1,InFormat,InFlags)
	, Size(InSize)
	{}
	
	// Dynamic cast methods.
	virtual FRHITextureCube* GetTextureCube() { return this; }
	
	/** @return The width and height of each face of the cubemap. */
	uint32 GetSize() const { return Size; }

private:

	uint32 Size;
};

class RHI_API FRHITextureReference : public FRHITexture
{
public:
	explicit FRHITextureReference()
		: FRHITexture(0,0,PF_Unknown,0)
	{}

	virtual FRHITextureReference* GetTextureReference() override { return this; }
	inline FRHITexture* GetReferencedTexture() const { return ReferencedTexture.GetReference(); }

protected:
	void SetReferencedTexture(FRHITexture* InTexture)
	{
		ReferencedTexture = InTexture;
	}

private:
	TRefCountPtr<FRHITexture> ReferencedTexture;
};

class RHI_API FRHITextureReferenceNullImpl : public FRHITextureReference
{
public:
	FRHITextureReferenceNullImpl()
		: FRHITextureReference()
	{}

	void SetReferencedTexture(FRHITexture* InTexture)
	{
		FRHITextureReference::SetReferencedTexture(InTexture);
	}
};

//
// Misc
//

class FRHIRenderQuery : public FRHIResource {};
class FRHIViewport : public FRHIResource {};

//
// Views
//

class FRHIUnorderedAccessView : public FRHIResource {};
class FRHIShaderResourceView : public FRHIResource {};

// Declare RHI resource reference types.
#define DEFINE_RHI_REFERENCE_TYPE(Type,ParentType) \
	typedef FRHI##Type*              F##Type##RHIParamRef; \
	typedef TRefCountPtr<FRHI##Type> F##Type##RHIRef;
ENUM_RHI_RESOURCE_TYPES(DEFINE_RHI_REFERENCE_TYPE);

class FRHIRenderTargetView
{
public:
	FTextureRHIParamRef Texture;
	uint32 MipIndex;

	/** Array slice or texture cube face.  Only valid if texture resource was created with TexCreate_TargetArraySlicesIndependently! */
	uint32 ArraySliceIndex;

	FRHIRenderTargetView() : 
		Texture(NULL),
		MipIndex(0),
		ArraySliceIndex(-1)
	{}

	FRHIRenderTargetView(const FRHIRenderTargetView& Other) :
		Texture(Other.Texture),
		MipIndex(Other.MipIndex),
		ArraySliceIndex(Other.ArraySliceIndex)
	{}

	FRHIRenderTargetView(FTextureRHIParamRef InTexture) :
		Texture(InTexture),
		MipIndex(0),
		ArraySliceIndex(-1)
	{}

	FRHIRenderTargetView(FTextureRHIParamRef InTexture, uint32 InMipIndex, uint32 InArraySliceIndex) :
		Texture(InTexture),
		MipIndex(InMipIndex),
		ArraySliceIndex(InArraySliceIndex)
	{}
};

// Template magic to convert an FRHI*Shader to its enum
template<typename TRHIShader> struct TRHIShaderToEnum {};
template<> struct TRHIShaderToEnum<FRHIVertexShader>	{ enum { ShaderFrequency = SF_Vertex }; };
template<> struct TRHIShaderToEnum<FRHIHullShader>		{ enum { ShaderFrequency = SF_Hull }; };
template<> struct TRHIShaderToEnum<FRHIDomainShader>	{ enum { ShaderFrequency = SF_Domain }; };
template<> struct TRHIShaderToEnum<FRHIPixelShader>		{ enum { ShaderFrequency = SF_Pixel }; };
template<> struct TRHIShaderToEnum<FRHIGeometryShader>	{ enum { ShaderFrequency = SF_Geometry }; };
template<> struct TRHIShaderToEnum<FRHIComputeShader>	{ enum { ShaderFrequency = SF_Compute }; };
template<> struct TRHIShaderToEnum<FVertexShaderRHIParamRef>	{ enum { ShaderFrequency = SF_Vertex }; };
template<> struct TRHIShaderToEnum<FHullShaderRHIParamRef>		{ enum { ShaderFrequency = SF_Hull }; };
template<> struct TRHIShaderToEnum<FDomainShaderRHIParamRef>	{ enum { ShaderFrequency = SF_Domain }; };
template<> struct TRHIShaderToEnum<FPixelShaderRHIParamRef>		{ enum { ShaderFrequency = SF_Pixel }; };
template<> struct TRHIShaderToEnum<FGeometryShaderRHIParamRef>	{ enum { ShaderFrequency = SF_Geometry }; };
template<> struct TRHIShaderToEnum<FComputeShaderRHIParamRef>	{ enum { ShaderFrequency = SF_Compute }; };
template<> struct TRHIShaderToEnum<FVertexShaderRHIRef>		{ enum { ShaderFrequency = SF_Vertex }; };
template<> struct TRHIShaderToEnum<FHullShaderRHIRef>		{ enum { ShaderFrequency = SF_Hull }; };
template<> struct TRHIShaderToEnum<FDomainShaderRHIRef>		{ enum { ShaderFrequency = SF_Domain }; };
template<> struct TRHIShaderToEnum<FPixelShaderRHIRef>		{ enum { ShaderFrequency = SF_Pixel }; };
template<> struct TRHIShaderToEnum<FGeometryShaderRHIRef>	{ enum { ShaderFrequency = SF_Geometry }; };
template<> struct TRHIShaderToEnum<FComputeShaderRHIRef>	{ enum { ShaderFrequency = SF_Compute }; };
