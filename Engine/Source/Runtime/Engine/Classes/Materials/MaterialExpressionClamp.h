// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionClamp.generated.h"

UENUM()
enum EClampMode
{
	CMODE_Clamp,
	CMODE_ClampMin,
	// Extra x to differentiate from the special value CMODE_Max, which is ignored by the property window
	CMODE_ClampMax,
};

UCLASS(HeaderGroup=Material, MinimalAPI)
class UMaterialExpressionClamp : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput Input;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Min;

	UPROPERTY(meta=(RequiredInput = "false"))
	FExpressionInput Max;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionClamp)
	TEnumAsByte<enum EClampMode> ClampMode;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionClamp)
	float MinDefault;

	UPROPERTY(EditAnywhere, Category=MaterialExpressionClamp)
	float MaxDefault;


	// Begin UObject Interface
	virtual void Serialize( FArchive& Ar ) OVERRIDE;
#if WITH_EDITOR
	virtual bool CanEditChange( const UProperty* InProperty ) const OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	// End UMaterialExpression Interface
};



