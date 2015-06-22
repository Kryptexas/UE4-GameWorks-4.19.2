// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "SEnumCurveKeyEditor.h"

#define LOCTEXT_NAMESPACE "EnumCurveKeyEditor"

void SEnumCurveKeyEditor::Construct(const FArguments& InArgs)
{
	Sequencer = InArgs._Sequencer;
	OwningSection = InArgs._OwningSection;
	Curve = InArgs._Curve;
	for (int32 i = 0; i < InArgs._Enum->NumEnums(); i++)
	{
		FString EnumDisplayValue = InArgs._Enum->GetDisplayNameText(i).ToString();
		if (EnumDisplayValue.Len() > 0)
		{
			EnumDisplayValue = InArgs._Enum->GetEnumName(i);
		}
		EnumValues.Add(MakeShareable(new FString(EnumDisplayValue)));
	}
	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());
	ChildSlot
	[
		SNew(SComboBox<TSharedPtr<FString>>)
		.ButtonStyle(FEditorStyle::Get(), "FlatButton.Light")
		.OptionsSource(&EnumValues)
		.InitiallySelectedItem(EnumValues[Curve->Evaluate(CurrentTime)])
		.OnGenerateWidget(this, &SEnumCurveKeyEditor::OnGenerateWidget)
		.OnSelectionChanged(this, &SEnumCurveKeyEditor::OnComboSelectionChanged)
		.ContentPadding(FMargin(2, 0))
		[
			SNew(STextBlock)
			.Font(FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.RegularFont"))
			.Text(this, &SEnumCurveKeyEditor::GetCurrentValue)
		]
	];
}

FText SEnumCurveKeyEditor::GetCurrentValue() const
{
	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());
	return FText::FromString(*EnumValues[Curve->Evaluate(CurrentTime)]);
}

TSharedRef<SWidget> SEnumCurveKeyEditor::OnGenerateWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock)
		.Text(FText::FromString(*InItem));
}

void SEnumCurveKeyEditor::OnComboSelectionChanged(TSharedPtr<FString> InSelectedItem, ESelectInfo::Type SelectInfo)
{
	FScopedTransaction Transaction(LOCTEXT("SetEnumKey", "Set enum key value"));
	OwningSection->SetFlags(RF_Transactional);
	OwningSection->Modify();

	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());

	bool bKeyWillBeAdded = Curve->IsKeyHandleValid(Curve->FindKey(CurrentTime)) == false;
	if (bKeyWillBeAdded)
	{
		if (OwningSection->GetStartTime() > CurrentTime)
		{
			OwningSection->SetStartTime(CurrentTime);
		}
		if (OwningSection->GetEndTime() < CurrentTime)
		{
			OwningSection->SetEndTime(CurrentTime);
		}
	}

	int32 SelectedIndex;
	EnumValues.Find(InSelectedItem, SelectedIndex);
	Curve->UpdateOrAddKey(CurrentTime, SelectedIndex);
	Sequencer->UpdateRuntimeInstances();
}

#undef LOCTEXT_NAMESPACE