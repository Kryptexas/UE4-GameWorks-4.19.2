// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionQualitySwitch.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionQualitySwitch : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** Default connection, used when a specific quality level input is missing. */
	UPROPERTY()
	FExpressionInput Default;

	UPROPERTY()
	FExpressionInput Inputs[EMaterialQualityLevel::Num];

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	
	virtual const TArray<FExpressionInput*> GetInputs() OVERRIDE;
	virtual FExpressionInput* GetInput(int32 InputIndex) OVERRIDE;
	virtual FString GetInputName(int32 InputIndex) const OVERRIDE;
	virtual bool IsInputConnectionRequired(int32 InputIndex) const OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) OVERRIDE {return MCT_Unknown;}
	virtual uint32 GetOutputType(int32 InputIndex) OVERRIDE {return MCT_Unknown;}
#endif
	// End UMaterialExpression Interface
};
