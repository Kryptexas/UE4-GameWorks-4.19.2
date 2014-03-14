// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MaterialEditorModule.h"
#include "UnrealEd.h"
#include "PropertyEditing.h"
#include "PropertyCustomizationHelpers.h"
#include "MaterialEditor.h"
#include "MaterialEditorInstanceDetailCustomization.h"
#include "ScopedTransaction.h"
#include "DetailWidgetRow.h"

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

void FMaterialInstanceParameterDetails::CustomizeDetails( IDetailLayoutBuilder& DetailLayout )
{
	FName DefaultCategoryName = NAME_None;
	IDetailCategoryBuilder& DefaultCategory = DetailLayout.EditCategory(DefaultCategoryName);

	// Add/hide properties
	DefaultCategory.AddProperty("PhysMaterial");
	DefaultCategory.AddProperty("Parent");
	DefaultCategory.AddProperty("LightmassSettings");
	DetailLayout.HideProperty("bUseOldStyleMICEditorGroups");
	DetailLayout.HideProperty("ParameterGroups");
	DetailLayout.HideProperty("BasePropertyOverrides");

	IDetailPropertyRow& RefractionDepthBias = DefaultCategory.AddProperty("RefractionDepthBias");
	RefractionDepthBias.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::ShouldShowMaterialRefractionSettings)));

	// Create a new category for a custom layout for the MIC parameters
	FName GroupsCategoryName = TEXT("ParameterGroups");
	IDetailCategoryBuilder& GroupsCategory = DetailLayout.EditCategory(GroupsCategoryName, NSLOCTEXT("MaterialEditor", "MICParamGroupsTitle", "Parameter Groups").ToString());
	TSharedRef<IPropertyHandle> ParameterGroupsProperty = DetailLayout.GetProperty("ParameterGroups");

	CreateGroupsWidget(ParameterGroupsProperty, GroupsCategory);

	//////////////////////////////////////////////////////////////////////////
	DetailLayout.HideProperty("BasePropertyOverrides");
	IDetailCategoryBuilder& MaterialCategory = DetailLayout.EditCategory(TEXT("MaterialOverrides"), NSLOCTEXT("MaterialEditor", "MICMaterialOverridesTitle", "Material Overrides").ToString());
	MaterialCategory.AddProperty("bOverrideBaseProperties");
	MaterialCategory.AddProperty("BasePropertyOverrides");
}

void FMaterialInstanceParameterDetails::CreateGroupsWidget(TSharedRef<IPropertyHandle> ParameterGroupsProperty, IDetailCategoryBuilder& GroupsCategory)
{
	check(MaterialEditorInstance);

	for (int32 GroupIdx = 0; GroupIdx < MaterialEditorInstance->ParameterGroups.Num(); ++GroupIdx)
	{
		FEditorParameterGroup& ParameterGroup = MaterialEditorInstance->ParameterGroups[GroupIdx];

		IDetailGroup& DetailGroup = GroupsCategory.AddGroup( ParameterGroup.GroupName, ParameterGroup.GroupName.ToString() );

		CreateSingleGroupWidget( ParameterGroup, ParameterGroupsProperty->GetChildHandle(GroupIdx), DetailGroup );
	}
}

void FMaterialInstanceParameterDetails::CreateSingleGroupWidget(FEditorParameterGroup& ParameterGroup, TSharedPtr<IPropertyHandle> ParameterGroupProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> ParametersArrayProperty = ParameterGroupProperty->GetChildHandle("Parameters");

	// Create a custom widget for each parameter in the group
	for (int32 ParamIdx = 0; ParamIdx < ParameterGroup.Parameters.Num(); ++ParamIdx)
	{
		TSharedPtr<IPropertyHandle> ParameterProperty = ParametersArrayProperty->GetChildHandle(ParamIdx);

		FString ParameterName = ParameterGroup.Parameters[ParamIdx]->ParameterName.ToString();
		
		UDEditorParameterValue* Parameter = ParameterGroup.Parameters[ParamIdx];
		UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
		UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
		UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
		UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
		UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
		UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);

		if (ScalarParam || SwitchParam || TextureParam || VectorParam)
		{
			CreateParameterValueWidget(Parameter, ParameterProperty, DetailGroup );
		}
		else if (CompMaskParam)
		{
			CreateMaskParameterValueWidget(Parameter, ParameterProperty, DetailGroup );
		}
		else if (FontParam)
		{
			CreateFontParameterValueWidget(Parameter, ParameterProperty, DetailGroup);
		}
		else
		{
			// Unsupported parameter type
			check(false);
		}
	}
}

