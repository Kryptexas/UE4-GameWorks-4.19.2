// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionConstant2Vector.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionConstant2Vector : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionConstant2Vector)
	float R;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionConstant2Vector)
	float G;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetOutputType(int32 OutputIndex) OVERRIDE {return MCT_Float2;}
#endif // WITH_EDITOR
	// End UMaterialExpression Interface
};



