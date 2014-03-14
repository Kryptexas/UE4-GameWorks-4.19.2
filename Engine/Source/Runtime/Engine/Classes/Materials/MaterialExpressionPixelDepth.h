// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Gives the depth of the current pixel being drawn for use in a material
 */

#pragma once
#include "MaterialExpressionPixelDepth.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionPixelDepth : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



