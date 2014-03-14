// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionPanner.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionPanner : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Coordinate;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Time;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionPanner)
	float SpeedX;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionPanner)
	float SpeedY;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual bool NeedsRealtimePreview() OVERRIDE { return Time.Expression==NULL && (SpeedX != 0.f || SpeedY != 0.f); }
	// End UMaterialExpression Interface

};



