// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Misc/Guid.h"
#include "Materials/MaterialExpressionFontSample.h"
#include "MaterialExpressionFontSampleParameter.generated.h"

class UFont;
struct FMaterialParameterInfo;

UCLASS(collapsecategories, hidecategories=Object, MinimalAPI)
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

#if WITH_EDITORONLY_DATA
	/** Controls where the this parameter is displayed in a material instance parameter list. The lower the number the higher up in the parameter list. */
	UPROPERTY(EditAnywhere, Category = MaterialExpressionFontSampleParameter)
	int32 SortPriority;
#endif

	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
#endif
	virtual bool MatchesSearchQuery( const TCHAR* SearchQuery ) override;
#if WITH_EDITOR
	virtual bool CanRenameNode() const override { return true; }
	virtual FString GetEditableName() const override;
	virtual void SetEditableName(const FString& NewName) override;

	virtual bool HasAParameterName() const override { return true; }
	virtual FName GetParameterName() const override { return ParameterName; }
	virtual void SetParameterName(const FName& Name) override { ParameterName = Name; }
#endif
	//~ End UMaterialExpression Interface
	
	/** Return whether this is the named parameter, and fill in its value */
	bool IsNamedParameter(const FMaterialParameterInfo& ParameterInfo, UFont*& OutFontValue, int32& OutFontPage) const;

#if WITH_EDITOR
	bool SetParameterValue(FName InParameterName, UFont* InFontValue, int32 InFontPage);
#endif

	/**
	*	Sets the default Font if none is set
	*/
	virtual void SetDefaultFont();
	
	ENGINE_API virtual FGuid& GetParameterExpressionId() override
	{
		return ExpressionGUID;
	}

	void GetAllParameterInfo(TArray<FMaterialParameterInfo> &OutParameterInfo, TArray<FGuid> &OutParameterIds, const FMaterialParameterInfo& InBaseParameterInfo) const;
};



