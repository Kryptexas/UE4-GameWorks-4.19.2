// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MaterialEditorInstanceDetailCustomization.h"
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
#include "Materials/MaterialInterface.h"
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
#include "MaterialPropertyHelpers.h"
#include "Widgets/Input/SButton.h"
#include "SBox.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "ModuleManager.h"
#include "AssetToolsModule.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInstance.h"

#define LOCTEXT_NAMESPACE "MaterialInstanceEditor"

TSharedRef<IDetailCustomization> FMaterialInstanceParameterDetails::MakeInstance(UMaterialEditorInstanceConstant* MaterialInstance, FGetShowHiddenParameters InShowHiddenDelegate)
{
	return MakeShareable(new FMaterialInstanceParameterDetails(MaterialInstance, InShowHiddenDelegate));
}

FMaterialInstanceParameterDetails::FMaterialInstanceParameterDetails(UMaterialEditorInstanceConstant* MaterialInstance, FGetShowHiddenParameters InShowHiddenDelegate)
	: MaterialEditorInstance(MaterialInstance)
	, ShowHiddenDelegate(InShowHiddenDelegate)
{
}

TOptional<float> FMaterialInstanceParameterDetails::OnGetValue(TSharedRef<IPropertyHandle> PropertyHandle)
{
	float Value = 0.0f;
	if (PropertyHandle->GetValue(Value) == FPropertyAccess::Success)
	{
		return TOptional<float>(Value);
	}

	// Value couldn't be accessed. Return an unset value
	return TOptional<float>();
}

void FMaterialInstanceParameterDetails::OnValueCommitted(float NewValue, ETextCommit::Type CommitType, TSharedRef<IPropertyHandle> PropertyHandle)
{	
	// Try setting as float, if that fails then set as int
	ensure(PropertyHandle->SetValue(NewValue) == FPropertyAccess::Success);
}

FString FMaterialInstanceParameterDetails::GetFunctionParentPath() const
{
	FString PathString;
	if (MaterialEditorInstance->SourceFunction)
	{
		PathString = MaterialEditorInstance->SourceFunction->Parent->GetPathName();
	}
	return PathString;
}

void FMaterialInstanceParameterDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	// Create a new category for a custom layout for the MIC parameters at the very top
	FName GroupsCategoryName = TEXT("ParameterGroups");
	IDetailCategoryBuilder& GroupsCategory = DetailLayout.EditCategory(GroupsCategoryName, LOCTEXT("MICParamGroupsTitle", "Parameter Groups"));
	TSharedRef<IPropertyHandle> ParameterGroupsProperty = DetailLayout.GetProperty("ParameterGroups");

	CreateGroupsWidget(ParameterGroupsProperty, GroupsCategory);

	// Create default category for class properties
	const FName DefaultCategoryName = NAME_None;
	IDetailCategoryBuilder& DefaultCategory = DetailLayout.EditCategory(DefaultCategoryName);
	DetailLayout.HideProperty("MaterialLayersParameterValues");
	if (MaterialEditorInstance->bIsFunctionPreviewMaterial)
	{
		// Customize Parent property so we can check for recursively set parents
		bool bShowParent = false;
		if(MaterialEditorInstance->SourceFunction->GetMaterialFunctionUsage() != EMaterialFunctionUsage::Default)
		{
			bShowParent = true;
		}
		if (bShowParent)
		{
			TSharedRef<IPropertyHandle> ParentPropertyHandle = DetailLayout.GetProperty("Parent");
			IDetailPropertyRow& ParentPropertyRow = DefaultCategory.AddProperty(ParentPropertyHandle);
			ParentPropertyHandle->MarkResetToDefaultCustomized();

			TSharedPtr<SWidget> NameWidget;
			TSharedPtr<SWidget> ValueWidget;
			FDetailWidgetRow Row;

			ParentPropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);

			ParentPropertyHandle->ClearResetToDefaultCustomized();

			const bool bShowChildren = true;
			ParentPropertyRow.CustomWidget(bShowChildren)
				.NameContent()
				.MinDesiredWidth(Row.NameWidget.MinWidth)
				.MaxDesiredWidth(Row.NameWidget.MaxWidth)
				[
					NameWidget.ToSharedRef()
				]
			.ValueContent()
				.MinDesiredWidth(Row.ValueWidget.MinWidth)
				.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
				[
					SNew(SObjectPropertyEntryBox)
					.ObjectPath(this, &FMaterialInstanceParameterDetails::GetFunctionParentPath)
					.AllowedClass(UMaterialFunctionInterface::StaticClass())
					.ThumbnailPool(DetailLayout.GetThumbnailPool())
					.AllowClear(true)
					.OnObjectChanged(this, &FMaterialInstanceParameterDetails::OnAssetChanged, ParentPropertyHandle)
					.OnShouldSetAsset(this, &FMaterialInstanceParameterDetails::OnShouldSetAsset)
					.NewAssetFactories(TArray<UFactory*>())
				];

			ValueWidget.Reset();


		}
		else
		{
			DetailLayout.HideProperty("Parent");
		}

		DetailLayout.HideProperty("PhysMaterial");
		DetailLayout.HideProperty("LightmassSettings");
		DetailLayout.HideProperty("bUseOldStyleMICEditorGroups");
		DetailLayout.HideProperty("ParameterGroups");
		DetailLayout.HideProperty("RefractionDepthBias");
		DetailLayout.HideProperty("bOverrideSubsurfaceProfile");
		DetailLayout.HideProperty("SubsurfaceProfile");
		DetailLayout.HideProperty("BasePropertyOverrides");
	}
	else
	{
		// Add PhysMaterial property
		DefaultCategory.AddProperty("PhysMaterial");

		// Customize Parent property so we can check for recursively set parents
		TSharedRef<IPropertyHandle> ParentPropertyHandle = DetailLayout.GetProperty("Parent");
		IDetailPropertyRow& ParentPropertyRow = DefaultCategory.AddProperty(ParentPropertyHandle);

		ParentPropertyHandle->MarkResetToDefaultCustomized();
	
		TSharedPtr<SWidget> NameWidget;
		TSharedPtr<SWidget> ValueWidget;
		FDetailWidgetRow Row;

		ParentPropertyRow.GetDefaultWidgets(NameWidget, ValueWidget, Row);
	
		ParentPropertyHandle->ClearResetToDefaultCustomized();

		const bool bShowChildren = true;
		ParentPropertyRow.CustomWidget(bShowChildren)
			.NameContent()
			.MinDesiredWidth(Row.NameWidget.MinWidth)
			.MaxDesiredWidth(Row.NameWidget.MaxWidth)
			[
				NameWidget.ToSharedRef()
			]
			.ValueContent()
			.MinDesiredWidth(Row.ValueWidget.MinWidth)
			.MaxDesiredWidth(Row.ValueWidget.MaxWidth)
			[
				SNew(SObjectPropertyEntryBox)
				.PropertyHandle(ParentPropertyHandle)
				.AllowedClass(UMaterialInterface::StaticClass())
				.ThumbnailPool(DetailLayout.GetThumbnailPool())
				.AllowClear(true)
				.OnShouldSetAsset(this, &FMaterialInstanceParameterDetails::OnShouldSetAsset)
			];

		ValueWidget.Reset();


		// Add/hide other properties
		DefaultCategory.AddProperty("LightmassSettings");
		DetailLayout.HideProperty("bUseOldStyleMICEditorGroups");
		DetailLayout.HideProperty("ParameterGroups");

		{
			IDetailPropertyRow& PropertyRow = DefaultCategory.AddProperty("RefractionDepthBias");
			PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::ShouldShowMaterialRefractionSettings)));
		}

		{
			IDetailPropertyRow& PropertyRow = DefaultCategory.AddProperty("bOverrideSubsurfaceProfile");
			PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::ShouldShowSubsurfaceProfile)));
		}

		{
			IDetailPropertyRow& PropertyRow = DefaultCategory.AddProperty("SubsurfaceProfile");
			PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::ShouldShowSubsurfaceProfile)));
		}

		DetailLayout.HideProperty("BasePropertyOverrides");
		CreateBasePropertyOverrideWidgets(DetailLayout);
	}

	// Add the preview mesh property directly from the material instance 
	FName PreviewingCategoryName = TEXT("Previewing");
	IDetailCategoryBuilder& PreviewingCategory = DetailLayout.EditCategory(PreviewingCategoryName, LOCTEXT("MICPreviewingCategoryTitle", "Previewing"));

	TArray<UObject*> ExternalObjects;
	ExternalObjects.Add(MaterialEditorInstance->SourceInstance);

	PreviewingCategory.AddExternalObjectProperty(ExternalObjects, TEXT("PreviewMesh"));

}

