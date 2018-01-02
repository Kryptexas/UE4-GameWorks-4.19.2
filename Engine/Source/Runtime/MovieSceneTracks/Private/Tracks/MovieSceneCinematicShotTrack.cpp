// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Tracks/MovieSceneCinematicShotTrack.h"
#include "MovieSceneSequence.h"
#include "MovieSceneCommonHelpers.h"
#include "Sections/MovieSceneCinematicShotSection.h"
#include "Compilation/MovieSceneCompilerRules.h"


#define LOCTEXT_NAMESPACE "MovieSceneCinematicShotTrack"


/* UMovieSceneSubTrack interface
 *****************************************************************************/
UMovieSceneCinematicShotTrack::UMovieSceneCinematicShotTrack(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(0, 0, 0, 127);
#endif
}

UMovieSceneSubSection* UMovieSceneCinematicShotTrack::AddSequence(UMovieSceneSequence* Sequence, float StartTime, float Duration, const bool& bInsertSequence)
{
	UMovieSceneSubSection* NewSection = UMovieSceneSubTrack::AddSequence(Sequence, StartTime, Duration, bInsertSequence);

	UMovieSceneCinematicShotSection* NewShotSection = Cast<UMovieSceneCinematicShotSection>(NewSection);

#if WITH_EDITOR

	if (Sequence != nullptr)
	{
		NewShotSection->SetShotDisplayName(Sequence->GetDisplayName().ToString());	
	}

#endif

	// When a new sequence is added, sort all sequences to ensure they are in the correct order
	MovieSceneHelpers::SortConsecutiveSections(Sections);

	// Once sequences are sorted fixup the surrounding sequences to fix any gaps
	//MovieSceneHelpers::FixupConsecutiveSections(Sections, *NewSection, false);

	return NewSection;
}

/* UMovieSceneTrack interface
 *****************************************************************************/

void UMovieSceneCinematicShotTrack::AddSection(UMovieSceneSection& Section)
{
	if (Section.IsA<UMovieSceneCinematicShotSection>())
	{
		Sections.Add(&Section);
	}
}


UMovieSceneSection* UMovieSceneCinematicShotTrack::CreateNewSection()
{
	return NewObject<UMovieSceneCinematicShotSection>(this, NAME_None, RF_Transactional);
}

void UMovieSceneCinematicShotTrack::RemoveSection(UMovieSceneSection& Section)
{
	Sections.Remove(&Section);
	//MovieSceneHelpers::FixupConsecutiveSections(Sections, Section, true);
	MovieSceneHelpers::SortConsecutiveSections(Sections);

	// @todo Sequencer: The movie scene owned by the section is now abandoned.  Should we offer to delete it?  
}

bool UMovieSceneCinematicShotTrack::SupportsMultipleRows() const
{
	return true;
}

FMovieSceneTrackSegmentBlenderPtr UMovieSceneCinematicShotTrack::GetTrackSegmentBlender() const
{
	// Apply a high pass filter to overlapping sections such that only the highest row in a track wins
	struct FCinematicShotTrackRowBlender : FMovieSceneTrackSegmentBlender
	{
		virtual void Blend(FSegmentBlendData& BlendData) const override
		{
			MovieSceneSegmentCompiler::ChooseLowestRowIndex(BlendData);
		}
	};
	return FCinematicShotTrackRowBlender();
}

FMovieSceneTrackRowSegmentBlenderPtr UMovieSceneCinematicShotTrack::GetRowSegmentBlender() const
{
	class FCinematicRowRules : public FMovieSceneTrackRowSegmentBlender
	{
		virtual void Blend(FSegmentBlendData& BlendData) const override
		{
			// Sort everything by priority, then latest start time wins
			if (BlendData.Num() <= 1)
			{
				return;
			}

			BlendData.Sort(SortPredicate);

			int32 RemoveAtIndex = 0;
			// Skip over any pre/postroll sections
			while (BlendData.IsValidIndex(RemoveAtIndex) && EnumHasAnyFlags(BlendData[RemoveAtIndex].Flags, ESectionEvaluationFlags::PreRoll | ESectionEvaluationFlags::PostRoll))
			{
				++RemoveAtIndex;
			}

			// Skip over the first genuine evaluation if it exists
			++RemoveAtIndex;

			int32 NumToRemove = BlendData.Num() - RemoveAtIndex;
			if (NumToRemove > 0)
			{
				BlendData.RemoveAt(RemoveAtIndex, NumToRemove, true);
			}
		}

		static bool SortPredicate(const FMovieSceneSectionData& A, const FMovieSceneSectionData& B)
		{
			// Always sort pre/postroll to the front of the array
			const bool PrePostRollA = EnumHasAnyFlags(A.Flags, ESectionEvaluationFlags::PreRoll | ESectionEvaluationFlags::PostRoll);
			const bool PrePostRollB = EnumHasAnyFlags(B.Flags, ESectionEvaluationFlags::PreRoll | ESectionEvaluationFlags::PostRoll);

			if (PrePostRollA != PrePostRollB)
			{
				return PrePostRollA;
			}
			else if (PrePostRollA)
			{
				return false;
			}
			else if (A.Section->GetOverlapPriority() == B.Section->GetOverlapPriority())
			{
				TRangeBound<float> StartBoundA = A.Section->IsInfinite() ? TRangeBound<float>::Open() : TRangeBound<float>::Inclusive(A.Section->GetStartTime());
				TRangeBound<float> StartBoundB = B.Section->IsInfinite() ? TRangeBound<float>::Open() : TRangeBound<float>::Inclusive(B.Section->GetStartTime());

				return TRangeBound<float>::MaxLower(StartBoundA, StartBoundB) == StartBoundA;
			}
			return A.Section->GetOverlapPriority() > B.Section->GetOverlapPriority();
		}
	};

	return FCinematicRowRules();
}

#if WITH_EDITOR
void UMovieSceneCinematicShotTrack::OnSectionMoved(UMovieSceneSection& Section)
{
	//MovieSceneHelpers::FixupConsecutiveSections(Sections, Section, false);
}
#endif

#if WITH_EDITORONLY_DATA
FText UMovieSceneCinematicShotTrack::GetDefaultDisplayName() const
{
	return LOCTEXT("TrackName", "Shots");
}
#endif

#undef LOCTEXT_NAMESPACE
