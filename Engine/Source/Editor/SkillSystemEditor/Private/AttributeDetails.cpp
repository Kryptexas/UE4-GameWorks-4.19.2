// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemEditorModulePrivatePCH.h"
#include "AttributeDetails.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "AttributeDetails.h"
#include "AttributeSet.h"
#include "BlueprintUtilities.h"
#include "KismetEditorUtilities.h"

#define LOCTEXT_NAMESPACE "AttributeDetailsCustomization"

// default name for base pose
#define DEFAULT_POSE_NAME	TEXT("Default")

DEFINE_LOG_CATEGORY(LogAttributeDetails);

TSharedRef<IStructCustomization> FAttributePropertyDetails::MakeInstance()
{
	return MakeShareable(new FAttributePropertyDetails());
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FAttributePropertyDetails::CustomizeStructHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	MyProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FGameplayAttribute,Attribute));

	PropertyOptions.Empty();
	PropertyOptions.Add(MakeShareable(new FString("None")));

	// Gather all UAttraibute classes
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass *ItClass = *It;
		if (ItClass->IsChildOf(UAttributeSet::StaticClass()) && !FKismetEditorUtilities::IsClassABlueprintSkeleton(ItClass))
		{
			for( TFieldIterator<UProperty> It(ItClass, EFieldIteratorFlags::ExcludeSuper) ; It ; ++It )
			{
				UProperty *Property = *It;
				PropertyOptions.Add(MakeShareable(new FString(FString::Printf(TEXT("%s.%s"), *ItClass->GetName(), *Property->GetName()))));
			}
		}
	}

	HeaderRow.
		NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		[
			SNew(STextComboBox)
			.OptionsSource( &PropertyOptions )
			.InitiallySelectedItem(GetPropertyType())
			.OnSelectionChanged( this, &FAttributePropertyDetails::OnChangeProperty )
		];
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FAttributePropertyDetails::CustomizeStructChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{


}

TSharedPtr<FString> FAttributePropertyDetails::GetPropertyType() const
{
	if (MyProperty.IsValid())
	{
		UProperty *PropertyValue = NULL;
		UObject *ObjPtr = NULL;
		MyProperty->GetValue(ObjPtr);
		PropertyValue = Cast<UProperty>(ObjPtr);
		if (PropertyValue)
		{
			FString FullString = PropertyValue->GetOuter()->GetName() + TEXT(".") + PropertyValue->GetName();
			for (int32 i=0; i < PropertyOptions.Num(); ++i)
			{
				if (PropertyOptions[i].IsValid() && PropertyOptions[i].Get()->Equals(FullString))
				{
					return PropertyOptions[i];
				}
			}
		}
	}

	return PropertyOptions[0]; // This should always be none
}

void FAttributePropertyDetails::OnChangeProperty(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo)
{
	if (ItemSelected.IsValid() && MyProperty.IsValid())
	{		
		FString FullString = *ItemSelected.Get();
		FString ClassName;
		FString PropertyName;

		FullString.Split( TEXT("."), &ClassName, &PropertyName);

		UClass *FoundClass = FindObject<UClass>(ANY_PACKAGE, *ClassName);
		if (FoundClass)
		{
			UProperty *Property = FindField<UProperty>(FoundClass, *PropertyName);
			if (Property)
			{
				const UObject *ObjPtr = Property;
				MyProperty->SetValue(ObjPtr);
				return;
			}
		}
	}
}

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

