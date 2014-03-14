// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionAntialiasedTextureMask.generated.h"

UENUM()
enum ETextureColorChannel
{
	TCC_Red,
	TCC_Green,
	TCC_Blue,
	TCC_Alpha,
	TCC_MAX,
};

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionAntialiasedTextureMask : public UMaterialExpressionTextureSampleParameter2D
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionAntialiasedTextureMask, meta=(UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float Threshold;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionAntialiasedTextureMask)
	TEnumAsByte<enum ETextureColorChannel> Channel;


	// Begin UMaterialExpressionTextureSampleParameter Interface
	virtual bool TextureIsValid( UTexture* InTexture ) OVERRIDE;
	virtual const TCHAR* GetRequirements() OVERRIDE;
	virtual void SetDefaultTexture() OVERRIDE;
	// End UMaterialExpressionTextureSampleParameter Interface

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



