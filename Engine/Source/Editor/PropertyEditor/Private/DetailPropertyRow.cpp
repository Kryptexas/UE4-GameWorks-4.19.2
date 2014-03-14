// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PropertyEditorPrivatePCH.h"
#include "DetailPropertyRow.h"
#include "PropertyHandleImpl.h"
#include "SResetToDefaultPropertyEditor.h"
#include "DetailItemNode.h"
#include "DetailCustomBuilderRow.h"
#include "DetailGroup.h"
#include "CustomChildBuilder.h"

FDetailPropertyRow::FDetailPropertyRow(TSharedPtr<FPropertyNode> InPropertyNode, TSharedRef<FDetailCategoryImpl> InParentCategory, TSharedPtr<FObjectPropertyNode> InExternalRootNode )
	: PropertyNode( InPropertyNode )
	, CustomIsEnabledAttrib( true )
	, ParentCategory( InParentCategory )
	, ExternalRootNode( InExternalRootNode )
	, bShowPropertyButtons( true )
	, bShowCustomPropertyChildren( true )
{
	if( InPropertyNode.IsValid() )
	{
		TSharedRef<FPropertyNode> PropertyNodeRef = PropertyNode.ToSharedRef();

		UProperty* Property = PropertyNodeRef->GetProperty();

		PropertyHandle = InParentCategory->GetParentLayoutImpl().GetPropertyHandle(PropertyNodeRef);

		if (PropertyNode->AsCategoryNode() == NULL)
		{
			MakePropertyEditor(InParentCategory->GetParentLayoutImpl().GetPropertyUtilities());
		}

		UStructProperty* StructProperty = Cast<UStructProperty>(Property);
		// Check if the property is valid for struct customization.  Note: Static arrays of structs will be a UStructProperty with array elements as children
		if (!PropertyEditorHelpers::IsStaticArray(*PropertyNodeRef) && StructProperty && StructProperty->Struct)
		{
			FPropertyEditorModule& PropertyEditorModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			FOnGetStructCustomizationInstance StructLayoutInstanceDelegate = PropertyEditorModule.GetStructCustomizaton(StructProperty->Struct->GetFName());
			if (StructLayoutInstanceDelegate.IsBound())
			{
				if (PropertyHandle->IsValidHandle())
				{
					CustomStructInterface = StructLayoutInstanceDelegate.Execute();
				}
			}
		}
	}
}

IDetailPropertyRow& FDetailPropertyRow::DisplayName( const FString& InDisplayName )
{
	if (PropertyNode.IsValid())
	{
		PropertyNode->SetDisplayNameOverride( InDisplayName );
	}
	return *this;
}

IDetailPropertyRow& FDetailPropertyRow::ToolTip( const FString& InToolTip )
{
	if (PropertyNode.IsValid())
	{
		PropertyNode->SetToolTipOverride( InToolTip );
	}
	return *this;
}

IDetailPropertyRow& FDetailPropertyRow::ShowPropertyButtons( bool bInShowPropertyButtons )
{
	bShowPropertyButtons = bInShowPropertyButtons;
	return *this;
}

IDetailPropertyRow& FDetailPropertyRow::EditCondition( TAttribute<bool> EditConditionValue, FOnBooleanValueChanged OnEditConditionValueChanged )
{
	CustomEditCondition = MakeShareable( new FCustomEditCondition );

	CustomEditCondition->EditConditionValue = EditConditionValue;
	CustomEditCondition->OnEditConditionValueChanged = OnEditConditionValueChanged;
	return *this;
}

IDetailPropertyRow& FDetailPropertyRow::IsEnabled( TAttribute<bool> InIsEnabled )
{
	CustomIsEnabledAttrib = InIsEnabled;
	return *this;
}

IDetailPropertyRow& FDetailPropertyRow::Visibility( TAttribute<EVisibility> Visibility )
{
	PropertyVisibility = Visibility;
	return *this;
}

IDetailPropertyRow& FDetailPropertyRow::OverrideResetToDefault( TAttribute<bool> IsResetToDefaultVisible, FSimpleDelegate OnResetToDefaultClicked )
{
	CustomResetToDefault = MakeShareable( new FCustomResetToDefault );

	CustomResetToDefault->IsResetToDefaultVisible = IsResetToDefaultVisible;
	CustomResetToDefault->OnResetToDefaultClicked = OnResetToDefaultClicked;

	return *this;
}

