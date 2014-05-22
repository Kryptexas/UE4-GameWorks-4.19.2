// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Absolute value material expression for user-defined materials
 *
 */

#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionAbs.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionAbs : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Link to the input expression to be evaluated */
	UPROPERTY()
	FExpressionInput Input;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface

};



