// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionDDY.generated.h"


UCLASS(HeaderGroup=Material)
class UMaterialExpressionDDY : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** The value we want to compute ddx/ddy from */
	UPROPERTY()
	FExpressionInput Value;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



