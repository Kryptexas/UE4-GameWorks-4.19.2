// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *
 *	A material expression that routes particle emitter parameters to the
 *	material.
 */

#pragma once
#include "MaterialExpressionDynamicParameter.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionDynamicParameter : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** 
	 *	The names of the parameters.
	 *	These will show up in Cascade when editing a particle system
	 *	that uses the material it is in...
	 */
	UPROPERTY(EditAnywhere, editfixedsize, Category=MaterialExpressionDynamicParameter)
	TArray<FString> ParamNames;

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	// End UObject Interface

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual TArray<FExpressionOutput>& GetOutputs() OVERRIDE;
	virtual int32 GetWidth() const OVERRIDE;
	virtual int32 GetLabelPadding() OVERRIDE { return 8; }
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) OVERRIDE;
	// End UMaterialExpression Interface
};



