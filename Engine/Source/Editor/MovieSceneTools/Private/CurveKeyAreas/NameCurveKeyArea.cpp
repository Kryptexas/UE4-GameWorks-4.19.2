// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "NameCurveKeyArea.h"


/* IKeyArea interface
 *****************************************************************************/

TArray<FKeyHandle> FNameCurveKeyArea::AddKeyUnique(float Time, EMovieSceneKeyInterpolation InKeyInterpolation, float TimeToCopyFrom)
{
	TArray<FKeyHandle> AddedKeyHandles;
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

		Curve.AddKey(Time, NAME_None, CurrentKey);
		AddedKeyHandles.Add(CurrentKey);
	}

	return AddedKeyHandles;
}


bool FNameCurveKeyArea::CanCreateKeyEditor()
{
	return false;
}


TSharedRef<SWidget> FNameCurveKeyArea::CreateKeyEditor(ISequencer* Sequencer)
{
	return SNullWidget::NullWidget;
}


void FNameCurveKeyArea::DeleteKey(FKeyHandle KeyHandle)
{
	Curve.DeleteKey(KeyHandle);
}


ERichCurveExtrapolation FNameCurveKeyArea::GetExtrapolationMode(bool bPreInfinity) const
{
	return RCCE_None;
}


ERichCurveInterpMode FNameCurveKeyArea::GetKeyInterpMode(FKeyHandle Keyhandle) const
{
	return RCIM_None;
}


ERichCurveTangentMode FNameCurveKeyArea::GetKeyTangentMode(FKeyHandle KeyHandle) const
{
	return RCTM_None;
}


float FNameCurveKeyArea::GetKeyTime(FKeyHandle KeyHandle) const
{
	return Curve.GetKeyTime(KeyHandle);
}


UMovieSceneSection* FNameCurveKeyArea::GetOwningSection()
{
	return OwningSection.Get();
}



FRichCurve* FNameCurveKeyArea::GetRichCurve()
{
	return nullptr;
}


TArray<FKeyHandle> FNameCurveKeyArea::GetUnsortedKeyHandles() const
{
	TArray<FKeyHandle> OutKeyHandles;

	for (auto It(Curve.GetKeyHandleIterator()); It; ++It)
	{
		OutKeyHandles.Add(It.Key());
	}

	return OutKeyHandles;
}


FKeyHandle FNameCurveKeyArea::MoveKey( FKeyHandle KeyHandle, float DeltaPosition )
{
	return Curve.SetKeyTime(KeyHandle, Curve.GetKeyTime(KeyHandle) + DeltaPosition);
}
	

void FNameCurveKeyArea::SetExtrapolationMode(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity)
{
	// do nothing
}


void FNameCurveKeyArea::SetKeyInterpMode(FKeyHandle KeyHandle, ERichCurveInterpMode InterpMode)
{
	// do nothing
}


void FNameCurveKeyArea::SetKeyTangentMode(FKeyHandle KeyHandle, ERichCurveTangentMode TangentMode)
{
	// do nothing
}


void FNameCurveKeyArea::SetKeyTime(FKeyHandle KeyHandle, float NewKeyTime) const
{
	Curve.SetKeyTime(KeyHandle, NewKeyTime);
}
