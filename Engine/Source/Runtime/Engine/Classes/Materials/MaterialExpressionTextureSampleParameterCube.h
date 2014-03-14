// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionTextureSampleParameterCube.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionTextureSampleParameterCube : public UMaterialExpressionTextureSampleParameter
{
	GENERATED_UCLASS_BODY()




	// Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	// End UMaterialExpression Interface

	// Begin UMaterialExpressionTextureSampleParameter Interface
	virtual bool TextureIsValid( UTexture* InTexture ) OVERRIDE;
	virtual const TCHAR* GetRequirements() OVERRIDE;
	virtual void SetDefaultTexture() OVERRIDE;
	// End UMaterialExpressionTextureSampleParameter Interface
};