void FMaterialInstanceParameterDetails::CreateGroupsWidget(TSharedRef<IPropertyHandle> ParameterGroupsProperty, IDetailCategoryBuilder& GroupsCategory)
{
	bool bShowSaveButtons = false;
	check(MaterialEditorInstance);
	for (int32 GroupIdx = 0; GroupIdx < MaterialEditorInstance->ParameterGroups.Num(); ++GroupIdx)
	{
		FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];
		if (ParameterGroup.GroupAssociation == EMaterialParameterAssociation::GlobalParameter
			&& ParameterGroup.GroupName != FMaterialPropertyHelpers::LayerParamName)
		{
			bShowSaveButtons = true;
			IDetailGroup& DetailGroup = GroupsCategory.AddGroup(ParameterGroup.GroupName, FText::FromName(ParameterGroup.GroupName), false, true);
			CreateSingleGroupWidget(ParameterGroup, ParameterGroupsProperty->GetChildHandle(GroupIdx), DetailGroup);
		}
	}
	if (bShowSaveButtons)
	{
		FDetailWidgetRow& SaveInstanceRow = GroupsCategory.AddCustomRow(LOCTEXT("SaveInstances", "Save Instances"));
		FOnClicked OnChildButtonClicked;
		FOnClicked OnSiblingButtonClicked;
		UMaterialInterface* LocalSourceInstance = MaterialEditorInstance->SourceInstance;
		UObject* LocalEditorInstance = MaterialEditorInstance;
		if (!MaterialEditorInstance->bIsFunctionPreviewMaterial)
		{
			OnChildButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, LocalSourceInstance, LocalEditorInstance);
			OnSiblingButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewMaterialInstance, MaterialEditorInstance->SourceInstance->Parent, LocalEditorInstance);
		}
		else
		{
			OnChildButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewFunctionInstance, 
				ImplicitConv<UMaterialFunctionInterface*>(MaterialEditorInstance->SourceFunction), LocalSourceInstance, LocalEditorInstance);
			OnSiblingButtonClicked = FOnClicked::CreateStatic(&FMaterialPropertyHelpers::OnClickedSaveNewFunctionInstance,
				ImplicitConv<UMaterialFunctionInterface*>(MaterialEditorInstance->SourceFunction->Parent), LocalSourceInstance, LocalEditorInstance);
		}
		SaveInstanceRow.ValueContent()
			.HAlign(HAlign_Fill)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				[
					SNullWidget::NullWidget
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Dark")
					.HAlign(HAlign_Center)
					.OnClicked(OnSiblingButtonClicked)
					.ToolTipText(LOCTEXT("SaveToSiblingInstance", "Save To Sibling Instance"))
					.Content()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.10"))
							.TextStyle(FEditorStyle::Get(), "NormalText.Important")
							.Text(FText::FromString(FString(TEXT("\xf0c7 \xf178"))) /*fa-filter*/)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "NormalText.Important")
							.Text(FText::FromString(FString(TEXT(" Save Sibling"))) /*fa-filter*/)
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(2.0f)
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "FlatButton.Dark")
					.HAlign(HAlign_Center)
					.OnClicked(OnChildButtonClicked)
					.ToolTipText(LOCTEXT("SaveToChildInstance", "Save To Child Instance"))
					.Content()
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.Font(FEditorStyle::Get().GetFontStyle("FontAwesome.10"))
							.TextStyle(FEditorStyle::Get(), "NormalText.Important")
							.Text(FText::FromString(FString(TEXT("\xf0c7 \xf149"))) /*fa-filter*/)
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(STextBlock)
							.TextStyle(FEditorStyle::Get(), "NormalText.Important")
							.Text(FText::FromString(FString(TEXT(" Save Child"))) /*fa-filter*/)
						]
					]
				]
			];
	}
}



