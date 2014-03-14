// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionDesaturation.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionDesaturation : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	// Outputs: Lerp(Input,dot(Input,LuminanceFactors)),Fraction)
	UPROPERTY()
	FExpressionInput Input;

	UPROPERTY()
	FExpressionInput Fraction;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionDesaturation)
	FLinearColor LuminanceFactors;    // Color component factors for converting a color to greyscale.


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE
	{
		OutCaptions.Add(TEXT("Desaturation"));
	}
	// End UMaterialExpression Interface
};



