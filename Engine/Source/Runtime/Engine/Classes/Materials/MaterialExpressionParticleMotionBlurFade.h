// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	UMaterialExpressionParticleMotionBlurFade: Exposes a value used to fade
		particle sprites under the effects of motion blur.
==============================================================================*/

#pragma once

#include "MaterialExpressionParticleMotionBlurFade.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionParticleMotionBlurFade : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