void FMaterialInstanceParameterDetails::CreateSingleGroupWidget(FEditorParameterGroup& ParameterGroup, TSharedPtr<IPropertyHandle> ParameterGroupProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> ParametersArrayProperty = ParameterGroupProperty->GetChildHandle("Parameters");

	// Create a custom widget for each parameter in the group
	for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
	{
		TSharedPtr<IPropertyHandle> ParameterProperty = ParametersArrayProperty->GetChildHandle(ParamIdx);

		UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
		UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
		UDEditorMaterialLayersParameterValue* LayersParam = Cast<UDEditorMaterialLayersParameterValue>(Parameter);
		UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
		UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
		UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
		UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
		UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);

		if (Parameter->ParameterInfo.Association == EMaterialParameterAssociation::GlobalParameter)
		{
			if (VectorParam && VectorParam->bIsUsedAsChannelMask)
			{
				CreateVectorChannelMaskParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
			else if (ScalarParam || SwitchParam || TextureParam || VectorParam || FontParam)
			{
				if (ScalarParam && ScalarParam->SliderMax > ScalarParam->SliderMin)
				{
					TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");
					ParameterValueProperty->SetInstanceMetaData("UIMin", FString::Printf(TEXT("%f"), ScalarParam->SliderMin));
					ParameterValueProperty->SetInstanceMetaData("UIMax", FString::Printf(TEXT("%f"), ScalarParam->SliderMax));
				}

				CreateParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
			else if (LayersParam)
			{
			}
			else if (CompMaskParam)
			{
				CreateMaskParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
			}
			else
			{
				// Unsupported parameter type
				check(false);
			}
		}
	}
}

void FMaterialInstanceParameterDetails::CreateParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup)
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");

	if (ParameterValueProperty->IsValidHandle())
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());

		FIsResetToDefaultVisible IsResetVisible = FIsResetToDefaultVisible::CreateStatic(&FMaterialPropertyHelpers::ShouldShowResetToDefault, Parameter, MaterialEditorInstance);
		FResetToDefaultHandler ResetHandler = FResetToDefaultHandler::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, Parameter, MaterialEditorInstance);
		FResetToDefaultOverride ResetOverride = FResetToDefaultOverride::Create(IsResetVisible, ResetHandler);

		PropertyRow
			.DisplayName(FText::FromName(Parameter->ParameterInfo.Name))
			.ToolTip(FMaterialPropertyHelpers::GetParameterExpressionDescription(Parameter, MaterialEditorInstance))
			.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance))
			.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)))
			.OverrideResetToDefault(ResetOverride);
	}
}

void FMaterialInstanceParameterDetails::CreateMaskParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");
	TSharedPtr<IPropertyHandle> RMaskProperty = ParameterValueProperty->GetChildHandle("R");
	TSharedPtr<IPropertyHandle> GMaskProperty = ParameterValueProperty->GetChildHandle("G");
	TSharedPtr<IPropertyHandle> BMaskProperty = ParameterValueProperty->GetChildHandle("B");
	TSharedPtr<IPropertyHandle> AMaskProperty = ParameterValueProperty->GetChildHandle("A");

	if (ParameterValueProperty->IsValidHandle())
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());
		PropertyRow.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance));
		// Handle reset to default manually
		PropertyRow.OverrideResetToDefault(FResetToDefaultOverride::Create(FResetToDefaultHandler::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, Parameter, MaterialEditorInstance)));
		PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)));

		const FText ParameterName = FText::FromName(Parameter->ParameterInfo.Name);

		FDetailWidgetRow& CustomWidget = PropertyRow.CustomWidget();
		CustomWidget
			.FilterString(ParameterName)
			.NameContent()
			[
				SNew(STextBlock)
				.Text(ParameterName)
				.ToolTipText(FMaterialPropertyHelpers::GetParameterExpressionDescription(Parameter, MaterialEditorInstance))
				.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
			]
		.ValueContent()
			.MaxDesiredWidth(200.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				RMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				RMaskProperty->CreatePropertyValueWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
			.AutoWidth()
			[
				GMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				GMaskProperty->CreatePropertyValueWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
			.AutoWidth()
			[
				BMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				BMaskProperty->CreatePropertyValueWidget()
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.Padding(FMargin(10.0f, 0.0f, 0.0f, 0.0f))
			.AutoWidth()
			[
				AMaskProperty->CreatePropertyNameWidget(FText::GetEmpty(), FText::GetEmpty(), false)
			]
		+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			.AutoWidth()
			[
				AMaskProperty->CreatePropertyValueWidget()
			]
			]
			];
	}
}

