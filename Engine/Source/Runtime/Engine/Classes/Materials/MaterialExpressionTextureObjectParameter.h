// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Node which creates a texture parameter and outputs the texture object itself, instead of sampling the texture first.
 * This is used with material functions to implement texture parameters without actually putting the parameter in the function.
 */

#pragma once
#include "MaterialExpressionTextureObjectParameter.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionTextureObjectParameter : public UMaterialExpressionTextureSampleParameter
{
	GENERATED_UCLASS_BODY()


	// Begin UMaterialExpression Interface
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual const TArray<FExpressionInput*> GetInputs() OVERRIDE;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual int32 CompilePreview(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 InputIndex) OVERRIDE {return MCT_Texture;}
#endif
	// End UMaterialExpression Interface

	// Begin UMaterialExpressionTextureSampleParameter Interface
	virtual const TCHAR* GetRequirements() OVERRIDE;
	// End UMaterialExpressionTextureSampleParameter Interface

};



