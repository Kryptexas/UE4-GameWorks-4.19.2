// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "Materials/MaterialExpression.h"
#include "MaterialExpressionBlackBody.generated.h"

UCLASS()
class UMaterialExpressionBlackBody : public UMaterialExpression
{
	GENERATED_BODY()
public:
	UMaterialExpressionBlackBody(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Temperature */
	UPROPERTY()
	FExpressionInput Temp;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	// End UMaterialExpression Interface
};
