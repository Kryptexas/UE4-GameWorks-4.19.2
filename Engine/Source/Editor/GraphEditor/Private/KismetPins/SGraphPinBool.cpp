// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "GraphEditorCommon.h"
#include "SGraphPinBool.h"

void SGraphPinBool::Construct(const FArguments& InArgs, UEdGraphPin* InGraphPinObj)
{
	SGraphPin::Construct(SGraphPin::FArguments(), InGraphPinObj);
}

TSharedRef<SWidget>	SGraphPinBool::GetDefaultValueWidget()
{
	return SNew(SCheckBox)
		.Style(FEditorStyle::Get(), "Graph.Checkbox")
		.IsChecked(this, &SGraphPinBool::IsDefaultValueChecked)
		.OnCheckStateChanged(this, &SGraphPinBool::OnDefaultValueCheckBoxChanged)
		.Visibility( this, &SGraphPin::GetDefaultValueVisibility );
}

ESlateCheckBoxState::Type SGraphPinBool::IsDefaultValueChecked() const
{
	FString CurrentValue = GraphPinObj->GetDefaultAsString();
	return CurrentValue.ToBool() ? ESlateCheckBoxState::Checked : ESlateCheckBoxState::Unchecked;
}

void SGraphPinBool::OnDefaultValueCheckBoxChanged(ESlateCheckBoxState::Type InIsChecked)
{
	const FString BoolString = (InIsChecked == ESlateCheckBoxState::Checked) ? TEXT("true") : TEXT("false");
	GraphPinObj->GetSchema()->TrySetDefaultValue(*GraphPinObj, BoolString);
}