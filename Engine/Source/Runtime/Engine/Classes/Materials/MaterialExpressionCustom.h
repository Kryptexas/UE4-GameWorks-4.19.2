// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionCustom.generated.h"

UENUM()
enum ECustomMaterialOutputType
{
	CMOT_Float1,
	CMOT_Float2,
	CMOT_Float3,
	CMOT_Float4,
	CMOT_MAX,
};

USTRUCT()
struct FCustomInput
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category=CustomInput)
	FString InputName;

	UPROPERTY()
	FExpressionInput Input;

};

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionCustom : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, Category=MaterialExpressionCustom)
	FString Code;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionCustom)
	TEnumAsByte<enum ECustomMaterialOutputType> OutputType;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionCustom)
	FString Description;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionCustom)
	TArray<struct FCustomInput> Inputs;


	// Begin UObject interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject interface.
	
	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual const TArray<FExpressionInput*> GetInputs() OVERRIDE;
	virtual FExpressionInput* GetInput(int32 InputIndex) OVERRIDE;
	virtual FString GetInputName(int32 InputIndex) const OVERRIDE;
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) OVERRIDE {return MCT_Unknown;}
	virtual uint32 GetOutputType(int32 OutputIndex) OVERRIDE;
#endif // WITH_EDITOR
	// End UMaterialExpression Interface
};



