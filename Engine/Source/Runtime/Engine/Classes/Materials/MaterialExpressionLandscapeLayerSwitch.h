// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionLandscapeLayerSwitch.generated.h"

UCLASS(collapsecategories, hidecategories=Object)
class UMaterialExpressionLandscapeLayerSwitch : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput LayerUsed;

	UPROPERTY()
	FExpressionInput LayerNotUsed;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeLayerSwitch)
	FName ParameterName;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionLandscapeLayerSwitch)
	uint32 PreviewUsed:1;

	/** GUID that should be unique within the material, this is used for parameter renaming. */
	UPROPERTY()
	FGuid ExpressionGUID;


public:

	// Begin UObject Interface
	virtual void Serialize(FArchive& Ar) override;
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) override;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual UTexture* GetReferencedTexture() override;
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) override {return MCT_Unknown;}
	virtual uint32 GetOutputType(int32 InputIndex) override {return MCT_Unknown;}
#endif
	// End UMaterialExpression Interface

	ENGINE_API virtual FGuid& GetParameterExpressionId() override
	{
		return ExpressionGUID;
	}

	void GetAllParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds);
};



