// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	UMaterialExpressionParticleMotionBlurFade: Exposes a value used to fade
		particle sprites under the effects of motion blur.
==============================================================================*/

#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionParticleMotionBlurFade.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionParticleMotionBlurFade : public UMaterialExpression
{
	GENERATED_BODY()
public:
	UMaterialExpressionParticleMotionBlurFade(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	// End UMaterialExpression Interface
};



