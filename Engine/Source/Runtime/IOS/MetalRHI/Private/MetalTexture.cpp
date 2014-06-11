// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalVertexBuffer.cpp: Metal texture RHI implementation.
=============================================================================*/

#include "MetalRHIPrivate.h"

FORCEINLINE int32 GetMetalCubeFace(ECubeFace Face)
{
	switch (Face)
	{
		case CubeFace_NegX:	return 0;
		case CubeFace_NegY:	return 1;
		case CubeFace_NegZ:	return 2;
		case CubeFace_PosX:	return 3;
		case CubeFace_PosY:	return 4;
		case CubeFace_PosZ:	return 5;
		default:			return -1;
	}
}

/** Texture reference class. */
class FMetalTextureReference : public FRHITextureReference
{
public:
	explicit FMetalTextureReference()
		: FRHITextureReference()
	{}

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

	void SetReferencedTexture(FRHITexture* InTexture)
	{
		FRHITextureReference::SetReferencedTexture(InTexture);
	}
};

/** Given a pointer to a RHI texture that was created by the Metal RHI, returns a pointer to the FMetalTextureBase it encapsulates. */
FMetalSurface& GetMetalSurfaceFromRHITexture(FRHITexture* Texture)
{
	if(Texture->GetTexture2D())
	{
		return ((FMetalTexture2D*)Texture)->Surface;
	}
	else if(Texture->GetTexture2DArray())
	{
		return ((FMetalTexture2DArray*)Texture)->Surface;
	}
	else if(Texture->GetTexture3D())
	{
		return ((FMetalTexture3D*)Texture)->Surface;
	}
	else if(Texture->GetTextureCube())
	{
		return ((FMetalTextureCube*)Texture)->Surface;
	}
	else if(Texture->GetTextureReference())
	{
		return GetMetalSurfaceFromRHITexture(static_cast<FMetalTextureReference*>(Texture)->GetReferencedTexture());
	}
	else
	{
		UE_LOG(LogMetal, Fatal, TEXT("Unknown RHI texture type"));
		return ((FMetalTexture2D*)Texture)->Surface;
	}
}

FMetalSurface::FMetalSurface(ERHIResourceType ResourceType, EPixelFormat Format, uint32 InSizeX, uint32 InSizeY, uint32 InSizeZ, uint32 NumSamples, bool bArray, uint32 ArraySize, uint32 NumMips, uint32 InFlags, FResourceBulkDataInterface* BulkData)
	: PixelFormat(Format)
    , MSAATexture(nil)
	, SizeX(InSizeX)
	, SizeY(InSizeY)
	, SizeZ(InSizeZ)
	, bIsCubemap(false)
	, Flags(InFlags)
{
	// the special back buffer surface will be updated in FMetalManager::BeginFrame - no need to set the texture here
	if (Flags & TexCreate_Presentable)
	{
		return;
	}
	
	MTLPixelFormat MTLFormat = (MTLPixelFormat)GPixelFormats[Format].PlatformFormat;
	MTLTextureDescriptor* Desc;
	
	if (ResourceType == RRT_TextureCube)
	{
		Desc = [MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLFormat
																	size:SizeX
															   mipmapped:(NumMips > 1)];
		bIsCubemap = true;
	}
	else
	{
		Desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLFormat
																 width:SizeX
																height:SizeY
															 mipmapped:(NumMips > 1)];
		Desc.depth = SizeZ;
	}

	// flesh out the descriptor
	if (bArray)
	{
		Desc.arrayLength = ArraySize;
	}
	Desc.mipmapLevelCount = NumMips;
	
	
	Texture = [FMetalManager::GetDevice() newTextureWithDescriptor:Desc];
	if (Texture == nil)
	{
		NSLog(@"Failed to create texture, desc  %@", Desc);
	}
	TRACK_OBJECT(Texture);
	
	// upload existing bulkdata
	if (BulkData)
	{
		UE_LOG(LogIOS, Display, TEXT("Got a bulk data texture, with %d mips"), NumMips);
		// copy over bulk data
		// Lock
		// Memcpy
		// Unlock

		// bulk data can be unloaded now
		BulkData->Discard();
	}
	
	LockedMemory = NULL;

	if (!FParse::Param(FCommandLine::Get(), TEXT("nomsaa")))
	{
		if (NumSamples > 1)
		{
			check(Flags & (TexCreate_RenderTargetable | TexCreate_DepthStencilTargetable));
			Desc.textureType = MTLTextureType2DMultisample;
	
			// allow commandline to override
			FParse::Value(FCommandLine::Get(), TEXT("msaa="), NumSamples);
			Desc.sampleCount = NumSamples;

			MSAATexture = [FMetalManager::GetDevice() newTextureWithDescriptor:Desc];
			if (Format == PF_DepthStencil)
			{
				[Texture release];
				Texture = MSAATexture;
			}
        
			NSLog(@"Creating %dx MSAA %d x %d %s surface", (int32)Desc.sampleCount, SizeX, SizeY, (Flags & TexCreate_RenderTargetable) ? "Color" : "Depth");
			if (MSAATexture == nil)
			{
				NSLog(@"Failed to create texture, desc  %@", Desc);
			}
			TRACK_OBJECT(MSAATexture);
		}
	}
}


