// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "MaterialPropertyHelpers.h"
#include "Misc/MessageDialog.h"
#include "Misc/Guid.h"
#include "UObject/UnrealType.h"
#include "Layout/Margin.h"
#include "Misc/Attribute.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "EditorStyleSet.h"
#include "Materials/MaterialInterface.h"
#include "MaterialEditor/DEditorFontParameterValue.h"
#include "MaterialEditor/DEditorMaterialLayersParameterValue.h"
#include "MaterialEditor/DEditorScalarParameterValue.h"
#include "MaterialEditor/DEditorStaticComponentMaskParameterValue.h"
#include "MaterialEditor/DEditorStaticSwitchParameterValue.h"
#include "MaterialEditor/DEditorTextureParameterValue.h"
#include "MaterialEditor/DEditorVectorParameterValue.h"
#include "MaterialEditor/MaterialEditorInstanceConstant.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialExpressionParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter.h"
#include "Materials/MaterialExpressionFontSampleParameter.h"
#include "Materials/MaterialExpressionMaterialAttributeLayers.h"
#include "EditorSupportDelegates.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "IDetailPropertyRow.h"
#include "DetailLayoutBuilder.h"
#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "PropertyCustomizationHelpers.h"
#include "ScopedTransaction.h"
#include "Materials/MaterialInstanceConstant.h"

#define LOCTEXT_NAMESPACE "MaterialPropertyHelper"

EVisibility FMaterialPropertyHelpers::ShouldShowExpression(UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance, FGetShowHiddenParameters ShowHiddenDelegate)
{
	bool bShowHidden = true;

	ShowHiddenDelegate.ExecuteIfBound(bShowHidden);

	return (bShowHidden || MaterialEditorInstance->VisibleExpressions.Contains(Parameter->ParameterInfo))? EVisibility::Visible: EVisibility::Collapsed;
}

bool FMaterialPropertyHelpers::IsOverriddenExpression(UDEditorParameterValue* Parameter)
{
	return Parameter->bOverride != 0;
}

