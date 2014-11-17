// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#include "SkillSystemEditorModulePrivatePCH.h"
#include "AttributeDetails.h"
#include "Editor/PropertyEditor/Public/PropertyEditing.h"
#include "AttributeDetails.h"
#include "AttributeSet.h"
#include "BlueprintUtilities.h"
#include "KismetEditorUtilities.h"
#include "SkillSystemGlobals.h"
#include "SkillSystemModule.h"

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
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass *Class = *ClassIt;
		if (Class->IsChildOf(UAttributeSet::StaticClass()) && !FKismetEditorUtilities::IsClassABlueprintSkeleton(Class))
		{
			for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::ExcludeSuper); PropertyIt; ++PropertyIt)
			{
				UProperty *Property = *PropertyIt;
				PropertyOptions.Add(MakeShareable(new FString(FString::Printf(TEXT("%s.%s"), *Class->GetName(), *Property->GetName()))));
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

	CurrentSelectedItem = InitWidgetContent();

	FSimpleDelegate OnCurveTableChangedDelegate = FSimpleDelegate::CreateSP(this, &FScalableFloatDetails::OnCurveTableChanged);
	CurveTableProperty->SetOnPropertyValueChanged(OnCurveTableChangedDelegate);

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
			.Padding(2.f, 2.f, 2.f, 2.f)
			[
				SAssignNew(RowNameComboButton, SComboButton)
				.OnGetMenuContent(this, &FScalableFloatDetails::GetListContent)
				.ContentPadding(FMargin(2.0f, 2.0f))
				.ButtonContent()
				[
					SNew(STextBlock)
					.Text(this, &FScalableFloatDetails::GetRowNameComboBoxContentText)
				]
			]
			+SHorizontalBox::Slot()
			//.FillWidth(1.f)
			.HAlign(HAlign_Fill)
			.Padding(0.f, 0.f, 2.f, 0.f)
			[
				CurveTableProperty->CreatePropertyValueWidget()
			]
		];	
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void FScalableFloatDetails::OnCurveTableChanged()
{
	CurrentSelectedItem = InitWidgetContent();
	if (RowNameComboListView.IsValid())
	{
		RowNameComboListView->SetSelection(CurrentSelectedItem);
		RowNameComboListView->RequestListRefresh();
	}
}

TSharedPtr<FString> FScalableFloatDetails::InitWidgetContent()
{
	TSharedPtr<FString> InitialValue = MakeShareable(new FString(TEXT("None")));

	FName RowName;
	const FPropertyAccess::Result RowResult = RowNameProperty->GetValue(RowName);
	RowNames.Empty();
	RowNames.Add(InitialValue);

	/** Get the properties we wish to work with */
	UCurveTable* CurveTable = GetCurveTable();

	if (CurveTable != NULL)
	{
		/** Extract all the row names from the RowMap */
		for (TMap<FName, FRichCurve*>::TConstIterator Iterator(CurveTable->RowMap); Iterator; ++Iterator)
		{
			/** Create a simple array of the row names */
			TSharedRef<FString> RowNameItem = MakeShareable(new FString(Iterator.Key().ToString()));
			RowNames.Add(RowNameItem);

			/** Set the initial value to the currently selected item */
			if (Iterator.Key() == RowName)
			{
				InitialValue = RowNameItem;
			}
		}
	}

	/** Reset the initial value to ensure a valid entry is set */
	if (RowResult != FPropertyAccess::MultipleValues)
	{
		FName NewValue = FName(**InitialValue);
		RowNameProperty->SetValue(NewValue);
	}

	return InitialValue;
}

UCurveTable * FScalableFloatDetails::GetCurveTable()
{
	UCurveTable* CurveTable = NULL;
	CurveTableProperty->GetValue((UObject*&)CurveTable);

	if (CurveTable == NULL)
	{
		CurveTable = ISkillSystemModule::Get().GetSkillSystemGlobals().GetGlobalCurveTable();
	}

	return CurveTable;
}

TSharedRef<SWidget> FScalableFloatDetails::GetListContent()
{
	SAssignNew(RowNameComboListView, SListView<TSharedPtr<FString> >)
		.ListItemsSource(&RowNames)
		.OnSelectionChanged(this, &FScalableFloatDetails::OnSelectionChanged)
		.OnGenerateRow(this, &FScalableFloatDetails::HandleRowNameComboBoxGenarateWidget)
		.SelectionMode(ESelectionMode::Single);

	if (CurrentSelectedItem.IsValid())
	{
		RowNameComboListView->SetSelection(CurrentSelectedItem);
	}

	return SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			SNew(SSearchBox)
			.OnTextChanged(this, &FScalableFloatDetails::OnFilterTextChanged)
		]
		+ SVerticalBox::Slot()
		.FillHeight(1.f)
		[
			RowNameComboListView.ToSharedRef()
		];
}

void FScalableFloatDetails::OnSelectionChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectInfo)
{
	if (SelectedItem.IsValid())
	{
		CurrentSelectedItem = SelectedItem;
		FName NewValue = FName(**SelectedItem);
		RowNameProperty->SetValue(NewValue);

		// Close the combo
		RowNameComboButton->SetIsOpen(false);
	}
}

TSharedRef<ITableRow> FScalableFloatDetails::HandleRowNameComboBoxGenarateWidget(TSharedPtr<FString> InItem, const TSharedRef<STableViewBase>& OwnerTable)
{
	return
		SNew(STableRow<TSharedPtr<FString>>, OwnerTable)
		[
			SNew(STextBlock).Text(*InItem)
		];
}

/** Display the current selection */
FString FScalableFloatDetails::GetRowNameComboBoxContentText() const
{
	FString RowName = TEXT("Multiple Values");
	const FPropertyAccess::Result RowResult = RowNameProperty->GetValue(RowName);
	if (RowResult != FPropertyAccess::MultipleValues)
	{
		TSharedPtr<FString> SelectedRowName = CurrentSelectedItem;
		if (SelectedRowName.IsValid())
		{
			RowName = *SelectedRowName;
		}
		else
		{
			RowName = TEXT("None");
		}
	}
	return RowName;
}

/** Called by Slate when the filter box changes text. */
void FScalableFloatDetails::OnFilterTextChanged(const FText& InFilterText)
{
	FString CurrentFilterText = InFilterText.ToString();

	FName RowName;
	const FPropertyAccess::Result RowResult = RowNameProperty->GetValue(RowName);
	RowNames.Empty();

	/** Get the properties we wish to work with */
	UCurveTable* CurveTable = GetCurveTable();

	if (CurveTable != NULL)
	{
		/** Extract all the row names from the RowMap */
		for (TMap<FName, FRichCurve*>::TConstIterator Iterator(CurveTable->RowMap); Iterator; ++Iterator)
		{
			/** Create a simple array of the row names */
			FString RowString = Iterator.Key().ToString();
			if (CurrentFilterText == TEXT("") || RowString.Contains(CurrentFilterText))
			{
				TSharedRef<FString> RowNameItem = MakeShareable(new FString(RowString));
				RowNames.Add(RowNameItem);
			}
		}
	}
	RowNameComboListView->RequestListRefresh();
}

bool FScalableFloatDetails::IsEditable( ) const
{
	return true;
}

void FScalableFloatDetails::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	
}

//-------------------------------------------------------------------------------------


#undef LOCTEXT_NAMESPACE