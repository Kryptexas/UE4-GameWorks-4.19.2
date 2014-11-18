// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "DetailCustomizationsPrivatePCH.h"
#include "DateTimeStructCustomization.h"


#define LOCTEXT_NAMESPACE "DateTimeStructCustomization"


/* IDetailCustomization interface
 *****************************************************************************/

void FDateTimeStructCustomization::CustomizeChildren( TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	/* do nothing */
}


void FDateTimeStructCustomization::CustomizeHeader( TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils )
{
	PropertyHandle = StructPropertyHandle;

	HeaderRow
		.NameContent()
		[
			StructPropertyHandle->CreatePropertyNameWidget()
		]
		.ValueContent()
		.MaxDesiredWidth(0.0f)
		.MinDesiredWidth(125.0f)
		[
			SNew(SEditableTextBox)
				.ClearKeyboardFocusOnCommit(false)
				.IsEnabled(!PropertyHandle->IsEditConst())
				.ForegroundColor(this, &FDateTimeStructCustomization::HandleTextBoxForegroundColor)
				.OnTextChanged(this, &FDateTimeStructCustomization::HandleTextBoxTextChanged)
				.OnTextCommitted(this, &FDateTimeStructCustomization::HandleTextBoxTextCommited)
				.SelectAllTextOnCommit(true)
				.Text(this, &FDateTimeStructCustomization::HandleTextBoxText)
		];
}


/* FDateTimeStructCustomization callbacks
 *****************************************************************************/

FSlateColor FDateTimeStructCustomization::HandleTextBoxForegroundColor( ) const
{
	if (InputValid)
	{
		static const FName InvertedForegroundName("InvertedForeground");
		return FEditorStyle::GetSlateColor(InvertedForegroundName);
	}

	return FLinearColor::Red;
}


FText FDateTimeStructCustomization::HandleTextBoxText( ) const
{
	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	
	if (RawData.Num() != 1)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}

	return FText::FromString(((FDateTime*)RawData[0])->ToString());
}


void FDateTimeStructCustomization::HandleTextBoxTextChanged( const FText& NewText )
{
	FDateTime DateTime;
	InputValid = FDateTime::Parse(NewText.ToString(), DateTime);
}


void FDateTimeStructCustomization::HandleTextBoxTextCommited( const FText& NewText, ETextCommit::Type CommitInfo )
{
	FDateTime ParsedDateTime;
								
	if (FDateTime::Parse(NewText.ToString(), ParsedDateTime))
	{
		TArray<void*> RawData;
		PropertyHandle->AccessRawData(RawData);

		for (auto RawDataInstance : RawData)
		{
			*(FDateTime*)RawDataInstance = ParsedDateTime;
		}
	}
}


#undef LOCTEXT_NAMESPACE