TSharedRef<IDetailCustomization> FAttributeDetails::MakeInstance()
{
	return MakeShareable(new FAttributeDetails);
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FAttributeDetails::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	MyProperty = DetailLayout.GetProperty("PropertyReference", UAttributeSet::StaticClass());

	PropertyOptions.Empty();
	PropertyOptions.Add(MakeShareable(new FString("None")));

	for( TFieldIterator<UProperty> It(UAttributeSet::StaticClass(), EFieldIteratorFlags::ExcludeSuper) ; It ; ++It )
	{
		UProperty *Property = *It;
		PropertyOptions.Add(MakeShareable(new FString(Property->GetName())));
	}

	//PropertyOptions.Add(MakeShareable(new FString("Test1")));
	//PropertyOptions.Add(MakeShareable(new FString("Test2")));

	IDetailCategoryBuilder& Category = DetailLayout.EditCategory("Variable", LOCTEXT("VariableDetailsCategory", "Variable").ToString());
	const FSlateFontInfo DetailFontInfo = IDetailLayoutBuilder::GetDetailFont();
	
	Category.AddCustomRow( TEXT("Replication") )
		//.Visibility(TAttribute<EVisibility>(this, &FBlueprintVarActionDetails::ReplicationVisibility))
		.NameContent()
		[
			SNew(STextBlock)
			.ToolTipText(LOCTEXT("PropertyType_Tooltip", "Which Property To Modify?"))
			.Text( LOCTEXT("PropertyModifierInfo", "Property") )
			.Font( DetailFontInfo )
		]
		.ValueContent()
		[
			SNew(STextComboBox)
			.OptionsSource( &PropertyOptions )
			.InitiallySelectedItem(GetPropertyType())
			.OnSelectionChanged( this, &FAttributeDetails::OnChangeProperty )
		];
	
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION


TSharedPtr<FString> FAttributeDetails::GetPropertyType() const
{
	if (!MyProperty.IsValid())
		return PropertyOptions[0];

	UProperty *PropertyValue = NULL;
	UObject *ObjPtr = NULL;
	MyProperty->GetValue(ObjPtr);
	PropertyValue = Cast<UProperty>(ObjPtr);

	if (PropertyValue != NULL)
	{
		for (int32 i=0; i < PropertyOptions.Num(); ++i)
		{
			if (PropertyOptions[i].IsValid() && PropertyOptions[i].Get()->Equals(PropertyValue->GetName()))
			{
				return PropertyOptions[i];
			}
		}
	}

	return PropertyOptions[0]; // This should always be none
}

void FAttributeDetails::OnChangeProperty(TSharedPtr<FString> ItemSelected, ESelectInfo::Type SelectInfo)
{
	if (!ItemSelected.IsValid())
	{
		return;
	}

	FString PropertyName = *ItemSelected.Get();

	for( TFieldIterator<UProperty> It(UAttributeSet::StaticClass(), EFieldIteratorFlags::ExcludeSuper) ; It ; ++It )
	{
		const UProperty *Property = *It;
		if (PropertyName == Property->GetName())
		{
			const UObject *ObjPtr = Property;
			MyProperty->SetValue(ObjPtr);
			return;
		}
	}
}

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

TSharedRef<IStructCustomization> FScalableFloatDetails::MakeInstance()
{
	return MakeShareable(new FScalableFloatDetails());
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FScalableFloatDetails::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 i = 0; i < NumChildren; i++)
	{
		FString PropName = StructPropertyHandle->GetChildHandle(i)->GetPropertyDisplayName();
		PropName;
	}

	ValueProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FScalableFloat,Value));
	CurveTableHandleProperty = StructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FScalableFloat,Curve));

	RowNameProperty = CurveTableHandleProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCurveTableRowHandle, RowName));
	CurveTableProperty = CurveTableHandleProperty->GetChildHandle(GET_MEMBER_NAME_CHECKED(FCurveTableRowHandle, CurveTable));

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MinDesiredWidth( 500 )
		.MaxDesiredWidth( 4096 )
		[
			SNew(SHorizontalBox)
			.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FScalableFloatDetails::IsEditable)))
			+SHorizontalBox::Slot()
			//.FillWidth(1.0f)
			.HAlign(HAlign_Fill)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				ValueProperty->CreatePropertyValueWidget()
			]
		
			+SHorizontalBox::Slot()
			//.FillWidth(1.f)
			.HAlign(HAlign_Fill)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				RowNameProperty->CreatePropertyValueWidget()
			]
			+SHorizontalBox::Slot()
			//.FillWidth(1.f)
			.HAlign(HAlign_Fill)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				CurveTableProperty->CreatePropertyValueWidget()
			]
		];

	/*
	uint32 NumChildren = 0;
	CurveTableHandleProperty ->GetNumChildren(NumChildren);

	for (uint32 i = 0; i < NumChildren; i++)
	{
		FString PropName = LOCTEXT("Value", "Value: ").ToString();
		TSharedPtr<IPropertyHandle> PropHandle = StructPropertyHandle->GetChildHandle(i);
		if (PropHandle->GetProperty())
		{
			StructBuilder.AddChildContent(TEXT("ValueChildren"))
				.NameContent()
				[
					SNew(STextBlock)
					.Text(PropName)
					.Font(StructCustomizationUtils.GetRegularFont())
				]
			.ValueContent()
				[
					PropHandle->CreatePropertyValueWidget()
				];
		}
	}
	


	FString Desc = TEXT("HEllo");

	HeaderRow.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
	.ValueContent()
		.MaxDesiredWidth(400.0f)
		[
			SNew(STextBlock)
			.Text(Desc)
			.Font(StructCustomizationUtils.GetRegularFont())
		];
	*/

}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

bool FScalableFloatDetails::IsEditable( ) const
{
	return true;
}

