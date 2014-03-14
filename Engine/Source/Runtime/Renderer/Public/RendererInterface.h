// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	RendererInterface.h: Renderer interface definition.
=============================================================================*/

#ifndef __RendererInterface_H__
#define __RendererInterface_H__

#include "ModuleInterface.h"
#include "ModuleManager.h"
#include "RenderingThread.h"
#include "RenderUtils.h"

// Forward declarations.
class FPrimitiveSceneProxy;
class FPrimitiveSceneInfo;
class FSceneViewFamily;
class FCanvas;
class UWorld;
class FSceneInterface;
class FMaterial;

// Shortcut for the allocator used by scene rendering.
class SceneRenderingAllocator
	: public TMemStackAllocator<>
{
};

/** All necessary data to create a render target from the pooled render targets. */
struct FPooledRenderTargetDesc
{
public:

	/** Default constructor, use one of the factory functions below to make a valid description */
	FPooledRenderTargetDesc()
		: Extent(0, 0)
		, Depth(0)
		, ArraySize(1)
		, bIsArray(false)
		, NumMips(0)
		, NumSamples(1)
		, Format(PF_Unknown)
		, Flags(TexCreate_None)
		, TargetableFlags(TexCreate_None)
		, bForceSeparateTargetAndShaderResource(false)
		, DebugName(TEXT("UnknownTexture"))
	{
		check(!IsValid());
	}

	/**
	 * Factory function to create 2D texture description
	 * @param InFlags bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV
	 */
	static FPooledRenderTargetDesc Create2DDesc(
		FIntPoint InExtent,
		EPixelFormat InFormat,
		uint32 InFlags,
		uint32 InTargetableFlags,
		bool bInForceSeparateTargetAndShaderResource,
		uint16 InNumMips = 1)
	{
		check(InExtent.X);
		check(InExtent.Y);

		FPooledRenderTargetDesc NewDesc;
		NewDesc.Extent = InExtent;
		NewDesc.Depth = 0;
		NewDesc.ArraySize = 1;
		NewDesc.bIsArray = false;
		NewDesc.NumMips = InNumMips;
		NewDesc.NumSamples = 1;
		NewDesc.Format = InFormat;
		NewDesc.Flags = InFlags;
		NewDesc.TargetableFlags = InTargetableFlags;
		NewDesc.bForceSeparateTargetAndShaderResource = bInForceSeparateTargetAndShaderResource;
		NewDesc.DebugName = TEXT("UnknownTexture2D");
		check(NewDesc.Is2DTexture());
		return NewDesc;
	}

	/**
	 * Factory function to create 3D texture description
	 * @param InFlags bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV
	 */
	static FPooledRenderTargetDesc CreateVolumeDesc(
		uint32 InSizeX,
		uint32 InSizeY,
		uint32 InSizeZ,
		EPixelFormat InFormat,
		uint32 InFlags,
		uint32 InTargetableFlags,
		bool bInForceSeparateTargetAndShaderResource,
		uint16 InNumMips = 1)
	{
		check(InSizeX);
		check(InSizeY);

		FPooledRenderTargetDesc NewDesc;
		NewDesc.Extent = FIntPoint(InSizeX,InSizeY);
		NewDesc.Depth = InSizeZ;
		NewDesc.ArraySize = 1;
		NewDesc.bIsArray = false;
		NewDesc.NumMips = InNumMips;
		NewDesc.NumSamples = 1;
		NewDesc.Format = InFormat;
		NewDesc.Flags = InFlags;
		NewDesc.TargetableFlags = InTargetableFlags;
		NewDesc.bForceSeparateTargetAndShaderResource = bInForceSeparateTargetAndShaderResource;
		NewDesc.DebugName = TEXT("UnknownTextureVolume");
		check(NewDesc.Is3DTexture());
		return NewDesc;
	}

	/**
	 * Factory function to create cube map texture description
	 * @param InFlags bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV
	 */
	static FPooledRenderTargetDesc CreateCubemapDesc(
		uint32 InExtent,
		EPixelFormat InFormat,
		uint32 InFlags,
		uint32 InTargetableFlags,
		bool bInForceSeparateTargetAndShaderResource,
		uint32 InArraySize = 1,
		uint16 InNumMips = 1)
	{
		check(InExtent);

		FPooledRenderTargetDesc NewDesc;
		NewDesc.Extent = FIntPoint(InExtent, 0);
		NewDesc.Depth = 0;
		NewDesc.ArraySize = InArraySize;
		// Note: this doesn't allow an array of size 1
		NewDesc.bIsArray = InArraySize > 1;
		NewDesc.NumMips = InNumMips;
		NewDesc.NumSamples = 1;
		NewDesc.Format = InFormat;
		NewDesc.Flags = InFlags;
		NewDesc.TargetableFlags = InTargetableFlags;
		NewDesc.bForceSeparateTargetAndShaderResource = bInForceSeparateTargetAndShaderResource;
		NewDesc.DebugName = TEXT("UnknownTextureCube");
		check(NewDesc.IsCubemap());

		return NewDesc;
	}

