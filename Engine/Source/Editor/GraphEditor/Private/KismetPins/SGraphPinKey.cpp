// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "SGraphPinComboBox.h"
#include "SGraphPinEnum.h"
#include "SGraphPinKey.h"

void SGraphPinKey::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	EKeys::GetAllKeys(KeyList);

	const FKey SelectedKey(*InGraphPinObj->GetDefaultAsString());

	if (SelectedKey.IsValid())
	{
		for (int32 KeyIndex = 0; KeyIndex < KeyList.Num(); ++KeyIndex)
		{
			if (KeyList[KeyIndex] == SelectedKey)
			{
				SelectedIndex = KeyIndex;
				break;
			}
		}
	}
	else
	{
		SelectedIndex = 0;
		InGraphPinObj->GetSchema()->TrySetDefaultValue(*InGraphPinObj, KeyList[SelectedIndex].ToString());
	}

	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SGraphPinKey::GetDefaultValueWidget()
{
	//Get list of enum indexes
	TArray< TSharedPtr<int32> > ComboItems;
	GenerateComboBoxIndexes( ComboItems );

	//Create widget
	return SNew(SPinComboBox)
		.ComboItemList( ComboItems )
		.VisibleText( this, &SGraphPinKey::OnGetText )
		.OnSelectionChanged( this, &SGraphPinKey::ComboBoxSelectionChanged )
		.Visibility( this, &SGraphPin::GetDefaultValueVisibility )
		.OnGetDisplayName(this, &SGraphPinKey::OnGetFriendlyName);
}

FString SGraphPinKey::OnGetFriendlyName(int32 EnumIndex)
{
	check(EnumIndex < KeyList.Num());
	return KeyList[EnumIndex].GetDisplayName().ToString();
}

void SGraphPinKey::ComboBoxSelectionChanged( TSharedPtr<int32> NewSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	check(*NewSelection < KeyList.Num());
	SelectedIndex = *NewSelection;
	GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, KeyList[SelectedIndex].ToString());
}

FString SGraphPinKey::OnGetText() const 
{
	return KeyList[SelectedIndex].GetDisplayName().ToString();
}

void SGraphPinKey::GenerateComboBoxIndexes( TArray< TSharedPtr<int32> >& OutComboBoxIndexes )
{
	for (int32 KeyIndex = 0; KeyIndex < KeyList.Num(); ++KeyIndex)
	{
		TSharedPtr<int32> EnumIdxPtr(new int32(KeyIndex));
		OutComboBoxIndexes.Add(EnumIdxPtr);
	}
}