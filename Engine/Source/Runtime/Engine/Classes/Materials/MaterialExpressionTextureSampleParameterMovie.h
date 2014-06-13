// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Movie texture paramater for material instance
 *
 */

#pragma once
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "MaterialExpressionTextureSampleParameterMovie.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionTextureSampleParameterMovie : public UMaterialExpressionTextureSampleParameter
{
	GENERATED_UCLASS_BODY()


	// Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	// End UMaterialExpression Interface

	// Begin UMaterialExpressionTextureSampleParameter Interface
	virtual bool TextureIsValid( UTexture* InTexture ) override;
	virtual const TCHAR* GetRequirements() override;
	// End UMaterialExpressionTextureSampleParameter Interface
};