void FDetailPropertyRow::GetDefaultWidgets( TSharedPtr<SWidget>& OutNameWidget, TSharedPtr<SWidget>& OutValueWidget ) 
{
	FDetailWidgetRow Row;
	GetDefaultWidgets(OutNameWidget, OutValueWidget, Row);
}

void FDetailPropertyRow::GetDefaultWidgets( TSharedPtr<SWidget>& OutNameWidget, TSharedPtr<SWidget>& OutValueWidget, FDetailWidgetRow& Row )
{
	TSharedPtr<FDetailWidgetRow> CustomStructRow;
	if ( CustomStructInterface.IsValid() ) 
	{
		CustomStructRow = MakeShareable(new FDetailWidgetRow);

		CustomStructInterface->CustomizeStructHeader(PropertyHandle.ToSharedRef(), *CustomStructRow, *this);
	}

	const bool bAddWidgetDecoration = false;
	MakeNameWidget(Row,CustomStructRow);
	MakeValueWidget(Row,CustomStructRow,bAddWidgetDecoration);

	OutNameWidget = Row.NameWidget.Widget;
	OutValueWidget = Row.ValueWidget.Widget;
}


bool FDetailPropertyRow::HasColumns() const
{
	// Regular properties always have columns
	return !CustomPropertyWidget.IsValid() || CustomPropertyWidget->HasColumns();
}

bool FDetailPropertyRow::ShowOnlyChildren() const
{
	return StructLayoutBuilder.IsValid() && CustomPropertyWidget.IsValid() && !CustomPropertyWidget->HasAnyContent();
}

bool FDetailPropertyRow::RequiresTick() const
{
	return PropertyVisibility.IsBound();
}

FDetailWidgetRow& FDetailPropertyRow::CustomWidget( bool bShowChildren )
{
	bShowCustomPropertyChildren = bShowChildren;
	CustomPropertyWidget = MakeShareable( new FDetailWidgetRow );
	return *CustomPropertyWidget;
}

TSharedPtr<FAssetThumbnailPool> FDetailPropertyRow::GetThumbnailPool() const
{
	TSharedPtr<FDetailCategoryImpl> ParentCategoryPinned = ParentCategory.Pin();
	return ParentCategoryPinned.IsValid() ? ParentCategoryPinned->GetParentLayout().GetThumbnailPool() : NULL;
}

FDetailWidgetRow FDetailPropertyRow::GetWidgetRow()
{
	if( HasColumns() )
	{
		FDetailWidgetRow Row;
	
		MakeNameWidget( Row, CustomPropertyWidget );
		MakeValueWidget( Row, CustomPropertyWidget );

		return Row;
	}
	else
	{
		return *CustomPropertyWidget;
	}
}

void FDetailPropertyRow::OnItemNodeInitialized( TSharedRef<FDetailCategoryImpl> InParentCategory, const TAttribute<bool>& InIsParentEnabled )
{
	IsParentEnabled = InIsParentEnabled;

	// Don't customize the user already customized
	if( !CustomPropertyWidget.IsValid() && CustomStructInterface.IsValid() )
	{
		CustomPropertyWidget = MakeShareable(new FDetailWidgetRow);

		CustomStructInterface->CustomizeStructHeader(PropertyHandle.ToSharedRef(), *CustomPropertyWidget, *this);
	}

	if( bShowCustomPropertyChildren && CustomStructInterface.IsValid() )
	{
		StructLayoutBuilder = MakeShareable(new FCustomChildrenBuilder(InParentCategory));
		CustomStructInterface->CustomizeStructChildren(PropertyHandle.ToSharedRef(), *StructLayoutBuilder, *this);
	}
}

void FDetailPropertyRow::OnGenerateChildren( FDetailNodeList& OutChildren )
{
	if( PropertyNode->AsCategoryNode() || PropertyNode->GetProperty() )
	{
		GenerateChildrenForPropertyNode( PropertyNode, OutChildren );
	}
}