void FMaterialInstanceParameterDetails::CreateParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");

	if( ParameterValueProperty->IsValidHandle() )
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create( TAttribute<bool>::FGetter::CreateSP( this, &FMaterialInstanceParameterDetails::IsOverriddenExpression, Parameter ) ) ;

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow( ParameterValueProperty.ToSharedRef() );

		PropertyRow
		.DisplayName( Parameter->ParameterName.ToString() )
		.ToolTip( GetParameterExpressionDescription(Parameter) )
		.EditCondition( IsParamEnabled, FOnBooleanValueChanged::CreateSP( this, &FMaterialInstanceParameterDetails::OnOverrideParameter, Parameter ) )
		.Visibility( TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::ShouldShowExpression, Parameter)) )
		// Handle reset to default manually
		.OverrideResetToDefault( true, FSimpleDelegate::CreateSP( this, &FMaterialInstanceParameterDetails::ResetToDefault, Parameter ) );
	}
}

void FMaterialInstanceParameterDetails::CreateMaskParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("ParameterValue");
	TSharedPtr<IPropertyHandle> RMaskProperty = ParameterValueProperty->GetChildHandle("R");
	TSharedPtr<IPropertyHandle> GMaskProperty = ParameterValueProperty->GetChildHandle("G");
	TSharedPtr<IPropertyHandle> BMaskProperty = ParameterValueProperty->GetChildHandle("B");
	TSharedPtr<IPropertyHandle> AMaskProperty = ParameterValueProperty->GetChildHandle("A");

	if( ParameterValueProperty->IsValidHandle() )
	{
		TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create( TAttribute<bool>::FGetter::CreateSP( this, &FMaterialInstanceParameterDetails::IsOverriddenExpression, Parameter ) ) ;

		IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow( ParameterValueProperty.ToSharedRef() );
		PropertyRow.EditCondition( IsParamEnabled, FOnBooleanValueChanged::CreateSP( this, &FMaterialInstanceParameterDetails::OnOverrideParameter, Parameter ) );
		// Handle reset to default manually
		PropertyRow.OverrideResetToDefault( true, FSimpleDelegate::CreateSP( this, &FMaterialInstanceParameterDetails::ResetToDefault, Parameter ) );
		PropertyRow.Visibility( TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::ShouldShowExpression, Parameter)) );

		const FString ParameterName = Parameter->ParameterName.ToString(); 

		FDetailWidgetRow& CustomWidget = PropertyRow.CustomWidget();
		CustomWidget
		.FilterString( ParameterName )
		.NameContent()
		[
			SNew(STextBlock)
			.Text( ParameterName )
			.ToolTipText( GetParameterExpressionDescription(Parameter) )
			.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
		]
		.ValueContent()
		.MaxDesiredWidth(200.0f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					RMaskProperty->CreatePropertyNameWidget( TEXT(""), false )
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					RMaskProperty->CreatePropertyValueWidget()
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.Padding( FMargin( 10.0f, 0.0f, 0.0f, 0.0f ) )
				.AutoWidth()
				[
					GMaskProperty->CreatePropertyNameWidget( TEXT(""), false )
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					GMaskProperty->CreatePropertyValueWidget()
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.Padding( FMargin( 10.0f, 0.0f, 0.0f, 0.0f ) )
				.AutoWidth()
				[
					BMaskProperty->CreatePropertyNameWidget( TEXT(""), false )
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					BMaskProperty->CreatePropertyValueWidget()
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.Padding( FMargin( 10.0f, 0.0f, 0.0f, 0.0f ) )
				.AutoWidth()
				[
					AMaskProperty->CreatePropertyNameWidget( TEXT(""), false )
				]
				+SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.AutoWidth()
				[
					AMaskProperty->CreatePropertyValueWidget()
				]
			]
		];
	}

	
}