	/** Comparison operator to test if a render target can be reused */
	bool operator==(const FPooledRenderTargetDesc& rhs) const
	{
		return Extent == rhs.Extent
			&& Depth == rhs.Depth
			&& bIsArray == rhs.bIsArray
			&& ArraySize == rhs.ArraySize
			&& NumMips == rhs.NumMips
			&& NumSamples == rhs.NumSamples
			&& Format == rhs.Format
			&& Flags == rhs.Flags
			&& TargetableFlags == rhs.TargetableFlags
			&& bForceSeparateTargetAndShaderResource == rhs.bForceSeparateTargetAndShaderResource;
	}

	bool IsCubemap() const
	{
		return Extent.X != 0 && Extent.Y == 0 && Depth == 0;
	}

	bool Is2DTexture() const
	{
		return Extent.X != 0 && Extent.Y != 0 && Depth == 0;
	}

	bool Is3DTexture() const
	{
		return Extent.X != 0 && Extent.Y != 0 && Depth != 0;
	}

	// @return true if this texture is a texture array
	bool IsArray() const
	{
		return bIsArray;
	}

	bool IsValid() const
	{
		if(NumSamples != 1)
		{
			if(NumSamples < 1 || NumSamples > 8)
			{
				// D3D11 limitations
				return false;
			}

			if(!Is2DTexture())
			{
				return false;
			}
		}

		return Extent.X != 0 && NumMips != 0 && NumSamples >=1 && NumSamples <=16 && Format != PF_Unknown
			&& ((TargetableFlags & TexCreate_UAV) == 0 || GRHIFeatureLevel == ERHIFeatureLevel::SM5);
	}

	/** 
	 * for debugging purpose
	 * @return e.g. (2D 128x64 PF_R8)
	 */
	FString GenerateInfoString() const
	{
		const TCHAR* FormatString = GetPixelFormatString(Format);

		FString FlagsString = TEXT("");

		uint32 LocalFlags = Flags | TargetableFlags;

		if(LocalFlags & TexCreate_RenderTargetable)
		{
			FlagsString += TEXT(" RT");
		}
		if(LocalFlags & TexCreate_SRGB)
		{
			FlagsString += TEXT(" sRGB");
		}
		if(NumSamples > 1)
		{
			FlagsString += FString::Printf(TEXT(" %dxMSAA"), NumSamples);
		}
		if(LocalFlags & TexCreate_UAV)
		{
			FlagsString += TEXT(" UAV");
		}

		FString ArrayString;

		if(IsArray())
		{
			ArrayString = FString::Printf(TEXT("[%d]"), ArraySize);
		}

		if(Is2DTexture())
		{
			return FString::Printf(TEXT("(2D%s %dx%d %s%s)"), *ArrayString, Extent.X, Extent.Y, FormatString, *FlagsString);
		}
		else if(Is3DTexture())
		{
			return FString::Printf(TEXT("(3D%s %dx%dx%d %s%s)"), *ArrayString, Extent.X, Extent.Y, Depth, FormatString, *FlagsString);
		}
		else if(IsCubemap())
		{
			return FString::Printf(TEXT("(Cube%s %d %s%s)"), *ArrayString, Extent.X, FormatString, *FlagsString);
		}
		else
		{
			return TEXT("(INVALID)");
		}
	}

	// useful when compositing graph takes an input as output format
	void Reset()
	{
		// Usually we don't want to propagate MSAA samples.
		NumSamples = 1;
	}

	/** In pixels, (0,0) if not set, (x,0) for cube maps, todo: make 3d int vector for volume textures */
	FIntPoint Extent;
	/** 0, unless it's texture array or volume texture */
	uint32 Depth;
	
	/** >1 if a texture array should be used (not supported on DX9) */
	uint32 ArraySize;
	/** true if an array texture. Note that ArraySize still can be 1 */
	bool bIsArray;

	/** Number of mips */
	uint16 NumMips;
	/** Number of MSAA samples, default: 1  */
	uint16 NumSamples;
	/** Texture format e.g. PF_B8G8R8A8 */
	EPixelFormat Format;
	/** The flags that must be set on both the shader-resource and the targetable texture. bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV */
	uint32 Flags;
	/** The flags that must be set on the targetable texture. bit mask combined from elements of ETextureCreateFlags e.g. TexCreate_UAV */
	uint32 TargetableFlags;
	/** Whether the shader-resource and targetable texture must be separate textures. */
	bool bForceSeparateTargetAndShaderResource;
	/** only set a pointer to memory that never gets released */
	const TCHAR *DebugName;
};


/**
 * Single render target item consists of a render surface and its resolve texture, Render thread side
 */
struct FSceneRenderTargetItem
{
	/** default constructor */
	FSceneRenderTargetItem() {}

	/** constructor */
	FSceneRenderTargetItem(FTextureRHIParamRef InTargetableTexture, FTextureRHIParamRef InShaderResourceTexture, FUnorderedAccessViewRHIRef InUAV)
		:	TargetableTexture(InTargetableTexture)
		,	ShaderResourceTexture(InShaderResourceTexture)
		,	UAV(InUAV)
	{}

