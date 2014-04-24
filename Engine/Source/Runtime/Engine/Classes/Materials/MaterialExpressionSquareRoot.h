// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionSquareRoot.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object)
class UMaterialExpressionSquareRoot : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	UPROPERTY()
	FExpressionInput Input;


	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
#if WITH_EDITOR
	virtual FString GetKeywords() const {return TEXT("sqrt");}
#endif // WITH_EDITOR
	// End UMaterialExpression Interface
};



