// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionLinearInterpolate.generated.h"

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionLinearInterpolate : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput A;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput B;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Alpha;

	/** only used if A is not hooked up */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLinearInterpolate)
	float ConstA;

	/** only used if B is not hooked up */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLinearInterpolate)
	float ConstB;

	/** only used if Alpha is not hooked up */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionLinearInterpolate)
	float ConstAlpha;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
#if WITH_EDITOR
	virtual FString GetKeywords() const {return TEXT("lerp");}
#endif // WITH_EDITOR
	// End UMaterialExpression Interface
};



