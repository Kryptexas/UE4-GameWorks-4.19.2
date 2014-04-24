// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionStaticSwitchParameter.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionStaticSwitchParameter : public UMaterialExpressionStaticBoolParameter
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput A;

	UPROPERTY()
	FExpressionInput B;


	// Begin UMaterialExpression Interface
	virtual FString GetInputName(int32 InputIndex) const OVERRIDE;
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) OVERRIDE {return MCT_Unknown;}
	virtual uint32 GetOutputType(int32 OutputIndex) OVERRIDE {return MCT_Unknown;}
#endif
	// End UMaterialExpression Interface
};

