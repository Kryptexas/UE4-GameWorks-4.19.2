// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "SGraphPinComboBox.h"
#include "SGraphPinNameList.h"

void SGraphPinNameList::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const TArray<TSharedPtr<FName>>& InNameList)
{
	NameList = InNameList;
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SGraphPinNameList::GetDefaultValueWidget()
{
	//Get list of name indexes
	TArray< TSharedPtr<int32> > ComboItems;
	GenerateComboBoxIndexes( ComboItems );

	TSharedPtr<FName> CurrentlySelectedName;
	if (NameList.Num() > 0)
	{
		// Preserve previous selection, or set to first in list
		FName PreviousSelection = FName(*GraphPinObj->GetDefaultAsString());
		CurrentlySelectedName = NameList[0];
		for (TSharedPtr<FName> ListNamePtr : NameList)
		{
			if (PreviousSelection == *ListNamePtr.Get())
			{
				CurrentlySelectedName = ListNamePtr;
				break;
			}
		}
		check(CurrentlySelectedName.IsValid());
	}

	// Create widget
	return SAssignNew(ComboBox, SNameComboBox)
		.ContentPadding(FMargin(6.0f, 2.0f))
		.OptionsSource(&NameList)
		.InitiallySelectedItem(CurrentlySelectedName)
		.OnSelectionChanged(this, &SGraphPinNameList::ComboBoxSelectionChanged)
		.Visibility(this, &SGraphPin::GetDefaultValueVisibility)
		;
}

FString SGraphPinNameList::OnGetFriendlyName(int32 NameIndex)
{
	check(NameIndex < NameList.Num());
	return NameList[NameIndex]->ToString();
}

void SGraphPinNameList::ComboBoxSelectionChanged(TSharedPtr<FName> NameItem, ESelectInfo::Type SelectInfo )
{
	check(NameItem.IsValid());
	GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, *NameItem->ToString());
}

FString SGraphPinNameList::OnGetText() const 
{
	FString SelectedString = GraphPinObj->GetDefaultAsString();
	return SelectedString;
}

void SGraphPinNameList::GenerateComboBoxIndexes( TArray< TSharedPtr<int32> >& OutComboBoxIndexes )
{
	for (int32 NameIndex = 0; NameIndex < NameList.Num(); NameIndex++)
	{
		TSharedPtr<int32> NameIdxPtr(new int32(NameIndex));
		OutComboBoxIndexes.Add(NameIdxPtr);
	}
}