ECheckBoxState FMaterialPropertyHelpers::IsOverriddenExpressionCheckbox(UDEditorParameterValue* Parameter)
{
	bool bIsOverridden = false;
	if (Parameter != nullptr)
	{
		bIsOverridden = Parameter->bOverride;
	}
	return bIsOverridden ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void FMaterialPropertyHelpers::OnOverrideParameter(bool NewValue, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FScopedTransaction Transaction( LOCTEXT( "OverrideParameter", "Override Parameter" ) );
	Parameter->Modify();
	Parameter->bOverride = NewValue;

	// Fire off a dummy event to the material editor instance, so it knows to update the material, then refresh the viewports.
	FPropertyChangedEvent OverrideEvent(NULL);
	MaterialEditorInstance->PostEditChangeProperty( OverrideEvent );
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FMaterialPropertyHelpers::OnOverrideParameterCheckbox(ECheckBoxState NewValue, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FScopedTransaction Transaction(LOCTEXT("OverrideParameter", "Override Parameter"));
	Parameter->Modify();

	Parameter->bOverride = (NewValue == ECheckBoxState::Checked);

	// Fire off a dummy event to the material editor instance, so it knows to update the material, then refresh the viewports.
	FPropertyChangedEvent OverrideEvent(NULL);
	MaterialEditorInstance->PostEditChangeProperty(OverrideEvent);
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
	FEditorSupportDelegates::UpdateUI.Broadcast();
}

FText FMaterialPropertyHelpers::GetParameterExpressionDescription(UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	// TODO: This needs to support functions added by SourceInstance layers
	UMaterial* BaseMaterial = MaterialEditorInstance->SourceInstance->GetMaterial();
	if ( BaseMaterial )
	{
		UMaterialExpression* MaterialExpression = BaseMaterial->FindExpressionByGUID<UMaterialExpression>(Parameter->ExpressionId);

		if (MaterialExpression)
		{
			return FText::FromString(MaterialExpression->Desc);
		}
	}

	return FText::GetEmpty();
}

void FMaterialPropertyHelpers::ResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FScopedTransaction Transaction( LOCTEXT( "ResetToDefault", "Reset To Default" ) );
	Parameter->Modify();

	const FMaterialParameterInfo& ParameterInfo = Parameter->ParameterInfo;
	
	UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);
	UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
	UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
	UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
	UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
	UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(Parameter);

	if (ScalarParam)
	{
		float OutValue;
		if (MaterialEditorInstance->SourceInstance->GetScalarParameterDefaultValue(ParameterInfo, OutValue))
		{
			ScalarParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (VectorParam)
	{
		FLinearColor OutValue;
		if (MaterialEditorInstance->SourceInstance->GetVectorParameterDefaultValue(ParameterInfo, OutValue))
		{
			VectorParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (TextureParam)
	{
		UTexture* OutValue;
		if (MaterialEditorInstance->SourceInstance->GetTextureParameterDefaultValue(ParameterInfo, OutValue))
		{
			TextureParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (FontParam)
	{
		UFont* OutFontValue;
		int32 OutFontPage;
		if (MaterialEditorInstance->SourceInstance->GetFontParameterDefaultValue(ParameterInfo, OutFontValue, OutFontPage))
		{
			FontParam->ParameterValue.FontValue = OutFontValue;
			FontParam->ParameterValue.FontPage = OutFontPage;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (SwitchParam)
	{
		bool OutValue;
		FGuid TempGuid(0,0,0,0);
		if (MaterialEditorInstance->SourceInstance->GetStaticSwitchParameterDefaultValue(ParameterInfo, OutValue, TempGuid))
		{
			SwitchParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (CompMaskParam)
	{
		bool OutValue[4];
		FGuid TempGuid(0,0,0,0);
		if (MaterialEditorInstance->SourceInstance->GetStaticComponentMaskParameterDefaultValue(ParameterInfo, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
		{
			CompMaskParam->ParameterValue.R = OutValue[0];
			CompMaskParam->ParameterValue.G = OutValue[1];
			CompMaskParam->ParameterValue.B = OutValue[2];
			CompMaskParam->ParameterValue.A = OutValue[3];
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
}

void FMaterialPropertyHelpers::ResetLayerAssetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle,  class UDEditorParameterValue* InParameter, TEnumAsByte<EMaterialParameterAssociation> InAssociation, int32 Index, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FScopedTransaction Transaction(LOCTEXT("ResetToDefault", "Reset To Default"));
	InParameter->Modify();

	const FMaterialParameterInfo& ParameterInfo = InParameter->ParameterInfo;
	UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(InParameter);

	if (LayersParam)
	{
		FMaterialLayersFunctions LayersValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->Parent->GetMaterialLayersParameterValue(ParameterInfo, LayersValue, TempGuid))
		{
			FMaterialLayersFunctions StoredValue = LayersParam->ParameterValue;
			if (InAssociation == EMaterialParameterAssociation::BlendParameter)
			{
			
				if (Index < LayersValue.Blends.Num())
				{
					StoredValue.Blends[Index] = LayersValue.Blends[Index];
				}
				else
				{
					StoredValue.Blends[Index] = nullptr;
				}
			}
			else if (InAssociation == EMaterialParameterAssociation::LayerParameter)
			{
				if (Index < LayersValue.Layers.Num())
				{
					StoredValue.Layers[Index] = LayersValue.Layers[Index];
				}
				else
				{
					StoredValue.Layers[Index] = nullptr;
				}
			}
			LayersParam->ParameterValue = StoredValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
}

void FMaterialPropertyHelpers::ResetLayerParamToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, class UMaterialFunctionInterface* OwningInterface, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FScopedTransaction Transaction(LOCTEXT("ResetToDefault", "Reset To Default"));
	Parameter->Modify();

	const FMaterialParameterInfo& ParameterInfo = Parameter->ParameterInfo;

	UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);
	UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
	UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
	UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
	UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);

	if (ScalarParam)
	{
		float OutValue;
		if (MaterialEditorInstance->SourceInstance->GetScalarParameterDefaultValue(ParameterInfo, OutValue))
		{
			ScalarParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (VectorParam)
	{
		FLinearColor OutValue;
		if (MaterialEditorInstance->SourceInstance->GetVectorParameterDefaultValue(ParameterInfo, OutValue))
		{
			VectorParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (TextureParam)
	{
		UTexture* OutValue;
		if (MaterialEditorInstance->SourceInstance->GetTextureParameterDefaultValue(ParameterInfo, OutValue))
		{
			TextureParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (FontParam)
	{
		UFont* OutFontValue;
		int32 OutFontPage;
		if (MaterialEditorInstance->SourceInstance->GetFontParameterDefaultValue(ParameterInfo, OutFontValue, OutFontPage))
		{
			FontParam->ParameterValue.FontValue = OutFontValue;
			FontParam->ParameterValue.FontPage = OutFontPage;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (SwitchParam)
	{
		bool OutValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticSwitchParameterDefaultValue(ParameterInfo, OutValue, TempGuid))
		{
			SwitchParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (CompMaskParam)
	{
		bool OutValue[4];
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticComponentMaskParameterDefaultValue(ParameterInfo, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
		{
			CompMaskParam->ParameterValue.R = OutValue[0];
			CompMaskParam->ParameterValue.G = OutValue[1];
			CompMaskParam->ParameterValue.B = OutValue[2];
			CompMaskParam->ParameterValue.A = OutValue[3];
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
}

bool FMaterialPropertyHelpers::ShouldLayerAssetShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* InParameter, TEnumAsByte<EMaterialParameterAssociation> InAssociation, int32 Index, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FMaterialParameterInfo& ParameterInfo = InParameter->ParameterInfo;
	UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(InParameter);

	if (LayersParam)
	{
		FMaterialLayersFunctions LayersValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->Parent->GetMaterialLayersParameterValue(ParameterInfo, LayersValue, TempGuid))
		{
			FMaterialLayersFunctions StoredValue = LayersParam->ParameterValue;
			if (InAssociation == EMaterialParameterAssociation::BlendParameter)
			{

				if (Index < LayersValue.Blends.Num() && StoredValue.Blends[Index] == LayersValue.Blends[Index])
				{
					return false;
				}
				else
				{
					return true;
				}
			}
			else if (InAssociation == EMaterialParameterAssociation::LayerParameter)
			{
				if (Index < LayersValue.Layers.Num() && StoredValue.Layers[Index] == LayersValue.Layers[Index])
				{
					return false;
				}
				else
				{
					return true;
				}
			}
		}
	}
	return false;
}

bool FMaterialPropertyHelpers::ShouldShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FMaterialParameterInfo& ParameterInfo = Parameter->ParameterInfo;

	UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
	UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
	UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
	UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
	UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);
	UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(Parameter);

	if (ScalarParam)
	{
		float OutValue;
		if (MaterialEditorInstance->SourceInstance->GetScalarParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (ScalarParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (FontParam)
	{
		UFont* OutFontValue;
		int32 OutFontPage;
		if (MaterialEditorInstance->SourceInstance->GetFontParameterDefaultValue(ParameterInfo, OutFontValue, OutFontPage))
		{
			if (FontParam->ParameterValue.FontValue != OutFontValue ||
				FontParam->ParameterValue.FontPage != OutFontPage)
			{
				return true;
			}
		}
	}
	else if (TextureParam)
	{
		UTexture* OutValue;
		if (MaterialEditorInstance->SourceInstance->GetTextureParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (TextureParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (VectorParam)
	{
		FLinearColor OutValue;
		if (MaterialEditorInstance->SourceInstance->GetVectorParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (VectorParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (SwitchParam)
	{
		bool OutValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticSwitchParameterDefaultValue(ParameterInfo, OutValue, TempGuid))
		{
			if (SwitchParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (CompMaskParam)
	{
		bool OutValue[4];
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticComponentMaskParameterDefaultValue(ParameterInfo, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
		{
			if (CompMaskParam->ParameterValue.R != OutValue[0] ||
			CompMaskParam->ParameterValue.G != OutValue[1] ||
			CompMaskParam->ParameterValue.B != OutValue[2] ||
			CompMaskParam->ParameterValue.A != OutValue[3])
			{
				return true;
			}
		}
	}
	return false;
}

bool FMaterialPropertyHelpers::LayerParamShouldShowResetToDefault(TSharedPtr<IPropertyHandle> PropertyHandle, class UDEditorParameterValue* Parameter, UMaterialFunctionInterface* OwningInterface, UMaterialEditorInstanceConstant* MaterialEditorInstance)
{
	const FMaterialParameterInfo& ParameterInfo = Parameter->ParameterInfo;

	UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
	UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
	UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
	UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
	UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);

	if (ScalarParam)
	{
		float OutValue;
		if (MaterialEditorInstance->SourceInstance->GetScalarParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (ScalarParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (FontParam)
	{
		UFont* OutFontValue;
		int32 OutFontPage;
		if (MaterialEditorInstance->SourceInstance->GetFontParameterDefaultValue(ParameterInfo, OutFontValue, OutFontPage))
		{
			if (FontParam->ParameterValue.FontValue != OutFontValue ||
				FontParam->ParameterValue.FontPage != OutFontPage)
			{
				return true;
			}
		}
	}
	else if (TextureParam)
	{
		UTexture* OutValue;
		if (MaterialEditorInstance->SourceInstance->GetTextureParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (TextureParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (VectorParam)
	{
		FLinearColor OutValue;
		if (MaterialEditorInstance->SourceInstance->GetVectorParameterDefaultValue(ParameterInfo, OutValue))
		{
			if (VectorParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (SwitchParam)
	{
		bool OutValue;
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticSwitchParameterDefaultValue(ParameterInfo, OutValue, TempGuid))
		{
			if (SwitchParam->ParameterValue != OutValue)
			{
				return true;
			}
		}
	}
	else if (CompMaskParam)
	{
		bool OutValue[4];
		FGuid TempGuid(0, 0, 0, 0);
		if (MaterialEditorInstance->SourceInstance->GetStaticComponentMaskParameterDefaultValue(ParameterInfo, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
		{
			if (CompMaskParam->ParameterValue.R != OutValue[0] ||
				CompMaskParam->ParameterValue.G != OutValue[1] ||
				CompMaskParam->ParameterValue.B != OutValue[2] ||
				CompMaskParam->ParameterValue.A != OutValue[3])
			{
				return true;
			}
		}
	}
	return false;
}


#undef LOCTEXT_NAMESPACE

