// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "TextureCube.generated.h"

UCLASS(hidecategories=Object, MinimalAPI)
class UTextureCube : public UTexture
{
	GENERATED_UCLASS_BODY()

public:
	/** Platform data. */
	TScopedPointer<FTexturePlatformData> PlatformData;

	// Begin UObject interface.
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	// End UObject interface.

	/** Trivial accessors. */
	FORCEINLINE int32 GetSizeX() const
	{
		if (PlatformData)
		{
			return PlatformData->SizeX;
		}
		return 0;
	}
	FORCEINLINE int32 GetSizeY() const
	{
		if (PlatformData)
		{
			return PlatformData->SizeY;
		}
		return 0;
	}
	FORCEINLINE int32 GetNumMips() const
	{
		if (PlatformData)
		{
			return PlatformData->Mips.Num();
		}
		return 0;
	}
	FORCEINLINE EPixelFormat GetPixelFormat() const
	{
		if (PlatformData)
		{
			return PlatformData->PixelFormat;
		}
		return PF_Unknown;
	}

	// Begin UTexture interface
	virtual float GetSurfaceWidth() const OVERRIDE { return GetSizeX(); }
	virtual float GetSurfaceHeight() const OVERRIDE { return GetSizeY(); }
	virtual FTextureResource* CreateResource() OVERRIDE;
	virtual void UpdateResource() OVERRIDE;
	virtual EMaterialValueType GetMaterialType() OVERRIDE { return MCT_TextureCube; }
	virtual TScopedPointer<FTexturePlatformData>* GetPlatformDataLink() OVERRIDE { return &PlatformData; }
	// End UTexture interface
	
	/**
	 * Calculates the size of this texture in bytes if it had MipCount miplevels streamed in.
	 *
	 * @param	MipCount	Number of mips to calculate size for, counting from the smallest 1x1 mip-level and up.
	 * @return	Size of MipCount mips in bytes
	 */
	uint32 CalcTextureMemorySize( int32 MipCount ) const;

	/**
	 * Calculates the size of this texture if it had MipCount miplevels streamed in.
	 *
	 * @param	Enum	Which mips to calculate size for.
	 * @return	Total size of all specified mips, in bytes
	 */
	virtual uint32 CalcTextureMemorySizeEnum( ETextureMipCount Enum ) const;
};



