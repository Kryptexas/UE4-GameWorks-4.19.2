// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTracksPrivatePCH.h"
#include "MovieScene3DTransformSection.h"
#include "MovieScene3DTransformTrack.h"


/* FMovieScene3DTransformKeyStruct interface
 *****************************************************************************/

void FMovieScene3DTransformKeyStruct::PropagateChanges(const FPropertyChangedEvent& ChangeEvent)
{
	for (int32 Index = 0; Index <= 2; ++Index)
	{
		TranslationKeys[Index]->Value = Translation[Index];
		ScaleKeys[Index]->Value = Scale[Index];
	}

	RotationKeys[0]->Value = Rotation.Pitch;
	RotationKeys[1]->Value = Rotation.Yaw;
	RotationKeys[2]->Value = Rotation.Roll;
}


/* FMovieScene3DTransformKeyStruct interface
 *****************************************************************************/

UMovieScene3DTransformSection::UMovieScene3DTransformSection(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{ }


/* UMovieScene3DTransformSection interface
 *****************************************************************************/
void UMovieScene3DTransformSection::EvalTranslation(float Time, FVector& OutTranslation) const
{
	OutTranslation.X = Translation[0].Eval(Time);
	OutTranslation.Y = Translation[1].Eval(Time);
	OutTranslation.Z = Translation[2].Eval(Time);
}


void UMovieScene3DTransformSection::EvalRotation(float Time, FRotator& OutRotation) const
{
	OutRotation.Roll = Rotation[0].Eval(Time);
	OutRotation.Pitch = Rotation[1].Eval(Time);
	OutRotation.Yaw = Rotation[2].Eval(Time);
}


void UMovieScene3DTransformSection::EvalScale(float Time, FVector& OutScale) const
{
	OutScale.X = Scale[0].Eval(Time);
	OutScale.Y = Scale[1].Eval(Time);
	OutScale.Z = Scale[2].Eval(Time);
}


/**
 * Chooses an appropriate curve from an axis and a set of curves
 */
static FRichCurve* ChooseCurve(EAxis::Type Axis, FRichCurve* Curves)
{
	switch(Axis)
	{
	case EAxis::X:
		return &Curves[0];

	case EAxis::Y:
		return &Curves[1];

	case EAxis::Z:
		return &Curves[2];

	default:
		check(false);
		return nullptr;
	}
}


FRichCurve& UMovieScene3DTransformSection::GetTranslationCurve(EAxis::Type Axis) 
{
	return *ChooseCurve(Axis, Translation);
}


FRichCurve& UMovieScene3DTransformSection::GetRotationCurve(EAxis::Type Axis)
{
	return *ChooseCurve(Axis, Rotation);
}


FRichCurve& UMovieScene3DTransformSection::GetScaleCurve(EAxis::Type Axis)
{
	return *ChooseCurve(Axis, Scale);
}


/* UMovieSceneSection interface
 *****************************************************************************/

void UMovieScene3DTransformSection::MoveSection(float DeltaTime, TSet<FKeyHandle>& KeyHandles)
{
	Super::MoveSection(DeltaTime, KeyHandles);

	// Move all the curves in this section
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		Translation[Axis].ShiftCurve(DeltaTime, KeyHandles);
		Rotation[Axis].ShiftCurve(DeltaTime, KeyHandles);
		Scale[Axis].ShiftCurve(DeltaTime, KeyHandles);
	}
}


void UMovieScene3DTransformSection::DilateSection(float DilationFactor, float Origin, TSet<FKeyHandle>& KeyHandles)
{
	Super::DilateSection(DilationFactor, Origin, KeyHandles);

	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		Translation[Axis].ScaleCurve(Origin, DilationFactor, KeyHandles);
		Rotation[Axis].ScaleCurve(Origin, DilationFactor, KeyHandles);
		Scale[Axis].ScaleCurve(Origin, DilationFactor, KeyHandles);
	}
}