	/** */
	void SafeRelease()
	{
		TargetableTexture.SafeRelease();
		ShaderResourceTexture.SafeRelease();
		UAV.SafeRelease();
	}

	bool IsValid() const
	{
		return TargetableTexture != 0
			|| ShaderResourceTexture != 0
			|| UAV != 0;
	}

	/** The 2D or cubemap texture that may be used as a render or depth-stencil target. */
	FTextureRHIRef TargetableTexture;
	/** The 2D or cubemap shader-resource 2D texture that the targetable textures may be resolved to. */
	FTextureRHIRef ShaderResourceTexture;
	/** only created if requested through the flag  */
	FUnorderedAccessViewRHIRef UAV;
};

/**
 * Render thread side, use TRefCountPtr<IPooledRenderTarget>, allows to use sharing and VisualizeTexture
 */
struct IPooledRenderTarget : public IRefCountedObject
{
	/** Checks if the reference count indicated that the rendertarget is unused and can be reused. */
	virtual bool IsFree() const = 0;
	/** Get all the data that is needed to create the render target. */
	virtual const FPooledRenderTargetDesc& GetDesc() const = 0;
	/** @param InName must not be 0 */
	virtual void SetDebugName(const TCHAR *InName) = 0;
	/** Only for debugging purpose */
	virtual uint32 ComputeMemorySize() const = 0;
	/** Get the low level internals (texture/surface) */
	inline FSceneRenderTargetItem& GetRenderTargetItem() { return RenderTargetItem; }
	/** Get the low level internals (texture/surface) */
	inline const FSceneRenderTargetItem& GetRenderTargetItem() const { return RenderTargetItem; }

protected:

	/** The internal references to the created render target */
	FSceneRenderTargetItem RenderTargetItem;
};


struct FQueryVisualizeTexureInfo
{
	TArray<FString> Entries;
};

/**
 * The public interface of the renderer module.
 */
class IRendererModule : public IModuleInterface
{
public:

	/** Call from the game thread to send a message to the rendering thread to being rendering this view family. */
	virtual void BeginRenderingViewFamily(FCanvas* Canvas,const FSceneViewFamily* ViewFamily) = 0;
	
	/**
	 * Allocates a new instance of the private FScene implementation for the given world.
	 * @param World - An optional world to associate with the scene.
	 * @param bInRequiresHitProxies - Indicates that hit proxies should be rendered in the scene.
	 */
	virtual FSceneInterface* AllocateScene(UWorld* World, bool bInRequiresHitProxies) = 0;
	
	virtual void RemoveScene(FSceneInterface* Scene) = 0;

	/**
	 * Updates static draw lists for the given set of materials for each allocated scene.
	 */
	virtual void UpdateStaticDrawListsForMaterials(const TArray<const FMaterial*>& Materials) = 0;

	/** Allocates a new instance of the private scene manager implementation of FSceneViewStateInterface */
	virtual class FSceneViewStateInterface* AllocateViewState() = 0;

	/** @return The number of lights that affect a primitive. */
	virtual uint32 GetNumDynamicLightsAffectingPrimitive(const class FPrimitiveSceneInfo* PrimitiveSceneInfo,const class FLightCacheInterface* LCI) = 0;

	/** Forces reallocation of scene render targets. */
	virtual void ReallocateSceneRenderTargets() = 0;

	/** Sets the buffer size of the render targets. */
	virtual void SceneRenderTargetsSetBufferSize(uint32 SizeX, uint32 SizeY) = 0;

	/** Draws a tile mesh element with the specified view. */
	virtual void DrawTileMesh(const FSceneView& View, const FMeshBatch& Mesh, bool bIsHitTesting, const class FHitProxyId& HitProxyId) = 0;

	/** Render thread side, use TRefCountPtr<IPooledRenderTarget>, allows to use sharing and VisualizeTexture */
	virtual void RenderTargetPoolFindFreeElement(const FPooledRenderTargetDesc& Desc, TRefCountPtr<IPooledRenderTarget> &Out, const TCHAR* InDebugName) = 0;
	
	/** Render thread side, to age the pool elements so they get released at some point */
	virtual void TickRenderTargetPool() = 0;

	virtual const TSet<FSceneInterface*>& GetAllocatedScenes() = 0;

	/** Renderer gets a chance to log some useful crash data */
	virtual void DebugLogOnCrash() = 0;

	/**  */
	// @param WorkScale >0, 10 for normal precision and runtime of less than a second
	// @param bDebugOut has no effect in shipping
	virtual void GPUBenchmark(FSynthBenchmarkResults& InOut, uint32 WorkScale = 10, bool bDebugOut = false) = 0;

	virtual void QueryVisualizeTexture(FQueryVisualizeTexureInfo& Out) = 0;
	virtual void ExecVisualizeTextureCmd(const FString& Cmd) = 0;

	virtual void UpdateMapNeedsLightingFullyRebuiltState(UWorld* World) = 0;
};



#endif
