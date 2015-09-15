// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MovieScenePrivatePCH.h"
#include "MovieSceneSection.h"

UMovieSceneSection::UMovieSceneSection( const FObjectInitializer& ObjectInitializer )
	: Super( ObjectInitializer )
	, StartTime(0.0f)
	, EndTime(0.0f)
	, RowIndex(0)
	, bIsActive(true)
	, bIsInfinite(false)
{
}

const UMovieSceneSection* UMovieSceneSection::OverlapsWithSections(const TArray<UMovieSceneSection*>& Sections, int32 TrackDelta, float TimeDelta) const
{
	int32 NewTrackIndex = RowIndex + TrackDelta;
	TRange<float> NewSectionRange = TRange<float>(StartTime + TimeDelta, EndTime + TimeDelta);
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
	{
		const UMovieSceneSection* InSection = Sections[SectionIndex];
		if (this != InSection && InSection->GetRowIndex() == NewTrackIndex)
		{
			if (NewSectionRange.Overlaps(InSection->GetRange()))
			{
				return InSection;
			}
		}
	}
	return nullptr;
}

void UMovieSceneSection::InitialPlacement(const TArray<UMovieSceneSection*>& Sections, float InStartTime, float InEndTime, bool bAllowMultipleRows)
{
	check(StartTime <= EndTime);

	StartTime = InStartTime;
	EndTime = InEndTime;

	RowIndex = 0;
	if (bAllowMultipleRows)
	{
		while (OverlapsWithSections(Sections) != nullptr)
		{
			++RowIndex;
		}
	}
	else
	{
		for (;;)
		{
			const UMovieSceneSection* OverlappedSection = OverlapsWithSections(Sections);

			if (OverlappedSection == nullptr)
			{
				break;
			}

			TSet<FKeyHandle> KeyHandles;
			MoveSection(OverlappedSection->GetEndTime() - StartTime, KeyHandles);
		}
	}
}

void UMovieSceneSection::AddKeyToCurve( FRichCurve& InCurve, float Time, float Value, FKeyParams KeyParams, const bool bUnwindRotation )
{
	if(IsTimeWithinSection(Time))
	{
		Modify();
		if (InCurve.GetNumKeys() == 0 && !KeyParams.bAddKeyEvenIfUnchanged)
		{
			InCurve.SetDefaultValue(Value);
		}
		else
		{
			FKeyHandle ExistingKeyHandle = InCurve.FindKey(Time);
			
			FKeyHandle NewKeyHandle = InCurve.UpdateOrAddKey(Time, Value, bUnwindRotation);

			if (!InCurve.IsKeyHandleValid(ExistingKeyHandle) && InCurve.IsKeyHandleValid(NewKeyHandle))
			{
				// Set the default key interpolation only if a new key was created
				switch (KeyParams.KeyInterpolation)
				{
					case MSKI_Auto:
						InCurve.SetKeyInterpMode(NewKeyHandle, RCIM_Cubic);
						InCurve.SetKeyTangentMode(NewKeyHandle, RCTM_Auto);
						break;
					case MSKI_User:
						InCurve.SetKeyInterpMode(NewKeyHandle, RCIM_Cubic);
						InCurve.SetKeyTangentMode(NewKeyHandle, RCTM_User);
						break;
					case MSKI_Break:
						InCurve.SetKeyInterpMode(NewKeyHandle, RCIM_Cubic);
						InCurve.SetKeyTangentMode(NewKeyHandle, RCTM_Break);
						break;
					case MSKI_Linear:
						InCurve.SetKeyInterpMode(NewKeyHandle, RCIM_Linear);
						InCurve.SetKeyTangentMode(NewKeyHandle, RCTM_Auto);
						break;
					case MSKI_Constant:
						InCurve.SetKeyInterpMode(NewKeyHandle, RCIM_Constant);
						InCurve.SetKeyTangentMode(NewKeyHandle, RCTM_Auto);
						break;
					default:
						InCurve.SetKeyInterpMode(NewKeyHandle, RCIM_Cubic);
						InCurve.SetKeyTangentMode(NewKeyHandle, RCTM_Auto);
						break;
				}
			}
		}
	}
}