void FDetailPropertyRow::GenerateChildrenForPropertyNode( TSharedPtr<FPropertyNode>& RootPropertyNode, FDetailNodeList& OutChildren )
{
	// Children should be disabled if we are disabled
	TAttribute<bool> ParentEnabledState = CustomIsEnabledAttrib;
	if( IsParentEnabled.IsBound() || HasEditCondition() )
	{
		// Bind a delegate to the edit condition so our children will be disabled if the edit condition fails
		ParentEnabledState.Bind( this, &FDetailPropertyRow::GetEnabledState );
	}

	if( StructLayoutBuilder.IsValid() && bShowCustomPropertyChildren )
	{
		const TArray< FDetailLayoutCustomization >& ChildRows = StructLayoutBuilder->GetChildCustomizations();

		for( int32 ChildIndex = 0; ChildIndex < ChildRows.Num(); ++ChildIndex )
		{
			TSharedRef<FDetailItemNode> ChildNodeItem = MakeShareable( new FDetailItemNode( ChildRows[ChildIndex], ParentCategory.Pin().ToSharedRef(), ParentEnabledState ) );
			ChildNodeItem->Initialize();
			OutChildren.Add( ChildNodeItem );
		}
	}
	else if (bShowCustomPropertyChildren || !CustomPropertyWidget.IsValid() )
	{
		for( int32 ChildIndex = 0; ChildIndex < RootPropertyNode->GetNumChildNodes(); ++ChildIndex )
		{
			TSharedPtr<FPropertyNode> ChildNode = RootPropertyNode->GetChildNode(ChildIndex);

			if( ChildNode.IsValid() && ChildNode->HasNodeFlags( EPropertyNodeFlags::IsCustomized ) == 0 )
			{
				if( ChildNode->AsObjectNode() )
				{
					// Skip over object nodes and generate their children.  Object nodes are not visible
					GenerateChildrenForPropertyNode( ChildNode, OutChildren );
				}
				else
				{
					FDetailLayoutCustomization Customization;
					Customization.PropertyRow = MakeShareable( new FDetailPropertyRow( ChildNode, ParentCategory.Pin().ToSharedRef() ) );
					TSharedRef<FDetailItemNode> ChildNodeItem = MakeShareable( new FDetailItemNode( Customization, ParentCategory.Pin().ToSharedRef(), ParentEnabledState ) );
					ChildNodeItem->Initialize();
					OutChildren.Add( ChildNodeItem );
				}
			}
		}
	}
}


TSharedRef<FPropertyEditor> FDetailPropertyRow::MakePropertyEditor( const TSharedRef<IPropertyUtilities>& PropertyUtilities )
{
	if( !PropertyEditor.IsValid() )
	{
		PropertyEditor = FPropertyEditor::Create( PropertyNode.ToSharedRef(), PropertyUtilities );
	}

	return PropertyEditor.ToSharedRef();
}

bool FDetailPropertyRow::HasEditCondition() const
{
	return ( PropertyEditor.IsValid() && PropertyEditor->HasEditCondition() ) || CustomEditCondition.IsValid();
}

bool FDetailPropertyRow::GetEnabledState() const
{
	if( IsParentEnabled.IsBound() )
	{
		return IsParentEnabled.Get();
	}
	else if( HasEditCondition() ) 
	{
		if( PropertyEditor.IsValid() && PropertyEditor->HasEditCondition() )
		{
			return PropertyEditor->IsEditConditionMet();
		}
		else
		{
			return CustomEditCondition->EditConditionValue.Get();
		}
	}
	else
	{
		return CustomIsEnabledAttrib.Get();
	}
}

void FDetailPropertyRow::MakeNameWidget( FDetailWidgetRow& Row, const TSharedPtr<FDetailWidgetRow> InCustomRow ) const
{
	EVerticalAlignment VerticalAlignment = VAlign_Center;
	EHorizontalAlignment HorizontalAlignment = HAlign_Fill;

	float MinWidth = 0.0f;
	float MaxWidth = 0.0f;

	if( InCustomRow.IsValid() )
	{
		VerticalAlignment = InCustomRow->NameWidget.VerticalAlignment;
		HorizontalAlignment = InCustomRow->NameWidget.HorizontalAlignment;

		MinWidth = InCustomRow->NameWidget.MinWidth;
		MaxWidth = InCustomRow->NameWidget.MaxWidth;
	}

	TAttribute<bool> IsEnabledAttrib = CustomIsEnabledAttrib;

	TSharedRef<SHorizontalBox> NameHorizontalBox = SNew( SHorizontalBox );
	
	if( HasEditCondition() )
	{
		IsEnabledAttrib.Bind( this, &FDetailPropertyRow::GetEnabledState );

		NameHorizontalBox->AddSlot()
		.AutoWidth()
		.Padding( 0.0f, 0.0f )
		.VAlign(VAlign_Center)
		[
			SNew( SEditConditionWidget, PropertyEditor )
			.CustomEditCondition( CustomEditCondition.IsValid() ? *CustomEditCondition : FCustomEditCondition() )
		];
	}

	TSharedPtr<SWidget> NameWidget;
	if( InCustomRow.IsValid() )
	{
		NameWidget = 
			SNew( SBox )
			.IsEnabled( IsEnabledAttrib )
			[
				InCustomRow->NameWidget.Widget
			];
	}
	else
	{
		NameWidget = 
			SNew( SPropertyNameWidget, PropertyEditor )
			.IsEnabled( IsEnabledAttrib )
			.DisplayResetToDefault( false );
	}

	NameHorizontalBox->AddSlot()
	.AutoWidth()
	[
		NameWidget.ToSharedRef()
	];

	Row.NameContent()
	.HAlign( HorizontalAlignment )
	.VAlign( VerticalAlignment )
	[
		NameHorizontalBox
	];
}

