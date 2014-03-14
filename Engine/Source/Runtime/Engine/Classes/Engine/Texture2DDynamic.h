// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Texture2DDynamic.generated.h"

UCLASS(hidecategories=Object)
class UTexture2DDynamic : public UTexture
{
	GENERATED_UCLASS_BODY()

	/** The width of the texture. */
	int32 SizeX;

	/** The height of the texture. */
	int32 SizeY;

	/** The format of the texture. */
	UPROPERTY(transient)
	TEnumAsByte<enum EPixelFormat> Format;

	/** The number of mip-maps in the texture. */
	int32 NumMips;

	/** Whether the texture can be used as a resolve target. */
	uint32 bIsResolveTarget:1;


public:
	// Begin UTexture Interface.
	virtual FTextureResource* CreateResource() OVERRIDE;
	virtual EMaterialValueType GetMaterialType() OVERRIDE { return MCT_Texture2D; }
	virtual float GetSurfaceWidth() const OVERRIDE;
	virtual float GetSurfaceHeight() const OVERRIDE;
	// End UTexture Interface.
	
	/**
	 * Initializes the texture with 1 mip-level and creates the render resource.
	 *
	 * @param InSizeX			- Width of the texture, in texels
	 * @param InSizeY			- Height of the texture, in texels
	 * @param InFormat			- Format of the texture, defaults to PF_B8G8R8A8
	 * @param InIsResolveTarget	- Whether the texture can be used as a resolve target
	 */
	void Init(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat = PF_B8G8R8A8, bool InIsResolveTarget = false);
	
	/** Creates and initializes a new Texture2DDynamic with the requested settings */
	class UTexture2DDynamic* Create(int32 InSizeX, int32 InSizeY, EPixelFormat InFormat = PF_B8G8R8A8, bool InIsResolveTarget = false);
};
