// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


/**
 * Allows the artists to quickly set up a Fresnel term. Returns:
 *
 *		pow(1 - max(Normal dot Camera,0),Exponent)
 */

#pragma once
#include "MaterialExpressionFresnel.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionFresnel : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput ExponentIn;

	/** The exponent to pass into the pow() function */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionFresnel)
	float Exponent;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput BaseReflectFractionIn;

	/** 
	 * Specifies the fraction of specular reflection when the surfaces is viewed from straight on.
	 * A value of 1 effectively disables Fresnel.
	 */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionFresnel)
	float BaseReflectFraction;

	/** The normal to dot with the camera FVector */
	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Normal;


	// Begin UObject Interface
#if WITH_EDITOR
	virtual bool CanEditChange( const UProperty* InProperty ) const OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE
	{
		OutCaptions.Add(FString(TEXT("Fresnel")));
	}
	// End UMaterialExpression Interface
};



