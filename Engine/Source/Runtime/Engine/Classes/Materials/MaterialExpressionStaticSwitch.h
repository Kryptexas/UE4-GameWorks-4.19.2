// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionStaticSwitch.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionStaticSwitch : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionStaticSwitch)
	uint32 DefaultValue:1;

	UPROPERTY()
	FExpressionInput A;

	UPROPERTY()
	FExpressionInput B;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Value;


	// Begin UObject Interface
#if WITH_EDITOR
	virtual bool CanEditChange( const UProperty* InProperty ) const OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual FString GetInputName(int32 InputIndex) const OVERRIDE;
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) OVERRIDE;
	virtual uint32 GetOutputType(int32 OutputIndex) OVERRIDE {return MCT_Unknown;}
#endif
	// End UMaterialExpression Interface
};



