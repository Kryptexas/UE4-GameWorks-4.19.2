// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionNormalize.generated.h"

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionNormalize : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput VectorInput;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE { OutCaptions.Add(TEXT("Normalize")); }
	// End UMaterialExpression Interface
};