void FMaterialInstanceParameterDetails::CreateVectorChannelMaskParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup)
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");

	if (ParameterValueProperty->IsValidHandle())
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateStatic(&FMaterialPropertyHelpers::IsOverriddenExpression, Parameter));

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow(ParameterValueProperty.ToSharedRef());
		PropertyRow.EditCondition(IsParamEnabled, FOnBooleanValueChanged::CreateStatic(&FMaterialPropertyHelpers::OnOverrideParameter, Parameter, MaterialEditorInstance));
		// Handle reset to default manually
		PropertyRow.OverrideResetToDefault(FResetToDefaultOverride::Create(FResetToDefaultHandler::CreateStatic(&FMaterialPropertyHelpers::ResetToDefault, Parameter, MaterialEditorInstance)));
		PropertyRow.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateStatic(&FMaterialPropertyHelpers::ShouldShowExpression, Parameter, MaterialEditorInstance, ShowHiddenDelegate)));

		const FText ParameterName = FText::FromName(Parameter->ParameterInfo.Name);

		// Combo box hooks for converting between our "enum" and colors
		FOnGetPropertyComboBoxStrings GetMaskStrings = FOnGetPropertyComboBoxStrings::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskComboBoxStrings);
		FOnGetPropertyComboBoxValue GetMaskValue = FOnGetPropertyComboBoxValue::CreateStatic(&FMaterialPropertyHelpers::GetVectorChannelMaskValue, Parameter);
		FOnPropertyComboBoxValueSelected SetMaskValue = FOnPropertyComboBoxValueSelected::CreateStatic(&FMaterialPropertyHelpers::SetVectorChannelMaskValue, ParameterValueProperty, Parameter, (UObject*)MaterialEditorInstance);

		// Widget replaces color picker with combo box
		FDetailWidgetRow& CustomWidget = PropertyRow.CustomWidget();
		CustomWidget
		.FilterString(ParameterName)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(ParameterName)
			.ToolTipText(FMaterialPropertyHelpers::GetParameterExpressionDescription(Parameter, MaterialEditorInstance))
			.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
		]
		.ValueContent()
		.MaxDesiredWidth(200.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					PropertyCustomizationHelpers::MakePropertyComboBox(ParameterValueProperty, GetMaskStrings, GetMaskValue, SetMaskValue)
				]
			]
		];
	}
}

bool FMaterialInstanceParameterDetails::IsVisibleExpression(UDEditorParameterValue* Parameter)
{
	return MaterialEditorInstance->VisibleExpressions.Contains(Parameter->ParameterInfo);
}

EVisibility FMaterialInstanceParameterDetails::ShouldShowExpression(UDEditorParameterValue* Parameter) const
{
	return FMaterialPropertyHelpers::ShouldShowExpression(Parameter, MaterialEditorInstance, ShowHiddenDelegate);
}

bool FMaterialInstanceParameterDetails::OnShouldSetAsset(const FAssetData& AssetData) const
{
	if (MaterialEditorInstance->bIsFunctionPreviewMaterial)
	{
		if (MaterialEditorInstance->SourceFunction->GetMaterialFunctionUsage() == EMaterialFunctionUsage::Default)
		{
			return false;
		}
		else
		{
			UMaterialFunctionInstance* FunctionInstance = Cast<UMaterialFunctionInstance>(AssetData.GetAsset());
			if (FunctionInstance != nullptr)
			{
				bool bIsChild = FunctionInstance->IsDependent(MaterialEditorInstance->SourceFunction);
				if (bIsChild)
				{
					FMessageDialog::Open(
						EAppMsgType::Ok,
						FText::Format(LOCTEXT("CannotSetExistingChildFunctionAsParent", "Cannot set {0} as a parent as it is already a child of this material function instance."), FText::FromName(AssetData.AssetName)));
				}
				return !bIsChild;
			}
		}
	}

	UMaterialInstance* MaterialInstance = Cast<UMaterialInstance>(AssetData.GetAsset());

	if (MaterialInstance != nullptr)
	{
		bool bIsChild = MaterialInstance->IsChildOf(MaterialEditorInstance->SourceInstance);
		if (bIsChild)
		{
			FMessageDialog::Open(
				EAppMsgType::Ok,
				FText::Format(LOCTEXT("CannotSetExistingChildAsParent", "Cannot set {0} as a parent as it is already a child of this material instance."), FText::FromName(AssetData.AssetName)));
		}
		return !bIsChild;
	}

	return true;
}

