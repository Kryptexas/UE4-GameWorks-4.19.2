// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "SBoolCurveKeyEditor.h"

#define LOCTEXT_NAMESPACE "BoolCurveKeyEditor"

void SBoolCurveKeyEditor::Construct(const FArguments& InArgs)
{
	Sequencer = InArgs._Sequencer;
	OwningSection = InArgs._OwningSection;
	Curve = InArgs._Curve;
	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());
	ChildSlot
	[
		SNew(SCheckBox)
		.IsChecked(this, &SBoolCurveKeyEditor::IsChecked)
		.OnCheckStateChanged(this, &SBoolCurveKeyEditor::OnCheckStateChanged)
	];
}

ECheckBoxState SBoolCurveKeyEditor::IsChecked() const
{
	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());
	return !!Curve->Evaluate(CurrentTime) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SBoolCurveKeyEditor::OnCheckStateChanged(ECheckBoxState NewCheckboxState)
{
	FScopedTransaction Transaction(LOCTEXT("SetBoolKey", "Set bool key value"));
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

	Curve->UpdateOrAddKey(CurrentTime, NewCheckboxState == ECheckBoxState::Checked ? 1 : 0);
	Sequencer->UpdateRuntimeInstances();
}

#undef LOCTEXT_NAMESPACE