FMetalSurface::~FMetalSurface()
{
    if (MSAATexture != nil)
    {
		FMetalManager::ReleaseObject(MSAATexture);
    }
    
	if (!(Flags & TexCreate_Presentable))
	{
		FMetalManager::ReleaseObject(Texture);
	}
}


void* FMetalSurface::Lock(uint32 MipIndex, uint32 ArrayIndex, EResourceLockMode LockMode, uint32& DestStride)
{
	// Calculate the dimensions of the mip-map.
	const uint32 BlockSizeX = GPixelFormats[PixelFormat].BlockSizeX;
	const uint32 BlockSizeY = GPixelFormats[PixelFormat].BlockSizeY;
	const uint32 BlockBytes = GPixelFormats[PixelFormat].BlockBytes;
	const uint32 MipSizeX = FMath::Max(SizeX >> MipIndex,BlockSizeX);
	const uint32 MipSizeY = FMath::Max(SizeY >> MipIndex,BlockSizeY);
	uint32 NumBlocksX = (MipSizeX + BlockSizeX - 1) / BlockSizeX;
	uint32 NumBlocksY = (MipSizeY + BlockSizeY - 1) / BlockSizeY;
	if ( PixelFormat == PF_PVRTC2 || PixelFormat == PF_PVRTC4 )
	{
		// PVRTC has minimum 2 blocks width and height
		NumBlocksX = FMath::Max<uint32>(NumBlocksX, 2);
		NumBlocksY = FMath::Max<uint32>(NumBlocksY, 2);
	}
	const uint32 MipBytes = NumBlocksX * NumBlocksY * BlockBytes;
	DestStride = NumBlocksX * BlockBytes;
	
	// allocate some temporary memory
	check(LockedMemory == NULL);
	LockedMemory = FMemory::Malloc(MipBytes);
	if (LockMode != RLM_WriteOnly)
	{
		// [Texture readPixels ...];
	}

	return LockedMemory;
}

void FMetalSurface::Unlock(uint32 MipIndex, uint32 ArrayIndex)
{
	const uint32 BlockSizeX = GPixelFormats[PixelFormat].BlockSizeX;
	const uint32 BlockSizeY = GPixelFormats[PixelFormat].BlockSizeY;
	const uint32 BlockBytes = GPixelFormats[PixelFormat].BlockBytes;
	const uint32 MipSizeX = FMath::Max(SizeX >> MipIndex,BlockSizeX);
	const uint32 MipSizeY = FMath::Max(SizeY >> MipIndex,BlockSizeY);
	uint32 NumBlocksX = (MipSizeX + BlockSizeX - 1) / BlockSizeX;
	uint32 NumBlocksY = (MipSizeY + BlockSizeY - 1) / BlockSizeY;
	uint32 MipBytes = NumBlocksX * NumBlocksY * BlockBytes;
	uint32 Stride = NumBlocksX * BlockBytes;
	if (PixelFormat == PF_PVRTC2 || PixelFormat == PF_PVRTC4)
	{
		MipBytes = 0;
		Stride = 0;
	}

	// upload the texture to the texture slice
	// @todo urban: The MipSizeZ would be needed for a volume texture - do we care?
	[Texture copyFromPixels:LockedMemory rowBytes:Stride imageBytes:MipBytes toSlice:ArrayIndex mipmapLevel:MipIndex origin:MTLOriginMake(0, 0, 0) size:MTLSizeMake(FMath::Max<uint32>(SizeX>>MipIndex, 1), FMath::Max<uint32>(SizeY>>MipIndex, 1), 1)];
	
	FMemory::Free(LockedMemory);
	LockedMemory = NULL;
}

uint32 FMetalSurface::GetMemorySize()
{
	return 0;
}

