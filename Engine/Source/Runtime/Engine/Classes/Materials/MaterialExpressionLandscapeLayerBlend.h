// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionLandscapeLayerBlend.generated.h"

UENUM()
enum ELandscapeLayerBlendType
{
	LB_AlphaBlend,
	LB_HeightBlend,
	LB_MAX,
};

USTRUCT()
struct FLayerBlendInput
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=LayerBlendInput)
	FName LayerName;

	UPROPERTY(EditAnywhere, Category=LayerBlendInput)
	TEnumAsByte<enum ELandscapeLayerBlendType> BlendType;

	UPROPERTY()
	FExpressionInput LayerInput;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput HeightInput;

	UPROPERTY(EditAnywhere, Category=LayerBlendInput)
	float PreviewWeight;


	FLayerBlendInput()
		: BlendType(0)
		, PreviewWeight(0)
	{
	}
};

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionLandscapeLayerBlend : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeLayerBlend)
	TArray<struct FLayerBlendInput> Layers;

	/** GUID that should be unique within the material, this is used for parameter renaming. */
	UPROPERTY()
	FGuid ExpressionGUID;


	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	// End UObject Interface

	// Begin UMaterialExpression Interface
#endif
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) OVERRIDE;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual const TArray<FExpressionInput*> GetInputs() OVERRIDE;
	virtual FExpressionInput* GetInput(int32 InputIndex) OVERRIDE;
	virtual FString GetInputName(int32 InputIndex) const OVERRIDE;
	virtual UTexture* GetReferencedTexture() OVERRIDE;
	// End UMaterialExpression Interface

	ENGINE_API virtual FGuid& GetParameterExpressionId() OVERRIDE
	{
		return ExpressionGUID;
	}

	/**
	 * Get list of parameter names for static parameter sets
	 */
	void GetAllParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds);
};



