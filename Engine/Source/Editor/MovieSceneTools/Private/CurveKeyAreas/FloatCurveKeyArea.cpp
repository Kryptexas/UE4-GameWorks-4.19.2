// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneToolsPrivatePCH.h"
#include "FloatCurveKeyArea.h"
#include "SFloatCurveKeyEditor.h"


/* IKeyArea interface
 *****************************************************************************/

TArray<FKeyHandle> FFloatCurveKeyArea::AddKeyUnique(float Time, EMovieSceneKeyInterpolation InKeyInterpolation, float TimeToCopyFrom)
{
	TArray<FKeyHandle> AddedKeyHandles;
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

		float Value = Curve->Eval(Time);

		if (TimeToCopyFrom != FLT_MAX)
		{
			Value = Curve->Eval(TimeToCopyFrom);
		}
		else if ( IntermediateValue.IsSet() )
		{
			Value = IntermediateValue.GetValue();
		}

		Curve->AddKey(Time, Value, false, CurrentKeyHandle);
		AddedKeyHandles.Add(CurrentKeyHandle);
		
		MovieSceneHelpers::SetKeyInterpolation(*Curve, CurrentKeyHandle, InKeyInterpolation);		
		
		// Copy the properties from the key if it exists
		FKeyHandle KeyHandleToCopy = Curve->FindKey(TimeToCopyFrom);

		if (Curve->IsKeyHandleValid(KeyHandleToCopy))
		{
			FRichCurveKey& CurrentKey = Curve->GetKey(CurrentKeyHandle);
			FRichCurveKey& KeyToCopy = Curve->GetKey(KeyHandleToCopy);
			CurrentKey.InterpMode = KeyToCopy.InterpMode;
			CurrentKey.TangentMode = KeyToCopy.TangentMode;
			CurrentKey.TangentWeightMode = KeyToCopy.TangentWeightMode;
			CurrentKey.ArriveTangent = KeyToCopy.ArriveTangent;
			CurrentKey.LeaveTangent = KeyToCopy.LeaveTangent;
			CurrentKey.ArriveTangentWeight = KeyToCopy.ArriveTangentWeight;
			CurrentKey.LeaveTangentWeight = KeyToCopy.LeaveTangentWeight;
		}
	}

	return AddedKeyHandles;
}


bool FFloatCurveKeyArea::CanCreateKeyEditor()
{
	return true;
}


TSharedRef<SWidget> FFloatCurveKeyArea::CreateKeyEditor(ISequencer* Sequencer)
{
	return SNew(SFloatCurveKeyEditor)
		.Sequencer(Sequencer)
		.OwningSection(OwningSection)
		.Curve(Curve)
		.IntermediateValue_Lambda([this] {
			return IntermediateValue;
		});
};


void FFloatCurveKeyArea::DeleteKey(FKeyHandle KeyHandle)
{
	if (Curve->IsKeyHandleValid(KeyHandle))
	{
		Curve->DeleteKey(KeyHandle);
	}
}


ERichCurveExtrapolation FFloatCurveKeyArea::GetExtrapolationMode(bool bPreInfinity) const
{
	if (bPreInfinity)
	{
		return Curve->PreInfinityExtrap;
	}

	return Curve->PostInfinityExtrap;
}


ERichCurveInterpMode FFloatCurveKeyArea::GetKeyInterpMode(FKeyHandle KeyHandle) const
{
	if (Curve->IsKeyHandleValid(KeyHandle))
	{
		return Curve->GetKeyInterpMode(KeyHandle);
	}

	return RCIM_None;
}


ERichCurveTangentMode FFloatCurveKeyArea::GetKeyTangentMode(FKeyHandle KeyHandle) const
{
	if (Curve->IsKeyHandleValid(KeyHandle))
	{
		return Curve->GetKeyTangentMode(KeyHandle);
	}

	return RCTM_None;
}


float FFloatCurveKeyArea::GetKeyTime(FKeyHandle KeyHandle) const
{
	return Curve->GetKeyTime(KeyHandle);
}


UMovieSceneSection* FFloatCurveKeyArea::GetOwningSection()
{
	return OwningSection;
}


FRichCurve* FFloatCurveKeyArea::GetRichCurve()
{
	return Curve;
}


TArray<FKeyHandle> FFloatCurveKeyArea::GetUnsortedKeyHandles() const
{
	TArray<FKeyHandle> OutKeyHandles;

	for (auto It(Curve->GetKeyHandleIterator()); It; ++It)
	{
		OutKeyHandles.Add(It.Key());
	}

	return OutKeyHandles;
}


FKeyHandle FFloatCurveKeyArea::MoveKey(FKeyHandle KeyHandle, float DeltaPosition)
{
	return Curve->SetKeyTime(KeyHandle, Curve->GetKeyTime(KeyHandle) + DeltaPosition);
}


void FFloatCurveKeyArea::SetExtrapolationMode(ERichCurveExtrapolation ExtrapMode, bool bPreInfinity)
{
	if (bPreInfinity)
	{
		Curve->PreInfinityExtrap = ExtrapMode;
	}
	else
	{
		Curve->PostInfinityExtrap = ExtrapMode;
	}
}


void FFloatCurveKeyArea::SetKeyInterpMode(FKeyHandle KeyHandle, ERichCurveInterpMode InterpMode)
{
	if (Curve->IsKeyHandleValid(KeyHandle))
	{
		Curve->SetKeyInterpMode(KeyHandle, InterpMode);
	}
}


void FFloatCurveKeyArea::SetKeyTangentMode(FKeyHandle KeyHandle, ERichCurveTangentMode TangentMode)
{
	if (Curve->IsKeyHandleValid(KeyHandle))
	{
		Curve->SetKeyTangentMode(KeyHandle, TangentMode);
	}
}


void FFloatCurveKeyArea::SetKeyTime(FKeyHandle KeyHandle, float NewKeyTime) const
{
	Curve->SetKeyTime(KeyHandle, NewKeyTime);
}
