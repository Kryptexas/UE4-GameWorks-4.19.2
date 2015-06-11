// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "SIntegralCurveKeyEditor.h"

#define LOCTEXT_NAMESPACE "IntegralCurveKeyEditor"

void SIntegralCurveKeyEditor::Construct(const FArguments& InArgs)
{
	Sequencer = InArgs._Sequencer;
	OwningSection = InArgs._OwningSection;
	Curve = InArgs._Curve;
	ChildSlot
	[
		SNew(SSpinBox<int32>)
		.Style(&FEditorStyle::GetWidgetStyle<FSpinBoxStyle>("Sequencer.AnimationOutliner.KeyEditorSpinBoxStyle"))
		.Font(FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.RegularFont"))
		.MinValue(TOptional<int32>())
		.MaxValue(TOptional<int32>())
		.MaxSliderValue(TOptional<int32>())
		.MinSliderValue(TOptional<int32>())
		.Delta(1)
		.MinDesiredWidth(60.0f)
		.Value(this, &SIntegralCurveKeyEditor::OnGetKeyValue)
		.OnValueChanged(this, &SIntegralCurveKeyEditor::OnValueChanged)
		.OnBeginSliderMovement(this, &SIntegralCurveKeyEditor::OnBeginSliderMovement)
		.OnEndSliderMovement(this, &SIntegralCurveKeyEditor::OnEndSliderMovement)
	];
}

void SIntegralCurveKeyEditor::OnBeginSliderMovement()
{
	GEditor->BeginTransaction(LOCTEXT("SetIntegralKey", "Set integral key value"));
	OwningSection->SetFlags(RF_Transactional);
	OwningSection->Modify();
}

void SIntegralCurveKeyEditor::OnEndSliderMovement(int32 Value)
{
	if (GEditor->IsTransactionActive())
	{
		GEditor->EndTransaction();
	}
}

int32 SIntegralCurveKeyEditor::OnGetKeyValue() const
{
	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());
	return Curve->Evaluate(CurrentTime);
}

void SIntegralCurveKeyEditor::OnValueChanged(int32 Value)
{
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

	Curve->UpdateOrAddKey(CurrentTime, Value);
	Sequencer->UpdateRuntimeInstances();
}

#undef LOCTEXT_NAMESPACE