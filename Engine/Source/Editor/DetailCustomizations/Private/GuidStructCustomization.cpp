// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	GuidStructCustomization.cpp: Implements the FGuidStructCustomization class.
=============================================================================*/

#include "DetailCustomizationsPrivatePCH.h"
#include "GuidStructCustomization.h"


#define LOCTEXT_NAMESPACE "FGuidStructCustomization"


/* FGuidStructCustomization static interface
 *****************************************************************************/

TSharedRef<IStructCustomization> FGuidStructCustomization::MakeInstance( )
{
	return MakeShareable(new FGuidStructCustomization);
}


/* IStructCustomization interface
 *****************************************************************************/

void FGuidStructCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	PropertyHandle = StructPropertyHandle;
	InputValid = true;

	// create quick-set menu
	FMenuBuilder QuickSetMenuBuilder(true, NULL);
	{
		FUIAction GenerateAction(FExecuteAction::CreateSP(this, &FGuidStructCustomization::HandleGuidActionClicked, EPropertyEditorGuidActions::Generate));
		QuickSetMenuBuilder.AddMenuEntry(LOCTEXT("GenerateAction", "Generate"), LOCTEXT("GenerateActionHint", "Generate a new random globally unique identifier (GUID)."), FSlateIcon(), GenerateAction);

		FUIAction InvalidateAction(FExecuteAction::CreateSP(this, &FGuidStructCustomization::HandleGuidActionClicked, EPropertyEditorGuidActions::Invalidate));
		QuickSetMenuBuilder.AddMenuEntry(LOCTEXT("InvalidateAction", "Invalidate"), LOCTEXT("InvalidateActionHint", "Set an invalid globally unique identifier (GUID)."), FSlateIcon(), InvalidateAction);
	}

	// create struct header
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(325.0f)
	.MaxDesiredWidth(325.0f)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				// text box
				SAssignNew(TextBox, SEditableTextBox)
					.ClearKeyboardFocusOnCommit(false)
					.ForegroundColor(this, &FGuidStructCustomization::HandleTextBoxForegroundColor)
					.OnTextChanged(this, &FGuidStructCustomization::HandleTextBoxTextChanged)
					.OnTextCommitted(this, &FGuidStructCustomization::HandleTextBoxTextCommited)
					.SelectAllTextOnCommit(true)
					.Text(this, &FGuidStructCustomization::HandleTextBoxText)
			]

		+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				// quick-set menu
				SNew(SComboButton)
					.ContentPadding(FMargin(6.0, 2.0))
					.MenuContent()
					[
						QuickSetMenuBuilder.MakeWidget()
					]
			]
	];
}


void FGuidStructCustomization::CustomizeStructChildren( TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IStructCustomizationUtils& StructCustomizationUtils )
{
	// do nothing
}


/* FGuidStructCustomization implementation
 *****************************************************************************/

void FGuidStructCustomization::SetGuidValue( const FGuid& Guid )
{
	for (uint32 ChildIndex = 0; ChildIndex < 4; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		ChildHandle->SetValue((int32)Guid[ChildIndex]);
	}
}


/* FGuidStructCustomization callbacks
 *****************************************************************************/

void FGuidStructCustomization::HandleGuidActionClicked( EPropertyEditorGuidActions::Type Action )
{
	if (Action == EPropertyEditorGuidActions::Generate)
	{
		SetGuidValue(FGuid::NewGuid());
	}
	else if (Action == EPropertyEditorGuidActions::Invalidate)
	{
		SetGuidValue(FGuid());
	}
}


FSlateColor FGuidStructCustomization::HandleTextBoxForegroundColor( ) const
{
	if (InputValid)
	{
		return FEditorStyle::GetSlateColor("InvertedForeground");
	}

	return FLinearColor::Red;
}


FText FGuidStructCustomization::HandleTextBoxText( ) const
{
	TArray<void*> RawData;
	PropertyHandle->AccessRawData(RawData);
	
	if (RawData.Num() != 1)
	{
		return LOCTEXT("MultipleValues", "Multiple Values");
	}

	return FText::FromString(((FGuid*)RawData[0])->ToString(EGuidFormats::DigitsWithHyphensInBraces));
}


void FGuidStructCustomization::HandleTextBoxTextChanged( const FText& NewText )
{
	FGuid Guid;
	InputValid = FGuid::Parse(NewText.ToString(), Guid);
}


void FGuidStructCustomization::HandleTextBoxTextCommited( const FText& NewText, ETextCommit::Type CommitInfo )
{
	FGuid ParsedGuid;
								
	if (FGuid::Parse(NewText.ToString(), ParsedGuid))
	{
		SetGuidValue(ParsedGuid);
	}
}


#undef LOCTEXT_NAMESPACE