/*-----------------------------------------------------------------------------
	Texture allocator support.
-----------------------------------------------------------------------------*/

void FMetalDynamicRHI::RHIGetTextureMemoryStats(FTextureMemoryStats& OutStats)
{

}

bool FMetalDynamicRHI::RHIGetTextureMemoryVisualizeData( FColor* /*TextureData*/, int32 /*SizeX*/, int32 /*SizeY*/, int32 /*Pitch*/, int32 /*PixelSize*/ )
{
	return false;
}

uint32 FMetalDynamicRHI::RHIComputeMemorySize(FTextureRHIParamRef TextureRHI)
{
	if(!TextureRHI)
	{
		return 0;
	}

	return GetMetalSurfaceFromRHITexture(TextureRHI).GetMemorySize();
}

/*-----------------------------------------------------------------------------
	2D texture support.
-----------------------------------------------------------------------------*/

FTexture2DRHIRef FMetalDynamicRHI::RHICreateTexture2D(uint32 SizeX, uint32 SizeY, uint8 Format, uint32 NumMips, uint32 NumSamples, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return new FMetalTexture2D((EPixelFormat)Format, SizeX, SizeY, NumMips, NumSamples, Flags, CreateInfo.BulkData);
}

FTexture2DRHIRef FMetalDynamicRHI::RHIAsyncCreateTexture2D(uint32 SizeX,uint32 SizeY,uint8 Format,uint32 NumMips,uint32 Flags,void** InitialMipData,uint32 NumInitialMips)
{
	UE_LOG(LogMetal, Fatal, TEXT("RHIAsyncCreateTexture2D is not supported"));
	return FTexture2DRHIRef();
}

void FMetalDynamicRHI::RHICopySharedMips(FTexture2DRHIParamRef DestTexture2D,FTexture2DRHIParamRef SrcTexture2D)
{

}

FTexture2DArrayRHIRef FMetalDynamicRHI::RHICreateTexture2DArray(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return new FMetalTexture2DArray((EPixelFormat)Format, SizeX, SizeY, SizeZ, NumMips, Flags, CreateInfo.BulkData);
}

FTexture3DRHIRef FMetalDynamicRHI::RHICreateTexture3D(uint32 SizeX, uint32 SizeY, uint32 SizeZ, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return new FMetalTexture3D((EPixelFormat)Format, SizeX, SizeY, SizeZ, NumMips, Flags, CreateInfo.BulkData);
}

void FMetalDynamicRHI::RHIGetResourceInfo(FTextureRHIParamRef Ref, FRHIResourceInfo& OutInfo)
{
}

void FMetalDynamicRHI::RHIGenerateMips(FTextureRHIParamRef SourceSurfaceRHI)
{

}

FTexture2DRHIRef FMetalDynamicRHI::RHIAsyncReallocateTexture2D(FTexture2DRHIParamRef OldTextureRHI, int32 NewMipCount, int32 NewSizeX, int32 NewSizeY, FThreadSafeCounter* RequestStatus)
{
	DYNAMIC_CAST_METGALRESOURCE(Texture2D,OldTexture);

	return NULL;
}

ETextureReallocationStatus FMetalDynamicRHI::RHIFinalizeAsyncReallocateTexture2D( FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted )
{
	return TexRealloc_Failed;
}

ETextureReallocationStatus FMetalDynamicRHI::RHICancelAsyncReallocateTexture2D( FTexture2DRHIParamRef Texture2D, bool bBlockUntilCompleted )
{
	return TexRealloc_Failed;
}


void* FMetalDynamicRHI::RHILockTexture2D(FTexture2DRHIParamRef TextureRHI,uint32 MipIndex,EResourceLockMode LockMode,uint32& DestStride,bool bLockWithinMiptail)
{
	DYNAMIC_CAST_METGALRESOURCE(Texture2D,Texture);
	return Texture->Surface.Lock(MipIndex, 0, LockMode, DestStride);
}

void FMetalDynamicRHI::RHIUnlockTexture2D(FTexture2DRHIParamRef TextureRHI,uint32 MipIndex,bool bLockWithinMiptail)
{
	DYNAMIC_CAST_METGALRESOURCE(Texture2D,Texture);
	Texture->Surface.Unlock(MipIndex, 0);
}

void* FMetalDynamicRHI::RHILockTexture2DArray(FTexture2DArrayRHIParamRef TextureRHI,uint32 TextureIndex,uint32 MipIndex,EResourceLockMode LockMode,uint32& DestStride,bool bLockWithinMiptail)
{
	DYNAMIC_CAST_METGALRESOURCE(Texture2DArray,Texture);
	return Texture->Surface.Lock(MipIndex, TextureIndex, LockMode, DestStride);
}

