// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "PropertyEditing.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "ScopedTransaction.h"
#include "MovieSceneSection.h"
#include "ISequencerObjectChangeListener.h"
#include "ISequencerSection.h"
#include "ISectionLayoutBuilder.h"
#include "IKeyArea.h"
#include "MovieSceneToolHelpers.h"
#include "SFloatCurveKeyEditor.h"
#include "SIntegralCurveKeyEditor.h"
#include "SEnumCurveKeyEditor.h"
#include "SBoolCurveKeyEditor.h"

void FFloatCurveKeyArea::AddKeyUnique(float Time)
{
	FKeyHandle CurrentKeyHandle = Curve->FindKey(Time);
	if (Curve->IsKeyHandleValid(CurrentKeyHandle) == false)
	{
		if (OwningSection->GetStartTime() > Time)
		{
			OwningSection->SetStartTime(Time);
		}
		if (OwningSection->GetEndTime() < Time)
		{
			OwningSection->SetEndTime(Time);
		}
		Curve->AddKey(Time, Curve->Eval(Time), false, CurrentKeyHandle);
	}
}

TSharedRef<SWidget> FFloatCurveKeyArea::CreateKeyEditor(ISequencer* Sequencer)
{
	return SNew(SFloatCurveKeyEditor)
		.Sequencer(Sequencer)
		.OwningSection(OwningSection)
		.Curve(Curve);
};

void FIntegralKeyArea::AddKeyUnique(float Time)
{
	FKeyHandle CurrentKey = Curve.FindKey(Time);
	if (Curve.IsKeyHandleValid(CurrentKey) == false)
	{
		if (OwningSection->GetStartTime() > Time)
		{
			OwningSection->SetStartTime(Time);
		}
		if (OwningSection->GetEndTime() < Time)
		{
			OwningSection->SetEndTime(Time);
		}
		Curve.AddKey(Time, Curve.Evaluate(Time), CurrentKey);
	}
}

TSharedRef<SWidget> FIntegralKeyArea::CreateKeyEditor(ISequencer* Sequencer)
{
	return SNew(SIntegralCurveKeyEditor)
		.Sequencer(Sequencer)
		.OwningSection(OwningSection)
		.Curve(&Curve);
};

TSharedRef<SWidget> FEnumKeyArea::CreateKeyEditor(ISequencer* Sequencer)
{
	return SNew(SEnumCurveKeyEditor)
		.Sequencer(Sequencer)
		.OwningSection(OwningSection)
		.Curve(&Curve)
		.Enum(Enum);
};

TSharedRef<SWidget> FBoolKeyArea::CreateKeyEditor(ISequencer* Sequencer)
{
	return SNew(SBoolCurveKeyEditor)
		.Sequencer(Sequencer)
		.OwningSection(OwningSection)
		.Curve(&Curve);
};

