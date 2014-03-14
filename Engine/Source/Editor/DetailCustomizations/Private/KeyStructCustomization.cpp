// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	KeyStructCustomization.cpp: Implements the FKeyStructCustomization class.
=============================================================================*/

#include "DetailCustomizationsPrivatePCH.h"
#include "KeyStructCustomization.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "FKeyStructCustomization"


/* FKeyStructCustomization static interface
 *****************************************************************************/

TSharedRef<IStructCustomization> FKeyStructCustomization::MakeInstance( )
{
	return MakeShareable(new FKeyStructCustomization);
}


/* IStructCustomization interface
 *****************************************************************************/

void FKeyStructCustomization::CustomizeStructHeader( TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IStructCustomizationUtils& StructCustomizationUtils )
{
	PropertyHandle = StructPropertyHandle;

	TArray<void*> StructPtrs;
	StructPropertyHandle->AccessRawData(StructPtrs);
	check(StructPtrs.Num() != 0);
	const FKey& SelectedKey = *(FKey*)StructPtrs[0];

	bool bMultipleValues = false;
	for (int32 StructPtrIndex = 1; StructPtrIndex < StructPtrs.Num(); ++StructPtrIndex)
	{
		if (*(FKey*)StructPtrs[StructPtrIndex] != SelectedKey)
		{
			bMultipleValues = true;
			break;
		}
	}

	TSharedPtr<FKey> InitialSelectedItem;
	TArray<FKey> AllKeys;
	EKeys::GetAllKeys(AllKeys);

	for (FKey Key : AllKeys)
	{
		TSharedPtr<FKey> InputKey = MakeShareable(new FKey(Key));
		InputKeys.Add(InputKey);
		if (Key == SelectedKey)
		{
			InitialSelectedItem = InputKey;
		}
	}
	
	// create struct header
	HeaderRow.NameContent()
	[
		StructPropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.MinDesiredWidth(125.0f)
	.MaxDesiredWidth(325.0f)
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				// text box
				SNew(SComboBox< TSharedPtr<FKey> >)
				.OptionsSource(&InputKeys)
				.InitiallySelectedItem(InitialSelectedItem)
				.OnGenerateWidget(this, &FKeyStructCustomization::OnGenerateComboWidget)			
				.OnSelectionChanged(this, &FKeyStructCustomization::OnSelectionChanged)
				.Content()
				[
					SAssignNew(TextBlock, STextBlock)
					.Text(bMultipleValues ? LOCTEXT("MultipleValues", "Multiple Values") : SelectedKey.GetDisplayName())
					.Font(StructCustomizationUtils.GetRegularFont())
				]
			]
	];
}


TSharedRef<SWidget> FKeyStructCustomization::OnGenerateComboWidget(TSharedPtr<FKey> Key)
{
	return
		SNew(STextBlock)
		.Text(Key->GetDisplayName());
}

void FKeyStructCustomization::OnSelectionChanged(TSharedPtr<FKey> SelectedItem, ESelectInfo::Type SelectInfo)
{
	PropertyHandle->SetValueFromFormattedString(SelectedItem->ToString());
	TextBlock->SetText(SelectedItem->GetDisplayName());
}

#undef LOCTEXT_NAMESPACE