void FScalableFloatDetails::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	/*
	if (StructPropertyHandle->IsValidHandle())
	{
		StructBuilder.AddChildProperty(ValueProperty.ToSharedRef());
		//StructBuilder.AddChildProperty(CurveTableHandleProperty.ToSharedRef());

		StructBuilder.AddChildProperty(RowNameProperty.ToSharedRef());
		StructBuilder.AddChildProperty(CurveTableProperty.ToSharedRef());
	}
	*/

	/*
	uint32 NumChildren = 0;
	StructPropertyHandle->GetNumChildren(NumChildren);

	for (uint32 i = 0; i < NumChildren; i++)
	{
		FString PropName = LOCTEXT("Value", "Value: ").ToString();
		TSharedPtr<IPropertyHandle> PropHandle = StructPropertyHandle->GetChildHandle(i);
		if (PropHandle->GetProperty())
		{
			StructBuilder.AddChildContent(TEXT("ValueChildren"))
				.NameContent()
				[
					SNew(STextBlock)
					.Text(PropName)
					.Font(StructCustomizationUtils.GetRegularFont())
				]
			.ValueContent()
				[
					PropHandle->CreatePropertyValueWidget()
				];
		}
	}
	*/
}

// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------

template< typename T >
TSharedPtr<FString> InitWidgetContent(TSharedPtr<IPropertyHandle> DataPropertyHandle, TSharedPtr<IPropertyHandle> RowNamePropertyHandle, TArray<TSharedPtr<FString> > &RowNames, FString LookForName)
{
	TSharedPtr<FString> InitialValue = MakeShareable( new FString( TEXT( "None" ) ) );

	FString RowName;
	const FPropertyAccess::Result RowResult = RowNamePropertyHandle->GetValue( RowName );
	RowNames.Empty();

	/** Get the properties we wish to work with */
	T* Table = NULL;
	DataPropertyHandle->GetValue( ( UObject*& )Table );

	if( Table != NULL )
	{
		/** Extract all the row names from the RowMap */
		for (auto It = Table->RowMap.CreateIterator(); It; ++It)
		{
			/** Create a simple array of the row names */
			FString FullRowName = It.Key().ToString();
			
			int32 StartIdx = FullRowName.Find(LookForName);

			if ( StartIdx != INDEX_NONE )
			{
				StartIdx += LookForName.Len() + 1;

				int32 EndIdx = FullRowName.Find(TEXT("."), ESearchCase::IgnoreCase, ESearchDir::FromStart, StartIdx);

				FString ParsedName = FullRowName.Mid(StartIdx, EndIdx - StartIdx);

				TSharedRef<FString> RowNameItem = MakeShareable( new FString( ParsedName ) );
				RowNames.Add( RowNameItem );

				/** Set the initial value to the currently selected item */
				
				if( ParsedName == RowName )
				{
					InitialValue = RowNameItem;
				}
				
			}
		}
	}

	/** Reset the initial value to ensure a valid entry is set */
	
	if ( RowResult != FPropertyAccess::MultipleValues )
	{
		FName NewValue = FName( **InitialValue );
		RowNamePropertyHandle->SetValue( NewValue );
	}
	

	return InitialValue;
}



//-------------------------------------------------------------------------------------

TSharedRef<IStructCustomization> FFlexTableRowHandleDetails::MakeInstance()
{
	return MakeShareable(new FFlexTableRowHandleDetails());
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION
void FFlexTableRowHandleDetails::CustomizeStructHeader( TSharedRef<class IPropertyHandle> InStructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	StructPropertyHandle = InStructPropertyHandle;
	
	DataTablePropertyHandle = StructPropertyHandle->GetChildHandle( "DataTable" );
	BaseStringPropertyHandle = StructPropertyHandle->GetChildHandle( "BaseString" );
	RowStringPropertyHandle = StructPropertyHandle->GetChildHandle( "RowName" );
	
	ParentArrayProperty = RowStringPropertyHandle->GetParentHandle();
	ParentArrayProperty = ParentArrayProperty->GetParentHandle();

	LookForName = *ParentArrayProperty->GetProperty()->GetName();

	// When the data table* changes (this is changed by the owning class
	FSimpleDelegate OnCurveTableChangedDelegate = FSimpleDelegate::CreateSP( this, &FFlexTableRowHandleDetails::OnCurveTableChanged );
	DataTablePropertyHandle->SetOnPropertyValueChanged( OnCurveTableChangedDelegate );

	CurrentSelectedItem = InitWidgetContent<UDataTable>(DataTablePropertyHandle, RowStringPropertyHandle, RowNames, LookForName);

	HeaderRow
		.NameContent()
		[
			InStructPropertyHandle->CreatePropertyNameWidget( TEXT( "" ), false )
		]
		.ValueContent()
		[
			SNew( SVerticalBox )
			.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(this, &FFlexTableRowHandleDetails::IsEditable)))
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SAssignNew( RowNameComboButton, SComboButton )
				.OnGetMenuContent( this, &FFlexTableRowHandleDetails::GetListContent )
				.ContentPadding( FMargin( 2.0f, 2.0f ) )
				.ButtonContent()
				[
					SNew( STextBlock )
					.Text( this, &FFlexTableRowHandleDetails::GetRowNameComboBoxContentText )
				]
			]
		];


}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FFlexTableRowHandleDetails::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{

}


