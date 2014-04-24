// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "MaterialExpressionFontSampleParameter.generated.h"

UCLASS(HeaderGroup=Material, collapsecategories, hidecategories=Object, MinimalAPI)
class UMaterialExpressionFontSampleParameter : public UMaterialExpressionFontSample
{
	GENERATED_UCLASS_BODY()

	/** name to be referenced when we want to find and set thsi parameter */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionFontSampleParameter)
	FName ParameterName;

	/** GUID that should be unique within the material, this is used for parameter renaming. */
	UPROPERTY()
	FGuid ExpressionGUID;

	/** The name of the parameter Group to display in MaterialInstance Editor. Default is None group */
	UPROPERTY(EditAnywhere, Category=MaterialExpressionFontSampleParameter)
	FName Group;

	// Begin UMaterialExpression Interface
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex, int32 MultiplexIndex) OVERRIDE;
	virtual void GetCaption(TArray<FString>& OutCaptions) const OVERRIDE;
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) OVERRIDE;
	// End UMaterialExpression Interface
	
	/**
	*	Sets the default Font if none is set
	*/
	virtual void SetDefaultFont();
	
	ENGINE_API virtual FGuid& GetParameterExpressionId() OVERRIDE
	{
		return ExpressionGUID;
	}

	void GetAllParameterNames(TArray<FName> &OutParameterNames, TArray<FGuid> &OutParameterIds);
};