void FDetailPropertyRow::MakeValueWidget( FDetailWidgetRow& Row, const TSharedPtr<FDetailWidgetRow> InCustomRow, bool bAddWidgetDecoration ) const
{
	EVerticalAlignment VerticalAlignment = VAlign_Center;
	EHorizontalAlignment HorizontalAlignment = HAlign_Left;

	float MinWidth = 0.0f;
	float MaxWidth = 0.0f;

	if( InCustomRow.IsValid() )
	{
		VerticalAlignment = InCustomRow->ValueWidget.VerticalAlignment;
		HorizontalAlignment = InCustomRow->ValueWidget.HorizontalAlignment;
	}

	TAttribute<bool> IsEnabledAttrib = CustomIsEnabledAttrib;
	if( HasEditCondition() )
	{
		IsEnabledAttrib.Bind( this, &FDetailPropertyRow::GetEnabledState );
	}

	TSharedRef<SHorizontalBox> ValueWidget = 
		SNew( SHorizontalBox )
		.IsEnabled( IsEnabledAttrib );

	if( InCustomRow.IsValid() )
	{
		MinWidth = InCustomRow->ValueWidget.MinWidth;
		MaxWidth = InCustomRow->ValueWidget.MaxWidth;

		ValueWidget->AddSlot()
		[
			InCustomRow->ValueWidget.Widget
		];
	}
	else
	{
		TSharedPtr<SPropertyValueWidget> PropertyValue;

		ValueWidget->AddSlot()
		[
			SAssignNew( PropertyValue, SPropertyValueWidget, PropertyEditor, ParentCategory.Pin()->GetParentLayoutImpl().GetPropertyUtilities() )
			.ShowPropertyButtons( false ) // We handle this ourselves
		];

		MinWidth = PropertyValue->GetMinDesiredWidth();
		MaxWidth = PropertyValue->GetMaxDesiredWidth();
	}

	if(bAddWidgetDecoration)
	{
		if( bShowPropertyButtons )
		{
			TArray< TSharedRef<SWidget> > RequiredButtons;
			PropertyEditorHelpers::MakeRequiredPropertyButtons( PropertyEditor.ToSharedRef(), /*OUT*/RequiredButtons );

			for( int32 ButtonIndex = 0; ButtonIndex < RequiredButtons.Num(); ++ButtonIndex )
			{
				ValueWidget->AddSlot()
				.AutoWidth()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding( 2.0f, 1.0f )
				[ 
					RequiredButtons[ButtonIndex]
				];
			}
		}

		ValueWidget->AddSlot()
		.Padding( 2.0f, 0.0f )
		.AutoWidth()
		.VAlign(VAlign_Center)
		.HAlign(HAlign_Left)
		[
			SNew( SResetToDefaultPropertyEditor, PropertyEditor.ToSharedRef() )
			.IsEnabled( IsEnabledAttrib )
			.CustomResetToDefault( CustomResetToDefault.IsValid() ? *CustomResetToDefault : FCustomResetToDefault() )
		];
	}

	Row.ValueContent()
	.HAlign( HorizontalAlignment )
	.VAlign( VerticalAlignment )	
	.MinDesiredWidth( MinWidth )
	.MaxDesiredWidth( MaxWidth )
	[
		ValueWidget
	];
}