/** Returns the ListView for the ComboButton */
TSharedRef<SWidget> FFlexTableRowHandleDetails::GetListContent()
{
	SAssignNew( RowNameComboListView, SListView<TSharedPtr<FString> > )
		.ListItemsSource( &RowNames )
		.OnSelectionChanged( this, &FFlexTableRowHandleDetails::OnSelectionChanged )
		.OnGenerateRow( this, &FFlexTableRowHandleDetails::HandleRowNameComboBoxGenarateWidget )
		.SelectionMode(ESelectionMode::Single);


	if( CurrentSelectedItem.IsValid() )
	{
		RowNameComboListView->SetSelection(CurrentSelectedItem);
	}

	return SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew( SSearchBox )
			.OnTextChanged( this, &FFlexTableRowHandleDetails::OnFilterTextChanged )
		]
	+SVerticalBox::Slot()
		.FillHeight( 1.f )
		[
			RowNameComboListView.ToSharedRef()
		];				
}

/** Delegate to refresh the drop down when the datatable changes */
void FFlexTableRowHandleDetails::OnCurveTableChanged()
{
	CurrentSelectedItem = InitWidgetContent<UDataTable>(DataTablePropertyHandle, RowStringPropertyHandle, RowNames, LookForName);
	if( RowNameComboListView.IsValid() )
	{
		RowNameComboListView->SetSelection( CurrentSelectedItem );
		RowNameComboListView->RequestListRefresh();
	}
}

void FFlexTableRowHandleDetails::OnFilterTextChanged( const FText& InFilterText )
{
#if 0
	FString CurrentFilterText = InFilterText.ToString();

	FName RowName;
	const FPropertyAccess::Result RowResult = RowNamePropertyHandle->GetValue( RowName );
	RowNames.Empty();

	/** Get the properties we wish to work with */
	UCurveTable* CurveTable = NULL;
	CurveTablePropertyHandle->GetValue( ( UObject*& )CurveTable );


	if( CurveTable != NULL )
	{
		/** Extract all the row names from the RowMap */
		for( TMap<FName, FRichCurve*>::TConstIterator Iterator( CurveTable->RowMap ); Iterator; ++Iterator )
		{
			/** Create a simple array of the row names */
			FString RowString = Iterator.Key().ToString();
			if( CurrentFilterText == TEXT("") || RowString.Contains(CurrentFilterText) )
			{
				TSharedRef<FString> RowNameItem = MakeShareable( new FString( RowString ) );				
				RowNames.Add( RowNameItem );					
			}			
		}
	}		
#endif
	RowNameComboListView->RequestListRefresh();
}

/** Display the current selection */
FString FFlexTableRowHandleDetails::GetRowNameComboBoxContentText( ) const
{
	FString RowName = TEXT( "Multiple Values" );
	const FPropertyAccess::Result RowResult = RowStringPropertyHandle->GetValue( RowName );
	if ( RowResult != FPropertyAccess::MultipleValues )
	{
		TSharedPtr<FString> SelectedRowName = CurrentSelectedItem;
		if ( SelectedRowName.IsValid() )
		{
			RowName = *SelectedRowName;
		}
		else
		{
			RowName = TEXT( "None" );
		}
	}
	return RowName;
}

/** Update the root data on a change of selection */
void FFlexTableRowHandleDetails::OnSelectionChanged( TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo )
{
	if( SelectedItem.IsValid() )
	{
		CurrentSelectedItem = SelectedItem; 
		FName NewValue = FName( **SelectedItem );
		RowStringPropertyHandle->SetValue( NewValue );

		// Close the combo
		RowNameComboButton->SetIsOpen( false );
	}
}

/** Return the representation of the the row names to display */
TSharedRef<ITableRow> FFlexTableRowHandleDetails::HandleRowNameComboBoxGenarateWidget( TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable )
{
	return
		SNew( STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew( STextBlock ).Text( *InItem )
		];
}

void FFlexTableRowHandleDetails::OnTest()
{
	DataTablePropertyHandle->NotifyPostChange();
}

bool FFlexTableRowHandleDetails::IsEditable() const
{
	bool bValidTable = false;
	if (DataTablePropertyHandle.IsValid())
	{
		UObject *Data;
		DataTablePropertyHandle->GetValue(Data);
		bValidTable = (Data != NULL);
	}

	return bValidTable;
}

#undef LOCTEXT_NAMESPACE