void FMetalDynamicRHI::RHIUnlockTexture2DArray(FTexture2DArrayRHIParamRef TextureRHI,uint32 TextureIndex,uint32 MipIndex,bool bLockWithinMiptail)
{
	DYNAMIC_CAST_METGALRESOURCE(Texture2DArray,Texture);
	Texture->Surface.Unlock(MipIndex, TextureIndex);
}

void FMetalDynamicRHI::RHIUpdateTexture2D(FTexture2DRHIParamRef TextureRHI, uint32 MipIndex, const struct FUpdateTextureRegion2D& UpdateRegion, uint32 SourcePitch, const uint8* SourceData)
{	
	DYNAMIC_CAST_METGALRESOURCE(Texture3D,Texture);

}

void FMetalDynamicRHI::RHIUpdateTexture3D(FTexture3DRHIParamRef TextureRHI,uint32 MipIndex,const FUpdateTextureRegion3D& UpdateRegion,uint32 SourceRowPitch,uint32 SourceDepthPitch,const uint8* SourceData)
{	
	DYNAMIC_CAST_METGALRESOURCE(Texture3D,Texture);

}


/*-----------------------------------------------------------------------------
	Cubemap texture support.
-----------------------------------------------------------------------------*/
FTextureCubeRHIRef FMetalDynamicRHI::RHICreateTextureCube(uint32 Size, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return new FMetalTextureCube((EPixelFormat)Format, Size, false, 1, NumMips, Flags, CreateInfo.BulkData);
}

FTextureCubeRHIRef FMetalDynamicRHI::RHICreateTextureCubeArray(uint32 Size, uint32 ArraySize, uint8 Format, uint32 NumMips, uint32 Flags, FRHIResourceCreateInfo& CreateInfo)
{
	return new FMetalTextureCube((EPixelFormat)Format, Size, true, ArraySize, NumMips, Flags, CreateInfo.BulkData);
}

void* FMetalDynamicRHI::RHILockTextureCubeFace(FTextureCubeRHIParamRef TextureCubeRHI,uint32 FaceIndex,uint32 ArrayIndex,uint32 MipIndex,EResourceLockMode LockMode,uint32& DestStride,bool bLockWithinMiptail)
{
	DYNAMIC_CAST_METGALRESOURCE(TextureCube,TextureCube);
	uint32 MetalFace = GetMetalCubeFace((ECubeFace)FaceIndex);
	return TextureCube->Surface.Lock(MipIndex, FaceIndex + 6 * ArrayIndex, LockMode, DestStride);
}

void FMetalDynamicRHI::RHIUnlockTextureCubeFace(FTextureCubeRHIParamRef TextureCubeRHI,uint32 FaceIndex,uint32 ArrayIndex,uint32 MipIndex,bool bLockWithinMiptail)
{
	DYNAMIC_CAST_METGALRESOURCE(TextureCube,TextureCube);
	uint32 MetalFace = GetMetalCubeFace((ECubeFace)FaceIndex);
	TextureCube->Surface.Unlock(MipIndex, FaceIndex + ArrayIndex * 6);
}




FTextureReferenceRHIRef FMetalDynamicRHI::RHICreateTextureReference()
{
	return new FMetalTextureReference();
}

void FMetalDynamicRHI::RHIUpdateTextureReference(FTextureReferenceRHIParamRef TextureRefRHI, FTextureRHIParamRef NewTextureRHI)
{
	FMetalTextureReference* TextureRef = (FMetalTextureReference*)TextureRefRHI;
	if (TextureRef)
	{
		TextureRef->SetReferencedTexture(NewTextureRHI);
	}
}


void FMetalDynamicRHI::RHIBindDebugLabelName(FTextureRHIParamRef TextureRHI, const TCHAR* Name)
{
}

void FMetalDynamicRHI::RHIVirtualTextureSetFirstMipInMemory(FTexture2DRHIParamRef TextureRHI, uint32 FirstMip)
{
	NOT_SUPPORTED("RHIVirtualTextureSetFirstMipInMemory");
}

void FMetalDynamicRHI::RHIVirtualTextureSetFirstMipVisible(FTexture2DRHIParamRef TextureRHI, uint32 FirstMip)
{
	NOT_SUPPORTED("RHIVirtualTextureSetFirstMipVisible");
}
