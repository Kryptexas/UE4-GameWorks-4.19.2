// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "SFloatCurveKeyEditor.h"

#define LOCTEXT_NAMESPACE "FloatCurveKeyEditor"

void SFloatCurveKeyEditor::Construct(const FArguments& InArgs)
{
	Sequencer = InArgs._Sequencer;
	OwningSection = InArgs._OwningSection;
	Curve = InArgs._Curve;
	ChildSlot
	[
		SNew(SSpinBox<float>)
		.Style(&FEditorStyle::GetWidgetStyle<FSpinBoxStyle>("Sequencer.AnimationOutliner.KeyEditorSpinBoxStyle"))
		.Font(FEditorStyle::GetFontStyle("Sequencer.AnimationOutliner.RegularFont"))
		.MinValue(TOptional<float>())
		.MaxValue(TOptional<float>())
		.MaxSliderValue(TOptional<float>())
		.MinSliderValue(TOptional<float>())
		.Delta(0.001f)
		.MinDesiredWidth(60.0f)
		.Value(this, &SFloatCurveKeyEditor::OnGetKeyValue)
		.OnValueChanged(this, &SFloatCurveKeyEditor::OnValueChanged)
		.OnBeginSliderMovement(this, &SFloatCurveKeyEditor::OnBeginSliderMovement)
		.OnEndSliderMovement(this, &SFloatCurveKeyEditor::OnEndSliderMovement)
	];
}

void SFloatCurveKeyEditor::OnBeginSliderMovement()
{
	GEditor->BeginTransaction(LOCTEXT("SetFloatKey", "Set float key value"));
	OwningSection->SetFlags(RF_Transactional);
	OwningSection->Modify();
}

void SFloatCurveKeyEditor::OnEndSliderMovement(float Value)
{
	if (GEditor->IsTransactionActive())
	{
		GEditor->EndTransaction();
	}
}

float SFloatCurveKeyEditor::OnGetKeyValue() const
{
	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());
	return Curve->Eval(CurrentTime);
}

void SFloatCurveKeyEditor::OnValueChanged(float Value)
{
	OwningSection->Modify();

	float CurrentTime = Sequencer->GetCurrentLocalTime(*Sequencer->GetFocusedMovieScene());
	FKeyHandle CurrentKeyHandle = Curve->FindKey(CurrentTime);
	if (Curve->IsKeyHandleValid(CurrentKeyHandle))
	{
		Curve->SetKeyValue(CurrentKeyHandle, Value);
	}
	else
	{
		Curve->AddKey(CurrentTime, Value, false, CurrentKeyHandle);
		if (OwningSection->GetStartTime() > CurrentTime)
		{
			OwningSection->SetStartTime(CurrentTime);
		}
		if (OwningSection->GetEndTime() < CurrentTime)
		{
			OwningSection->SetEndTime(CurrentTime);
		}
	}
	Sequencer->UpdateRuntimeInstances();
}

#undef LOCTEXT_NAMESPACE