void FMaterialInstanceParameterDetails::OnAssetChanged(const FAssetData & InAssetData, TSharedRef<IPropertyHandle> InHandle)
{
	if (MaterialEditorInstance->bIsFunctionPreviewMaterial &&
		MaterialEditorInstance->SourceFunction->GetMaterialFunctionUsage() != EMaterialFunctionUsage::Default)
	{
		UMaterialFunctionInterface* NewParent = Cast<UMaterialFunctionInterface>(InAssetData.GetAsset());
		if (NewParent != nullptr)
		{
			MaterialEditorInstance->SourceFunction->SetParent(NewParent);
			FPropertyChangedEvent ParentChanged = FPropertyChangedEvent(InHandle->GetProperty());
			MaterialEditorInstance->PostEditChangeProperty(ParentChanged);
		}
	}
}

EVisibility FMaterialInstanceParameterDetails::ShouldShowMaterialRefractionSettings() const
{
	return (MaterialEditorInstance->SourceInstance->GetMaterial()->bUsesDistortion && IsTranslucentBlendMode(MaterialEditorInstance->SourceInstance->GetBlendMode())) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility FMaterialInstanceParameterDetails::ShouldShowSubsurfaceProfile() const
{
	EMaterialShadingModel Model = MaterialEditorInstance->SourceInstance->GetShadingModel();

	return (Model == MSM_SubsurfaceProfile) ? EVisibility::Visible : EVisibility::Collapsed;
}


void FMaterialInstanceParameterDetails::CreateBasePropertyOverrideWidgets(IDetailLayoutBuilder& DetailLayout)
{
	IDetailCategoryBuilder& DetailCategory = DetailLayout.EditCategory(NAME_None);
	
	static FName GroupName(TEXT("BasePropertyOverrideGroup"));
	IDetailGroup& BasePropertyOverrideGroup = DetailCategory.AddGroup(GroupName, LOCTEXT("BasePropertyOverrideGroup", "Material Property Overrides"), false, false);

	TAttribute<bool> IsOverrideOpacityClipMaskValueEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::OverrideOpacityClipMaskValueEnabled));
	TAttribute<bool> IsOverrideBlendModeEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::OverrideBlendModeEnabled));
	TAttribute<bool> IsOverrideShadingModelEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::OverrideShadingModelEnabled));
	TAttribute<bool> IsOverrideTwoSidedEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::OverrideTwoSidedEnabled));
	TAttribute<bool> IsOverrideDitheredLODTransitionEnabled = TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::OverrideDitheredLODTransitionEnabled));

	TSharedRef<IPropertyHandle> BasePropertyOverridePropery = DetailLayout.GetProperty("BasePropertyOverrides");
	TSharedPtr<IPropertyHandle> OpacityClipMaskValueProperty = BasePropertyOverridePropery->GetChildHandle("OpacityMaskClipValue");
	TSharedPtr<IPropertyHandle> BlendModeProperty = BasePropertyOverridePropery->GetChildHandle("BlendMode");
	TSharedPtr<IPropertyHandle> ShadingModelProperty = BasePropertyOverridePropery->GetChildHandle("ShadingModel");
	TSharedPtr<IPropertyHandle> TwoSidedProperty = BasePropertyOverridePropery->GetChildHandle("TwoSided");
	TSharedPtr<IPropertyHandle> DitheredLODTransitionProperty = BasePropertyOverridePropery->GetChildHandle("DitheredLODTransition");

	IDetailPropertyRow& OpacityClipMaskValuePropertyRow = BasePropertyOverrideGroup.AddPropertyRow(OpacityClipMaskValueProperty.ToSharedRef());
	OpacityClipMaskValuePropertyRow
		.DisplayName(OpacityClipMaskValueProperty->GetPropertyDisplayName())
		.ToolTip(OpacityClipMaskValueProperty->GetToolTipText())
		.EditCondition(IsOverrideOpacityClipMaskValueEnabled, FOnBooleanValueChanged::CreateSP(this, &FMaterialInstanceParameterDetails::OnOverrideOpacityClipMaskValueChanged));

	IDetailPropertyRow& BlendModePropertyRow = BasePropertyOverrideGroup.AddPropertyRow(BlendModeProperty.ToSharedRef());
	BlendModePropertyRow
		.DisplayName(BlendModeProperty->GetPropertyDisplayName())
		.ToolTip(BlendModeProperty->GetToolTipText())
		.EditCondition(IsOverrideBlendModeEnabled, FOnBooleanValueChanged::CreateSP(this, &FMaterialInstanceParameterDetails::OnOverrideBlendModeChanged));

	IDetailPropertyRow& ShadingModelPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(ShadingModelProperty.ToSharedRef());
	ShadingModelPropertyRow
		.DisplayName(ShadingModelProperty->GetPropertyDisplayName())
		.ToolTip(ShadingModelProperty->GetToolTipText())
		.EditCondition(IsOverrideShadingModelEnabled, FOnBooleanValueChanged::CreateSP(this, &FMaterialInstanceParameterDetails::OnOverrideShadingModelChanged));

	IDetailPropertyRow& TwoSidedPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(TwoSidedProperty.ToSharedRef());
	TwoSidedPropertyRow
		.DisplayName(TwoSidedProperty->GetPropertyDisplayName())
		.ToolTip(TwoSidedProperty->GetToolTipText())
		.EditCondition(IsOverrideTwoSidedEnabled, FOnBooleanValueChanged::CreateSP(this, &FMaterialInstanceParameterDetails::OnOverrideTwoSidedChanged));

	IDetailPropertyRow& DitheredLODTransitionPropertyRow = BasePropertyOverrideGroup.AddPropertyRow(DitheredLODTransitionProperty.ToSharedRef());
	DitheredLODTransitionPropertyRow
		.DisplayName(DitheredLODTransitionProperty->GetPropertyDisplayName())
		.ToolTip(DitheredLODTransitionProperty->GetToolTipText())
		.EditCondition(IsOverrideDitheredLODTransitionEnabled, FOnBooleanValueChanged::CreateSP(this, &FMaterialInstanceParameterDetails::OnOverrideDitheredLODTransitionChanged));
}