void FMaterialInstanceParameterDetails::CreateFontParameterValueWidget(UDEditorParameterValue* Parameter, TSharedPtr<IPropertyHandle> ParameterProperty, IDetailGroup& DetailGroup )
{
	TSharedPtr<IPropertyHandle> FontPageProperty = ParameterProperty->GetChildHandle("FontPage");
	TSharedPtr<IPropertyHandle> ParameterValueProperty = ParameterProperty->GetChildHandle("FontValue");

	TAttribute<bool> IsParamEnabled = TAttribute<bool>::Create( TAttribute<bool>::FGetter::CreateSP( this, &FMaterialInstanceParameterDetails::IsOverriddenExpression, Parameter ) ) ;

	IDetailPropertyRow& PropertyRow = DetailGroup.AddPropertyRow( ParameterProperty.ToSharedRef() );
	PropertyRow.IsEnabled( IsParamEnabled );
	// Handle reset to default manually
	PropertyRow.OverrideResetToDefault( true, FSimpleDelegate::CreateSP( this, &FMaterialInstanceParameterDetails::ResetToDefault, Parameter ) );
	PropertyRow.Visibility( TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateSP(this, &FMaterialInstanceParameterDetails::ShouldShowExpression, Parameter)) );

	FDetailWidgetRow& CustomWidget = PropertyRow.CustomWidget();

	CustomWidget
	.FilterString( Parameter->ParameterName.ToString() )
	.NameContent()
	[
		SNew(STextBlock)
		.Text( NSLOCTEXT("FMaterialInstanceParameterDetails", "FontPageLabel", "Page") )
		.ToolTipText( GetParameterExpressionDescription(Parameter) )
		.Font(FEditorStyle::GetFontStyle(TEXT("PropertyWindow.NormalFont")))
	]
	.ValueContent()
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.Padding(0.0f, 0.0f, 2.0f, 0.0f)
		[
			FontPageProperty->CreatePropertyValueWidget()
		]
		+SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			ParameterValueProperty->CreatePropertyNameWidget()
		]
	];

}

bool FMaterialInstanceParameterDetails::IsVisibleExpression(UDEditorParameterValue* Parameter)
{
	return MaterialEditorInstance->VisibleExpressions.Contains(Parameter->ExpressionId);
}

EVisibility FMaterialInstanceParameterDetails::ShouldShowExpression(UDEditorParameterValue* Parameter) const
{
	bool bShowHidden = true;

	ShowHiddenDelegate.ExecuteIfBound(bShowHidden);

	return (bShowHidden || MaterialEditorInstance->VisibleExpressions.Contains(Parameter->ExpressionId))? EVisibility::Visible: EVisibility::Collapsed;
}

bool FMaterialInstanceParameterDetails::IsOverriddenExpression(UDEditorParameterValue* Parameter)
{
	return Parameter->bOverride != 0;
}

void FMaterialInstanceParameterDetails::OnOverrideParameter(bool NewValue, class UDEditorParameterValue* Parameter)
{
	const FScopedTransaction Transaction( NSLOCTEXT( "MaterialInstanceEditor", "OverrideParameter", "Override Parameter" ) );
	Parameter->Modify();
	Parameter->bOverride = NewValue;

	// Fire off a dummy event to the material editor instance, so it knows to update the material, then refresh the viewports.
	FPropertyChangedEvent OverrideEvent(NULL);
	MaterialEditorInstance->PostEditChangeProperty( OverrideEvent );
	FEditorSupportDelegates::RedrawAllViewports.Broadcast();
}

