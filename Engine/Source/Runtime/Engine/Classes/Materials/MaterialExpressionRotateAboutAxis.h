// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionRotateAboutAxis.generated.h"

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionRotateAboutAxis : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput NormalizedRotationAxis;

	UPROPERTY()
	FExpressionInput RotationAngle;

	UPROPERTY()
	FExpressionInput PivotPoint;

	UPROPERTY()
	FExpressionInput Position;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionRotateAboutAxis)
	float Period;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface

};



