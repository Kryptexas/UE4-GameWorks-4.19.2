// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionDepthOfFieldFunction.generated.h"

UENUM()
enum EDepthOfFieldFunctionValue
{
	// 0:in Focus .. 1:Near or Far
	TDOF_NearAndFarMask,
	// 0:in Focus or Far .. 1:Near
	TDOF_NearMask,
	// 0:in Focus or Near .. 1:Far
	TDOF_FarMask,
	TDOF_MAX,
};

UCLASS(HeaderGroup=Material)
class UMaterialExpressionDepthOfFieldFunction : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Determines the mapping place to use on the terrain. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionDepthOfFieldFunction)
	TEnumAsByte<enum EDepthOfFieldFunctionValue> FunctionValue;

	/** usually nothing or PixelDepth */
	UPROPERTY()
	FExpressionInput Depth;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