FString FMaterialInstanceParameterDetails::GetParameterExpressionDescription(UDEditorParameterValue* Parameter) const
{
	UMaterial* BaseMaterial = MaterialEditorInstance->SourceInstance->GetMaterial();
	if ( BaseMaterial )
	{
		UMaterialExpressionParameter* Expression = BaseMaterial->FindExpressionByGUID<UMaterialExpressionParameter>( Parameter->ExpressionId );
		if ( Expression )
		{
			return Expression->Desc;
		}

		UMaterialExpressionTextureSampleParameter* TextureExpression = BaseMaterial->FindExpressionByGUID<UMaterialExpressionTextureSampleParameter>( Parameter->ExpressionId );
		if ( TextureExpression )
		{
			return TextureExpression->Desc;
		}

		UMaterialExpressionFontSampleParameter* FontExpression = BaseMaterial->FindExpressionByGUID<UMaterialExpressionFontSampleParameter>( Parameter->ExpressionId );
		if ( FontExpression )
		{
			return FontExpression->Desc;
		}
	}

	return "";
}

void FMaterialInstanceParameterDetails::ResetToDefault( class UDEditorParameterValue* Parameter )
{
	const FScopedTransaction Transaction( NSLOCTEXT( "MaterialInstanceEditor", "ResetToDefault", "Reset To Default" ) );
	Parameter->Modify();
	FName ParameterName = Parameter->ParameterName;
	
	UDEditorFontParameterValue* FontParam = Cast<UDEditorFontParameterValue>(Parameter);
	UDEditorScalarParameterValue* ScalarParam = Cast<UDEditorScalarParameterValue>(Parameter);
	UDEditorStaticComponentMaskParameterValue* CompMaskParam = Cast<UDEditorStaticComponentMaskParameterValue>(Parameter);
	UDEditorStaticSwitchParameterValue* SwitchParam = Cast<UDEditorStaticSwitchParameterValue>(Parameter);
	UDEditorTextureParameterValue* TextureParam = Cast<UDEditorTextureParameterValue>(Parameter);
	UDEditorVectorParameterValue* VectorParam = Cast<UDEditorVectorParameterValue>(Parameter);

	if (ScalarParam)
	{
		float OutValue;
		if (MaterialEditorInstance->Parent->GetScalarParameterValue(ParameterName, OutValue))
		{
			ScalarParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (FontParam)
	{
		UFont* OutFontValue;
		int32 OutFontPage;
		if (MaterialEditorInstance->Parent->GetFontParameterValue(ParameterName, OutFontValue,OutFontPage))
		{
			FontParam->FontValue = OutFontValue;
			FontParam->FontPage = OutFontPage;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (TextureParam)
	{
		UTexture* OutValue;
		if (MaterialEditorInstance->Parent->GetTextureParameterValue(ParameterName, OutValue))
		{
			TextureParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (VectorParam)
	{
		FLinearColor OutValue;
		if (MaterialEditorInstance->Parent->GetVectorParameterValue(ParameterName, OutValue))
		{
			VectorParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (SwitchParam)
	{
		bool OutValue;
		FGuid TempGuid(0,0,0,0);
		if (MaterialEditorInstance->Parent->GetStaticSwitchParameterValue(ParameterName, OutValue, TempGuid))
		{
			SwitchParam->ParameterValue = OutValue;
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
	else if (CompMaskParam)
	{
		bool OutValue[4];
		FGuid TempGuid(0,0,0,0);
		if (MaterialEditorInstance->Parent->GetStaticComponentMaskParameterValue(ParameterName, OutValue[0], OutValue[1], OutValue[2], OutValue[3], TempGuid))
		{
			CompMaskParam->ParameterValue.R = OutValue[0];
			CompMaskParam->ParameterValue.G = OutValue[1];
			CompMaskParam->ParameterValue.B = OutValue[2];
			CompMaskParam->ParameterValue.A = OutValue[3];
			MaterialEditorInstance->CopyToSourceInstance();
		}
	}
}

EVisibility FMaterialInstanceParameterDetails::ShouldShowMaterialRefractionSettings() const
{
	return (MaterialEditorInstance->SourceInstance->GetMaterial()->bUsesDistortion) ? EVisibility::Visible : EVisibility::Collapsed;
}
