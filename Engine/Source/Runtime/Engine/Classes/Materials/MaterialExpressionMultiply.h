// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionMultiply.generated.h"

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionMultiply : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput A;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput B;

	/** only used if A is not hooked up */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionMultiply)
	float ConstA;

	/** only used if B is not hooked up */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionMultiply)
	float ConstB;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
#if WITH_EDITOR
	virtual FString GetKeywords() const {return TEXT("*");}
#endif // WITH_EDITOR
	// End UMaterialExpression Interface
};



