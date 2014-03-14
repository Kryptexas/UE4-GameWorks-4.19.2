// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionSceneColor.generated.h"

UENUM()
namespace EMaterialSceneAttributeInputMode
{
	enum Type
	{
		Coordinates,
		OffsetFraction
	};
}

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionSceneColor : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()
	
	/**
	* Coordinates - UV coordinates to apply to the scene color lookup.
	* OffsetFraction - 	An offset to apply to the scene color lookup in a 2d fraction of the screen.
	*/ 
	UPROPERTY(EditAnywhere, Category=MaterialExpressionSceneColor)
	TEnumAsByte<enum EMaterialSceneAttributeInputMode::Type> InputMode;

	/**
	* Based on the input mode the input will be treated as either:
	* UV coordinates to apply to the scene color lookup or 
	* an offset to apply to the scene color lookup, in a 2d fraction of the screen.
	*/
	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Input;

	UPROPERTY()
	FExpressionInput OffsetFraction_DEPRECATED;

	// Begin UObject interface.
	virtual void PostLoad() OVERRIDE;
	// End UObject interface.

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual FString GetInputName(int32 InputIndex) const OVERRIDE;
	// End UMaterialExpression Interface
};



