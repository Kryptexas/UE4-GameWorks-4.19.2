// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionDistanceCullFade.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionDistanceCullFade : public UMaterialExpression
{
	GENERATED_BODY()
public:
	UMaterialExpressionDistanceCullFade(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	// End UMaterialExpression Interface
};



