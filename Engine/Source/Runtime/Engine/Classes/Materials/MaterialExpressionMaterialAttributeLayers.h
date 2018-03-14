// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "MaterialExpressionIO.h"
#include "Materials/MaterialExpression.h"
#include "Materials/MaterialLayersFunctions.h"
#include "MaterialExpressionMaterialAttributeLayers.generated.h"

class FMaterialCompiler;
class UMaterialFunctionInterface;
class UMaterialExpressionMaterialFunctionCall;
struct FMaterialParameterInfo;

UCLASS(hidecategories=Object, MinimalAPI)
class UMaterialExpressionMaterialAttributeLayers : public UMaterialExpression
{
	GENERATED_UCLASS_BODY()

	/** name to be referenced when we want to find and set this parameter */
	UPROPERTY(EditAnywhere, Category=LayersParameter)
	FName ParameterName;

	/** GUID that should be unique within the material, this is used for parameter renaming. */
	UPROPERTY()
	FGuid ExpressionGUID;

	UPROPERTY()
	FMaterialAttributesInput Input;

	UPROPERTY(EditAnywhere, Category=Layers)
	FMaterialLayersFunctions DefaultLayers;

	const TArray<UMaterialFunctionInterface*>& GetLayers() const
	{
		return ParamLayers ? ParamLayers->Layers : DefaultLayers.Layers;
	}

	const TArray<UMaterialFunctionInterface*>& GetBlends() const
	{
		return ParamLayers ? ParamLayers->Blends : DefaultLayers.Blends;
	}

#if WITH_EDITOR

	const TArray<FText>& GetLayerNames() const
	{
		return ParamLayers ? ParamLayers->LayerNames : DefaultLayers.LayerNames;
	}

	const TArray<bool>& GetShouldFilterLayers() const
	{
		return ParamLayers ? ParamLayers->RestrictToLayerRelatives : DefaultLayers.RestrictToLayerRelatives;
	}

	const TArray<bool>& GetShouldFilterBlends() const
	{
		return ParamLayers ? ParamLayers->RestrictToBlendRelatives : DefaultLayers.RestrictToBlendRelatives;
	}
#endif

	const TArray<bool>& GetLayerStates() const
	{
		return ParamLayers ? ParamLayers->LayerStates : DefaultLayers.LayerStates;
	}

	UPROPERTY(Transient)
	TArray<UMaterialExpressionMaterialFunctionCall*> LayerCallers;

	UPROPERTY(Transient)
	TArray<UMaterialExpressionMaterialFunctionCall*> BlendCallers;

	UPROPERTY(Transient)
	bool bIsLayerGraphBuilt;

	//~ Begin UObject Interface
	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	virtual bool NeedsLoadForClient() const override { return true; };
	//~ Begin UObject Interface

	ENGINE_API void RebuildLayerGraph(bool bReportErrors);
	ENGINE_API void OverrideLayerGraph(const FMaterialLayersFunctions* OverrideLayers);
	bool ValidateLayerConfiguration(FMaterialCompiler* Compiler, bool bReportErrors);

	void GetDependentFunctions(TArray<UMaterialFunctionInterface*>& DependentFunctions) const;
	UMaterialFunctionInterface* GetParameterAssociatedFunction(const FMaterialParameterInfo& ParameterInfo) const;
	void GetParameterAssociatedFunctions(const FMaterialParameterInfo& ParameterInfo, TArray<UMaterialFunctionInterface*>& AssociatedFunctions) const;

	//~ Begin UMaterialExpression Interface
#if WITH_EDITOR
	virtual int32 Compile(FMaterialCompiler* Compiler, int32 OutputIndex) override;
	virtual void GetCaption(TArray<FString>& OutCaptions) const override;
	virtual void GetExpressionToolTip(TArray<FString>& OutToolTip) override;
#endif
	virtual const TArray<FExpressionInput*> GetInputs()override;
	virtual FExpressionInput* GetInput(int32 InputIndex)override;
	virtual FName GetInputName(int32 InputIndex) const override;
	virtual bool IsInputConnectionRequired(int32 InputIndex) const override {return false;}
#if WITH_EDITOR
	virtual uint32 GetInputType(int32 InputIndex) override;
	virtual bool IsResultMaterialAttributes(int32 OutputIndex) override {return true;}
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
	bool IsNamedParameter(const FMaterialParameterInfo& ParameterInfo, FMaterialLayersFunctions& OutLayers, FGuid& OutExpressionGuid) const;
	
	ENGINE_API virtual FGuid& GetParameterExpressionId() override
	{
		return ExpressionGUID;
	}

	void GetAllParameterInfo(TArray<FMaterialParameterInfo> &OutParameterInfo, TArray<FGuid> &OutParameterIds, const FMaterialParameterInfo& InBaseParameterInfo) const;

private:
	/** Internal pointer to parameter-driven layer graph */
	const FMaterialLayersFunctions* ParamLayers;
};