void UMovieScene3DTransformSection::GetKeyHandles(TSet<FKeyHandle>& KeyHandles) const
{
	for (int32 Axis = 0; Axis < 3; ++Axis)
	{
		for (auto It(Translation[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Translation[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}

		for (auto It(Rotation[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Rotation[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}

		for (auto It(Scale[Axis].GetKeyHandleIterator()); It; ++It)
		{
			float Time = Scale[Axis].GetKeyTime(It.Key());
			if (IsTimeWithinSection(Time))
			{
				KeyHandles.Add(It.Key());
			}
		}
	}
}


TSharedPtr<FStructOnScope> UMovieScene3DTransformSection::GetKeyStruct(const TArray<FKeyHandle>& KeyHandles)
{
	TSharedRef<FStructOnScope> KeyStruct = MakeShareable(new FStructOnScope(FMovieScene3DTransformKeyStruct::StaticStruct()));
	auto Struct = (FMovieScene3DTransformKeyStruct*)KeyStruct->GetStructMemory();
	{
		for (int32 Index = 0; Index <= 2; ++Index)
		{
			Struct->TranslationKeys[Index] = &Translation[Index].GetFirstMatchingKey(KeyHandles);
			Struct->Translation[Index] = Struct->TranslationKeys[Index]->Value;
			Struct->RotationKeys[Index] = &Rotation[Index].GetFirstMatchingKey(KeyHandles);
			Struct->ScaleKeys[Index] = &Scale[Index].GetFirstMatchingKey(KeyHandles);
			Struct->Scale[Index] = Struct->ScaleKeys[Index]->Value;
		}

		Struct->Rotation.Pitch = Struct->RotationKeys[0]->Value;
		Struct->Rotation.Yaw = Struct->RotationKeys[1]->Value;
		Struct->Rotation.Roll = Struct->RotationKeys[2]->Value;
	}

	return KeyStruct;
}


/* UMovieSceneSection interface
 *****************************************************************************/

// This function uses a template to avoid duplicating this for const and non-const versions
template<typename CurveType>
CurveType* GetCurveForChannelAndAxis(EKey3DTransformChannel::Type Channel, EAxis::Type Axis,
	CurveType* TranslationCurves, CurveType* RotationCurves, CurveType* ScaleCurves)
{
	CurveType* ChannelCurves = nullptr;
	switch (Channel)
	{
	case EKey3DTransformChannel::Translation:
		ChannelCurves = TranslationCurves;
		break;
	case EKey3DTransformChannel::Rotation:
		ChannelCurves = RotationCurves;
		break;
	case EKey3DTransformChannel::Scale:
		ChannelCurves = ScaleCurves;
		break;
	}

	if (ChannelCurves != nullptr)
	{
		switch (Axis)
		{
		case EAxis::X:
			return &ChannelCurves[0];
		case EAxis::Y:
			return &ChannelCurves[1];
		case EAxis::Z:
			return &ChannelCurves[2];
		}
	}

	checkf(false, TEXT("Invalid channel and axis combination."));
	return nullptr;
}


bool UMovieScene3DTransformSection::NewKeyIsNewData(float Time, const FTransformKey& TransformKey) const
{
	const FRichCurve* KeyCurve = GetCurveForChannelAndAxis(TransformKey.Channel, TransformKey.Axis, Translation, Rotation, Scale);
	return FMath::IsNearlyEqual(KeyCurve->Eval(Time), TransformKey.Value) == false;
}


bool UMovieScene3DTransformSection::HasKeys(const FTransformKey& TransformKey) const
{
	const FRichCurve* KeyCurve = GetCurveForChannelAndAxis(TransformKey.Channel, TransformKey.Axis, Translation, Rotation, Scale);
	return KeyCurve->GetNumKeys() != 0;
}


void UMovieScene3DTransformSection::AddKey(float Time, const FTransformKey& TransformKey, EMovieSceneKeyInterpolation KeyInterpolation)
{
	FRichCurve* KeyCurve = GetCurveForChannelAndAxis(TransformKey.Channel, TransformKey.Axis, Translation, Rotation, Scale);
	AddKeyToCurve(*KeyCurve, Time, TransformKey.Value, KeyInterpolation);
}


void UMovieScene3DTransformSection::SetDefault(const FTransformKey& TransformKey)
{
	FRichCurve* KeyCurve = GetCurveForChannelAndAxis(TransformKey.Channel, TransformKey.Axis, Translation, Rotation, Scale);
	SetCurveDefault(*KeyCurve, TransformKey.Value);
}
