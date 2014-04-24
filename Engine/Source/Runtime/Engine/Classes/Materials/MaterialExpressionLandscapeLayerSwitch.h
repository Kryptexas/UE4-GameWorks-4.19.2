// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionLandscapeLayerSwitch.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
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
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) OVERRIDE;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual UTexture* GetReferencedTexture() OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) OVERRIDE {return MCT_Unknown;}
	virtual uint32 GetOutputType(int32 InputIndex) OVERRIDE {return MCT_Unknown;}
#endif
	// End UMaterialExpression Interface

	ENGINE_API virtual FGuid& GetParameterExpressionId() OVERRIDE
	{
		return ExpressionGUID;
	}

	void GetAllParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds);
};



