// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionParticleSubUV.generated.h"

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionParticleSubUV : public UMaterialExpressionTextureSample
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionParticleSubUV)
	uint32 bBlend:1;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual int32 GetWidth() const OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual int32 GetLabelPadding() OVERRIDE { return 8; }
	// End UMaterialExpression Interface
};



