// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "SGraphPinComboBox.h"
#include "SGraphPinEnum.h"
#include "SGraphPinNameList.h"

void SGraphPinNameList::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj, const TArray<FName>& InNameList)
{
	NameList = InNameList;
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SGraphPinNameList::GetDefaultValueWidget()
{
	//Get list of enum indexes
	TArray< TSharedPtr<int32> > ComboItems;
	GenerateComboBoxIndexes( ComboItems );

	//Create widget
	return SAssignNew(ComboBox, SPinComboBox)
		.ComboItemList( ComboItems )
		.VisibleText( this, &SGraphPinNameList::OnGetText )
		.OnSelectionChanged( this, &SGraphPinNameList::ComboBoxSelectionChanged )
		.Visibility( this, &SGraphPin::GetDefaultValueVisibility )
		.OnGetDisplayName(this, &SGraphPinNameList::OnGetFriendlyName);
}

FString SGraphPinNameList::OnGetFriendlyName(int32 EnumIndex)
{
	check(EnumIndex < NameList.Num());
	return NameList[EnumIndex].ToString();
}

void SGraphPinNameList::ComboBoxSelectionChanged( TSharedPtr<int32> NewSelection, ESelectInfo::Type /*SelectInfo*/ )
{
	check(*NewSelection < NameList.Num());
	GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, NameList[*NewSelection].ToString());
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
		TSharedPtr<int32> EnumIdxPtr(new int32(NameIndex));
		OutComboBoxIndexes.Add(EnumIdxPtr);
	}
}