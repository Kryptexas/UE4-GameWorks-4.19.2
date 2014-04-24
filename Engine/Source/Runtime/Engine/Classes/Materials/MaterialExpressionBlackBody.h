// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionBlackBody.generated.h"

UCLASS(HeaderGroup=Material)
class UMaterialExpressionBlackBody : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Temperature */
	UPROPERTY()
	FExpressionInput Temp;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};
