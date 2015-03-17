// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
	MaterialExpressionParticleDirection: Exposes the direction of a particle to
		the material editor.
==============================================================================*/

#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionParticleDirection.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionParticleDirection : public UMaterialExpression
{
	GENERATED_BODY()
public:
	UMaterialExpressionParticleDirection(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	// End UMaterialExpression Interface
};



