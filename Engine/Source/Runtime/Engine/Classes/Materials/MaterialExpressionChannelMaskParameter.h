// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "MaterialExpressionChannelMaskParameter.generated.h"

UENUM()
namespace EChannelMaskParameterColor
{
	enum Type
	{
		Red,
		Green,
		Blue,
		Alpha,
	};
}

UCLASS(collapsecategories, hidecategories=(Object, MaterialExpressionVectorParameter), MinimalAPI)
class UMaterialExpressionChannelMaskParameter : public UMaterialExpressionVectorParameter
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionChannelMaskParameter)
	TEnumAsByte<EChannelMaskParameterColor::Type> MaskChannel;

	UPROPERTY()
	FExpressionInput Input;

#if WITH_EDITOR
	virtual bool SetParameterValue(FName InParameterName, FLinearColor InValue) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif
	
	virtual bool IsInputConnectionRequired(int32 InputIndex) const override {return true;}
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) override {return MCT_Float4;}
#endif

	virtual bool IsUsedAsChannelMask() override {return true;}
};
