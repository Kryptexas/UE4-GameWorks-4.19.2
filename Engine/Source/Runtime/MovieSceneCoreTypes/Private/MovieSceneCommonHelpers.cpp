// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneCoreTypesPCH.h"
#include "IMovieScenePlayer.h"

namespace MovieSceneHelpers
{
	TArray<UMovieSceneSection*> GetTraversedSections( const TArray<UMovieSceneSection*>& Sections, float CurrentTime, float PreviousTime )
	{
		TArray<UMovieSceneSection*> TraversedSections;

		bool bPlayingBackwards = CurrentTime - PreviousTime < 0.0f;
	
		float MaxTime = bPlayingBackwards ? PreviousTime : CurrentTime;
		float MinTime = bPlayingBackwards ? CurrentTime : PreviousTime;

		TRange<float> SliderRange(MinTime, MaxTime);

		for( int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex )
		{
			UMovieSceneSection* Section = Sections[SectionIndex];
			if( SliderRange.Overlaps( TRange<float>( Section->GetRange() ) ) )
			{
				TraversedSections.Add( Section );
			}
		}

		return TraversedSections;
	}

	UMovieSceneSection* FindSectionAtTime( const TArray<UMovieSceneSection*>& Sections, float Time )
	{
		// Only update the section if the position is within the time span of the section
		// Or if there are no sections at the time, the left closest section to the time
		// Or in the case that Time is before all sections, take the section with the earliest start time
		UMovieSceneSection* ClosestSection = NULL;
		float ClosestSectionTime = 0.f;
		UMovieSceneSection* EarliestSection = NULL;
		float EarliestSectionTime = 0.f;
		for( int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex )
		{
			UMovieSceneSection* Section = Sections[SectionIndex];

			//@todo Sequencer - There can be multiple sections overlapping in time. Returning instantly does not account for that.
			if( Section->IsTimeWithinSection( Time ) )
			{
				return Section;
			}

			float EndTime = Section->GetEndTime();
			if (EndTime < Time)
			{
				float ClosestTime = Time - EndTime;
				if (!ClosestSection || ClosestTime < ClosestSectionTime)
				{
					ClosestSection = Section;
					ClosestSectionTime = ClosestTime;
				}
			}

			float StartTime = Section->GetStartTime();
			if (!EarliestSection || StartTime < EarliestSectionTime)
			{
				EarliestSection = Section;
				EarliestSectionTime = StartTime;
			}
		}

		// if we get here, we are off of any section
		// if ClosestSection, then we take the closest to left of this time
		// else, we take the EarliestSection
		// if that's NULL, then there are no sections
		return ClosestSection ? ClosestSection : EarliestSection;
	}

}