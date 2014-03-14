// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionTextureSample.generated.h"


/** defines how MipValue is used */
UENUM()
enum ETextureMipValueMode
{
	TMVM_None UMETA(DisplayName="None"),
	TMVM_MipLevel UMETA(DisplayName="MipLevel"),
	TMVM_MipBias UMETA(DisplayName="MipBias"),
	TMVM_MAX,
};

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionTextureSample : public UMaterialExpressionTextureBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Coordinates;

	/** 
	 * Texture object input which overrides Texture if specified. 
	 * This only shows up in material functions and is used to implement texture parameters without actually putting the texture parameter in the function.
	 */
	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput TextureObject;

	/** Meaning depends on MipValueMode */
	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput MipValue;

	/** Noise function, affects performance and look */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionTextureSample, meta=(DisplayName = "MipValueMode"))
	TEnumAsByte<enum ETextureMipValueMode> MipValueMode;

	// Begin UObject Interface
#if WITH_EDITOR
	virtual bool CanEditChange(const UProperty* InProperty) const OVERRIDE;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual const TArray<FExpressionInput*> GetInputs() OVERRIDE;
	virtual FExpressionInput* GetInput(int32 InputIndex) OVERRIDE;
	virtual FString GetInputName(int32 InputIndex) const OVERRIDE;
	virtual int32 GetWidth() const OVERRIDE;
	virtual int32 GetLabelPadding() OVERRIDE { return 8; }
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex);
#endif
	// End UMaterialExpression Interface

	void UpdateTextureResource(class UTexture* Texture);
};



