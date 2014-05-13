// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "MaterialExpressionSpeedTreeColorVariation.generated.h"


UCLASS(collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionSpeedTreeColorVariation : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()	

	UPROPERTY()
	FExpressionInput Input;

	UPROPERTY(EditAnywhere, Category = MaterialExpressionSpeedTreeColorVariation, meta = (ToolTip = "The amount of color variation to apply"))
	float Amount;

	UPROPERTY(EditAnywhere, Category = MaterialExpressionSpeedTreeColorVariation, meta = (ToolTip = "Preserve the vibrance of the input in the output (helps when colors get darker as a result of the color adjustment)"))
	uint32 bPreserveVibrance : 1;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};


