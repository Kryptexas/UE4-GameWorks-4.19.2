// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/SlateEnums.h"
#include "Layout/Visibility.h"
#include "Materials/MaterialLayersFunctions.h"

struct FAssetData;
class IDetailGroup;
class IDetailLayoutBuilder;
class IPropertyHandle;
class UMaterialEditorInstanceConstant;
enum class ECheckBoxState : uint8;

DECLARE_DELEGATE_OneParam(FGetShowHiddenParameters, bool&);


/*-----------------------------------------------------------------------------
   FMaterialInstanceBaseParameterDetails
-----------------------------------------------------------------------------*/

class FMaterialPropertyHelpers
{
public:
/** Returns true if the parameter is being overridden */
	static bool IsOverriddenExpression(class UDEditorParameterValue* Parameter);

	static ECheckBoxState IsOverriddenExpressionCheckbox(UDEditorParameterValue* Parameter);

	/** Gets the expression description of this parameter from the base material */
	static	FText GetParameterExpressionDescription(class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	/**
	 * Called when a parameter is overridden;
	 */
	static void OnOverrideParameter(bool NewValue, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	static void OnOverrideParameterCheckbox(ECheckBoxState NewValue, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	/** Reset to default implementation.  Resets Parameter to default */
	static void ResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	static void ResetLayerAssetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* InParameter, TEnumAsByte<EMaterialParameterAssociation> InAssociation, int32 Index, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	static void ResetLayerParamToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, class UMaterialFunctionInterface* OwningInterface, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	/** If reset to default button should show */
	static bool ShouldShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	static bool ShouldLayerAssetShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* InParameter, TEnumAsByte<EMaterialParameterAssociation> InAssociation, int32 Index, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	static bool LayerParamShouldShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, class UMaterialFunctionInterface* OwningInterface, UMaterialEditorInstanceConstant* MaterialEditorInstance);

	static EVisibility ShouldShowExpression(UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance, FGetShowHiddenParameters ShowHiddenDelegate);
};

