// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "MaterialExpressionTextureSampleParameterMedia.generated.h"


/**
 * Implements a material instance parameter for media textures.
 */
UCLASS(collapsecategories, hidecategories=Object)
class MEDIAASSETS_API UMaterialExpressionTextureSampleParameterMedia
	: public UMaterialExpressionTextureSampleParameter
{
	GENERATED_UCLASS_BODY()

public:

	// UMaterialExpressionTextureSampleParameter interface
	virtual const TCHAR* GetRequirements( ) override;
	virtual bool TextureIsValid( UTexture* InTexture ) override;

public:

	// UMaterialExpression Interface

	virtual void GetCaption( TArray<FString>& OutCaptions ) const override;
};
