// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	SlateDynamicImageBrush.h: Declares the FSlateDynamicImageBrush structure.
=============================================================================*/

#pragma once


/**
 * Ignores the Margin. Just renders the image. Can tile the image instead of stretching.
 */
struct SLATECORE_API FSlateDynamicImageBrush
	: public FSlateBrush
{
	/**
	 * @param InTexture		The UTexture2D being used for this brush.
	 * @param InImageSize		How large should the image be (not necessarily the image size on disk)
	 * @param InTint		The tint of the image
	 * @param InTiling		How do we tile if at all?
	 * @param InImageType		The type of image this this is
	 */
	FORCENOINLINE FSlateDynamicImageBrush( 
		class UTexture2D* InTexture, 
		const FVector2D& InImageSize,
		const FName InTextureName,
		const FLinearColor& InTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), 
		ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, 
		ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor
	)
		: FSlateBrush(ESlateBrushDrawType::Image, FName(TEXT("None")), FMargin(0.0f), InTiling, InImageType, InImageSize, InTint, (UObject*)InTexture)
	{
		bIsDynamicallyLoaded = true;

		// if we have a texture, make a unique name
		if (GetResourceObject() != nullptr)
		{
			ResourceName = InTextureName;
		}
	}

	/**
	 * @param InTextureName		The name of the texture to load.
	 * @param InImageSize		How large should the image be (not necessarily the image size on disk)
	 * @param InTint		The tint of the image.
	 * @param InTiling		How do we tile if at all?
	 * @param InImageType		The type of image this this is
	 */
	FSlateDynamicImageBrush( 
		const FName InTextureName,
		const FVector2D& InImageSize,
		const FLinearColor& InTint = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f), 
		ESlateBrushTileType::Type InTiling = ESlateBrushTileType::NoTile, 
		ESlateBrushImageType::Type InImageType = ESlateBrushImageType::FullColor
	)
		: FSlateBrush(ESlateBrushDrawType::Image, InTextureName, FMargin(0.0f), InTiling, InImageType, InImageSize, InTint, nullptr, true)
	{
		bIsDynamicallyLoaded = true;
	}

	/**
	 * Destructor.
	 */
	virtual ~FSlateDynamicImageBrush();
};