bool FMaterialInstanceParameterDetails::OverrideOpacityClipMaskValueEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_OpacityMaskClipValue;
}

bool FMaterialInstanceParameterDetails::OverrideBlendModeEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_BlendMode;
}

bool FMaterialInstanceParameterDetails::OverrideShadingModelEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_ShadingModel;
}

bool FMaterialInstanceParameterDetails::OverrideTwoSidedEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_TwoSided;
}

bool FMaterialInstanceParameterDetails::OverrideDitheredLODTransitionEnabled() const
{
	return MaterialEditorInstance->BasePropertyOverrides.bOverride_DitheredLODTransition;
}

void FMaterialInstanceParameterDetails::OnOverrideOpacityClipMaskValueChanged(bool NewValue)
{
	MaterialEditorInstance->BasePropertyOverrides.bOverride_OpacityMaskClipValue = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FMaterialInstanceParameterDetails::OnOverrideBlendModeChanged(bool NewValue)
{
	MaterialEditorInstance->BasePropertyOverrides.bOverride_BlendMode = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FMaterialInstanceParameterDetails::OnOverrideShadingModelChanged(bool NewValue)
{
	MaterialEditorInstance->BasePropertyOverrides.bOverride_ShadingModel = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FMaterialInstanceParameterDetails::OnOverrideTwoSidedChanged(bool NewValue)
{
	MaterialEditorInstance->BasePropertyOverrides.bOverride_TwoSided = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

void FMaterialInstanceParameterDetails::OnOverrideDitheredLODTransitionChanged(bool NewValue)
{
	MaterialEditorInstance->BasePropertyOverrides.bOverride_DitheredLODTransition = NewValue;
	MaterialEditorInstance->PostEditChange();
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

#undef LOCTEXT_NAMESPACE

