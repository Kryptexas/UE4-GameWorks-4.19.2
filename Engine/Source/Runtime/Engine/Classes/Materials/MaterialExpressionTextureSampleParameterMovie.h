// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movie texture paramater for material instance
 *
 */

#pragma once
#include "MaterialExpressionTextureSampleParameterMovie.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionTextureSampleParameterMovie : public UMaterialExpressionTextureSampleParameter
{
	GENERATED_UCLASS_BODY()


	// Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface

	// Begin UMaterialExpressionTextureSampleParameter Interface
	virtual bool TextureIsValid( UTexture* InTexture ) OVERRIDE;
	virtual const TCHAR* GetRequirements() OVERRIDE;
	// End UMaterialExpressionTextureSampleParameter Interface
};



