// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionBumpOffset.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionBumpOffset : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	// Outputs: Coordinate + Eye.xy * (Height - ReferencePlane) * HeightRatio
	UPROPERTY()
	FExpressionInput Coordinate;

	UPROPERTY()
	FExpressionInput Height;

	UPROPERTY()
	FExpressionInput HeightRatioInput;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionBumpOffset)
	float HeightRatio;    // Perceived height as a fraction of width.

	UPROPERTY(EditAnywhere, Category=MaterialExpressionBumpOffset)
	float ReferencePlane;    // Height at which no offset is applied.


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



