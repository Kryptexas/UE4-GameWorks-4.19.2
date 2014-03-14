// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionTextureCoordinate.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionTextureCoordinate : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Texture coordinate index */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureCoordinate)
	int32 CoordinateIndex;

	/** Controls how much the texture tiles horizontally, by scaling the U component of the vertex UVs by the specified amount. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureCoordinate)
	float UTiling;

	/** Controls how much the texture tiles vertically, by scaling the V component of the vertex UVs by the specified amount. */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureCoordinate)
	float VTiling;

	/** Would like to unmirror U or V 
	 *  - if the texture is mirrored and if you would like to undo mirroring for this texture sample, use this to unmirror */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureCoordinate)
	uint32 UnMirrorU:1;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureCoordinate)
	uint32 UnMirrorV:1;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



