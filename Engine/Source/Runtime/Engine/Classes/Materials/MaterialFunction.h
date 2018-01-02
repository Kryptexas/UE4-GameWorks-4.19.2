// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Misc/Guid.h"
#include "Templates/Casts.h"
#include "Materials/MaterialFunctionInterface.h"
#include "StaticParameterSet.h"
#include "MaterialFunction.generated.h"

class UMaterial;
class UTexture;
struct FPropertyChangedEvent;

/**
 * A Material Function is a collection of material expressions that can be reused in different materials
 */
UCLASS(BlueprintType, hidecategories=object, MinimalAPI)
class UMaterialFunction : public UMaterialFunctionInterface
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
	/** Used in the material editor, points to the function asset being edited, which this function is just a preview for. */
	UPROPERTY(transient)
	class UMaterialFunction* ParentFunction;

#endif // WITH_EDITORONLY_DATA
	/** Description of the function which will be displayed as a tooltip wherever the function is used. */
	UPROPERTY(EditAnywhere, Category=MaterialFunction, AssetRegistrySearchable)
	FString Description;

	/** Whether to list this function in the material function library, which is a window in the material editor that lists categorized functions. */
	UPROPERTY(EditAnywhere, Category=MaterialFunction, AssetRegistrySearchable)
	uint32 bExposeToLibrary:1;
	
	/** If true, parameters in this function will have a prefix added to their group name. */
	UPROPERTY(EditAnywhere, Category=MaterialFunction)
	uint32 bPrefixParameterNames:1;

#if WITH_EDITORONLY_DATA
	/** 
	 * Categories that this function belongs to in the material function library.  
	 * Ideally categories should be chosen carefully so that there are not too many.
	 */
	UPROPERTY(AssetRegistrySearchable)
	TArray<FString> LibraryCategories_DEPRECATED;

	/** 
	 * Categories that this function belongs to in the material function library.  
	 * Ideally categories should be chosen carefully so that there are not too many.
	 */
	UPROPERTY(EditAnywhere, Category=MaterialFunction, AssetRegistrySearchable)
	TArray<FText> LibraryCategoriesText;
#endif
	/** Array of material expressions, excluding Comments.  Used by the material editor. */
	UPROPERTY()
	TArray<class UMaterialExpression*> FunctionExpressions;

#if WITH_EDITORONLY_DATA
	/** Array of comments associated with this material; viewed in the material editor. */
	UPROPERTY()
	TArray<class UMaterialExpressionComment*> FunctionEditorComments;

	UPROPERTY(transient)
	UMaterial* PreviewMaterial;
#endif // WITH_EDITORONLY_DATA

private:
	/** Transient flag used to track re-entrance in recursive functions like IsDependent. */
	UPROPERTY(transient)
	uint32 bReentrantFlag:1;

public:
	//~ Begin UObject Interface.
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	//~ End UObject Interface.

	void SetMaterialFunctionUsage(EMaterialFunctionUsage Usage) { MaterialFunctionUsage = Usage; }

	//~ Begin UMaterialFunctionInterface interface
	virtual EMaterialFunctionUsage GetMaterialFunctionUsage() override { return MaterialFunctionUsage; }

	/** Recursively update all function call expressions in this function, or in nested functions. */
	virtual void UpdateFromFunctionResource() override;

	/** Get the inputs and outputs that this function exposes, for a function call expression to use. */
	virtual void GetInputsAndOutputs(TArray<struct FFunctionExpressionInput>& OutInputs, TArray<struct FFunctionExpressionOutput>& OutOutputs) const override;

	virtual bool ValidateFunctionUsage(class FMaterialCompiler* Compiler, const FFunctionExpressionOutput& Output) override;

#if WITH_EDITOR
	virtual int32 Compile(class FMaterialCompiler* Compiler, const struct FFunctionExpressionOutput& Output) override;

	/** Called during compilation before entering the function. */
	virtual void LinkIntoCaller(const TArray<FFunctionExpressionInput>& CallerInputs) override;

	virtual void UnlinkFromCaller() override;
#endif

	/** @return true if this function is dependent on the passed in function, directly or indirectly. */
	virtual bool IsDependent(UMaterialFunctionInterface* OtherFunction) override;

	/** Returns an array of the functions that this function is dependent on, directly or indirectly. */
	ENGINE_API virtual void GetDependentFunctions(TArray<UMaterialFunctionInterface*>& DependentFunctions) const override;

	/** Appends textures referenced by the expressions in this function. */
	virtual void AppendReferencedTextures(TArray<UTexture*>& InOutTextures) const override;

#if WITH_EDITOR
	virtual UMaterialInterface* GetPreviewMaterial() override;

	virtual void UpdateInputOutputTypes() override;

	/**
	 * Checks whether a Material Function is arranged in the old style, with inputs flowing from right to left
	 */
	virtual bool HasFlippedCoordinates() const override;
#endif

	virtual UMaterialFunctionInterface* GetBaseFunction() override { return this; }
	virtual const UMaterialFunctionInterface* GetBaseFunction() const override { return this; }
	virtual const TArray<UMaterialExpression*>* GetFunctionExpressions() const override { return &FunctionExpressions; }
	virtual const FString* GetDescription() const override { return &Description; }

	virtual bool GetReentrantFlag() const override { return bReentrantFlag; }
	virtual void SetReentrantFlag(const bool bIsReentrant) override { bReentrantFlag = bIsReentrant; }
	//~ End UMaterialFunctionInterface interface


#if WITH_EDITOR
	ENGINE_API bool SetVectorParameterValueEditorOnly(FName ParameterName, FLinearColor InValue);
	ENGINE_API bool SetScalarParameterValueEditorOnly(FName ParameterName, float InValue);
	ENGINE_API bool SetTextureParameterValueEditorOnly(FName ParameterName, class UTexture* InValue);
	ENGINE_API bool SetFontParameterValueEditorOnly(FName ParameterName, class UFont* InFontValue, int32 InFontPage);
	ENGINE_API bool SetStaticComponentMaskParameterValueEditorOnly(FName ParameterName, bool R, bool G, bool B, bool A, FGuid OutExpressionGuid);
	ENGINE_API bool SetStaticSwitchParameterValueEditorOnly(FName ParameterName, bool OutValue, FGuid OutExpressionGuid);
#endif // WITH_EDITOR
};
