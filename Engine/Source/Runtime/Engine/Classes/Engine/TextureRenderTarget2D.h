// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "TextureRenderTarget2D.generated.h"

/**
 * TextureRenderTarget2D
 *
 * 2D render target texture resource. This can be used as a target
 * for rendering as well as rendered as a regular 2D texture resource.
 *
 */
UCLASS(hidecategories=Object, hidecategories=Texture, MinimalAPI)
class UTextureRenderTarget2D : public UTextureRenderTarget
{
	GENERATED_UCLASS_BODY()

	/** The width of the texture.												*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=TextureRenderTarget2D, AssetRegistrySearchable)
	int32 SizeX;

	/** The height of the texture.												*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=TextureRenderTarget2D, AssetRegistrySearchable)
	int32 SizeY;

	/** the color the texture is cleared to */
	UPROPERTY()
	FLinearColor ClearColor;

	/** The addressing mode to use for the X axis.								*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureRenderTarget2D, AssetRegistrySearchable)
	TEnumAsByte<enum TextureAddress> AddressX;

	/** The addressing mode to use for the Y axis.								*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=TextureRenderTarget2D, AssetRegistrySearchable)
	TEnumAsByte<enum TextureAddress> AddressY;

	/** True to force linear gamma space for this render target */
	UPROPERTY()
	uint32 bForceLinearGamma:1;

	/** Whether to support storing HDR values, which requires more memory. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=TextureRenderTarget2D, AssetRegistrySearchable)
	uint32 bHDR:1;

	/** Normally the format is derived from bHDR, this allows code to set the format explicitly. */
	UPROPERTY()
	TEnumAsByte<enum EPixelFormat> OverrideFormat;

	/**
	 * Initialize the settings needed to create a render target texture
	 * and create its resource
	 * @param	InSizeX - width of the texture
	 * @param	InSizeY - height of the texture
	 * @param	InFormat - format of the texture
	 * @param	bInForceLinearGame - forces render target to use linear gamma space
	 */
	ENGINE_API void InitCustomFormat(uint32 InSizeX, uint32 InSizeY, EPixelFormat InOverrideFormat, bool bInForceLinearGamma);

	/** Initializes the render target, the format will be derived from the value of bHDR. */
	ENGINE_API void InitAutoFormat(uint32 InSizeX, uint32 InSizeY);

	/**
	 * Utility for creating a new UTexture2D from a TextureRenderTarget2D
	 * TextureRenderTarget2D must be square and a power of two size.
	 * @param Outer - Outer to use when constructing the new Texture2D.
	 * @param NewTexName - Name of new UTexture2D object.
	 * @param ObjectFlags - Flags to apply to the new Texture2D object
	 * @param Flags - Various control flags for operation (see EConstructTextureFlags)
	 * @param AlphaOverride - If specified, the values here will become the alpha values in the resulting texture
	 * @return New UTexture2D object.
	 */
	ENGINE_API UTexture2D* ConstructTexture2D(UObject* Outer, const FString& NewTexName, EObjectFlags ObjectFlags, uint32 Flags=CTF_Default, TArray<uint8>* AlphaOverride=NULL);

	ENGINE_API void UpdateResourceImmediate();

	// Begin UTexture interface.
	virtual float GetSurfaceWidth() const OVERRIDE { return SizeX; }
	virtual float GetSurfaceHeight() const OVERRIDE { return SizeY; }
	virtual FTextureResource* CreateResource() OVERRIDE;
	virtual EMaterialValueType GetMaterialType() OVERRIDE;
	// End UTexture interface.

	// Begin UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostLoad() OVERRIDE;
	virtual SIZE_T GetResourceSize(EResourceSizeMode::Type Mode) OVERRIDE;
	virtual FString GetDesc() OVERRIDE;
	// End UObject interface

	FORCEINLINE int32 GetNumMips() const
	{
		return 1;
	}

	FORCEINLINE EPixelFormat GetFormat() const
	{
		if (OverrideFormat == PF_Unknown)
		{
			return bHDR ? PF_FloatRGBA : PF_B8G8R8A8;
		}
		else
		{
			return OverrideFormat;
		}
	}
};



