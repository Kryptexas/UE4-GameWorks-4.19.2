// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionRotator.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionRotator : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to 'ConstCoordinate' if not specified"))
	FExpressionInput Coordinate;

	UPROPERTY(meta = (RequiredInput = "false", ToolTip = "Defaults to Game Time if not specified"))
	FExpressionInput Time;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionRotator)
	float CenterX;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionRotator)
	float CenterY;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionRotator)
	float Speed;

	/** only used if Coordinate is not hooked up */
	UPROPERTY(EditAnywhere, Category = MaterialExpressionRotator)
	uint32 ConstCoordinate;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual bool NeedsRealtimePreview() OVERRIDE { return Time.Expression==NULL && Speed != 0.f; }
	// End UMaterialExpression Interface

};



