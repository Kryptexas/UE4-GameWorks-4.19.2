// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneTrack.h"
#include "MovieScene.h"
#include "Evaluation/MovieSceneSegment.h"
#include "Compilation/MovieSceneSegmentCompiler.h"
#include "Compilation/MovieSceneCompilerRules.h"
#include "Compilation/IMovieSceneTemplateGenerator.h"

#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "Evaluation/MovieSceneEvaluationTemplate.h"

#include "MovieSceneEvaluationCustomVersion.h"

UMovieSceneTrack::UMovieSceneTrack(const FObjectInitializer& InInitializer)
	: Super(InInitializer)
{
#if WITH_EDITORONLY_DATA
	TrackTint = FColor(127, 127, 127, 0);
#endif
}

void UMovieSceneTrack::PostInitProperties()
{
	// Propagate sub object flags from our outer (movie scene) to ourselves. This is required for tracks that are stored on blueprints (archetypes) so that they can be referenced in worlds.
	if (GetOuter()->HasAnyFlags(RF_ClassDefaultObject|RF_ArchetypeObject))
	{
		SetFlags(GetOuter()->GetMaskedFlags(RF_PropagateToSubObjects));
	}
	
	Super::PostInitProperties();
}

void UMovieSceneTrack::PostLoad()
{
	Super::PostLoad();

	if (GetLinkerCustomVersion(FMovieSceneEvaluationCustomVersion::GUID) < FMovieSceneEvaluationCustomVersion::ChangeEvaluateNearestSectionDefault)
	{
		EvalOptions.bEvalNearestSection = EvalOptions.bEvaluateNearestSection_DEPRECATED;
	}
}

void UMovieSceneTrack::UpdateEasing()
{
	int32 MaxRows = GetMaxRowIndex();
	TArray<UMovieSceneSection*> RowSections;

	for (int32 RowIndex = 0; RowIndex <= MaxRows; ++RowIndex)
	{
		RowSections.Reset();

		for (UMovieSceneSection* Section : GetAllSections())
		{
			if (Section && Section->GetRowIndex() == RowIndex)
			{
				RowSections.Add(Section);
			}
		}

		for (int32 Index = 0; Index < RowSections.Num(); ++Index)
		{
			UMovieSceneSection* CurrentSection = RowSections[Index];

			// Check overlaps with exclusive ranges so that sections can butt up against each other
			UMovieSceneTrack* OuterTrack = CurrentSection->GetTypedOuter<UMovieSceneTrack>();
			float MaxEaseIn = 0.f;
			float MaxEaseOut = 0.f;
			bool bIsEntirelyUnderlapped = false;

			TRange<float> CurrentSectionRange = CurrentSection->GetRange();
			for (int32 OtherIndex = 0; OtherIndex < RowSections.Num(); ++OtherIndex)
			{
				if (OtherIndex == Index)
				{
					continue;
				}

				UMovieSceneSection* Other = RowSections[OtherIndex];
				TRange<float> OtherSectionRange = Other->GetRange();

				bIsEntirelyUnderlapped = OtherSectionRange.Contains(CurrentSectionRange);

				// Check the lower bound of the current section against the other section's upper bound
				const bool bSectionRangeContainsOtherUpperBound = !OtherSectionRange.GetUpperBound().IsOpen() && !CurrentSectionRange.GetLowerBound().IsOpen() && CurrentSectionRange.Contains(OtherSectionRange.GetUpperBoundValue());
				const bool bSectionRangeContainsOtherLowerBound = !OtherSectionRange.GetLowerBound().IsOpen() && !CurrentSectionRange.GetUpperBound().IsOpen() && CurrentSectionRange.Contains(OtherSectionRange.GetLowerBoundValue());
				if (bSectionRangeContainsOtherUpperBound && !bSectionRangeContainsOtherLowerBound)
				{
					MaxEaseIn = FMath::Max(MaxEaseIn, OtherSectionRange.GetUpperBoundValue() - CurrentSectionRange.GetLowerBoundValue());
				}

				if (bSectionRangeContainsOtherLowerBound &&!bSectionRangeContainsOtherUpperBound)
				{
					MaxEaseOut = FMath::Max(MaxEaseOut, CurrentSectionRange.GetUpperBoundValue() - OtherSectionRange.GetLowerBoundValue());
				}
			}

			const bool bIsFinite = CurrentSectionRange.HasLowerBound() && CurrentSectionRange.HasUpperBound();
			const float Max = bIsFinite ? CurrentSectionRange.Size<float>() : TNumericLimits<float>::Max();

			if (MaxEaseOut == 0.f && MaxEaseIn == 0.f && bIsEntirelyUnderlapped)
			{
				MaxEaseOut = MaxEaseIn = Max * 0.25f;
			}

			// Only modify the section if the ease in or out times have actually changed
			MaxEaseIn = FMath::Clamp(MaxEaseIn, 0.f, Max);
			MaxEaseOut = FMath::Clamp(MaxEaseOut, 0.f, Max);

			if (CurrentSection->Easing.AutoEaseInTime != MaxEaseIn || CurrentSection->Easing.AutoEaseOutTime != MaxEaseOut)
			{
				CurrentSection->Modify();

				CurrentSection->Easing.AutoEaseInTime = MaxEaseIn;
				CurrentSection->Easing.AutoEaseOutTime = MaxEaseOut;
			}
		}
	}
}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
TInlineValue<FMovieSceneSegmentCompilerRules> UMovieSceneTrack::GetRowCompilerRules() const
{
	return TInlineValue<FMovieSceneSegmentCompilerRules>();
}

TInlineValue<FMovieSceneSegmentCompilerRules> UMovieSceneTrack::GetTrackCompilerRules() const
{
	return TInlineValue<FMovieSceneSegmentCompilerRules>();
}

FMovieSceneTrackRowSegmentBlenderPtr UMovieSceneTrack::GetRowSegmentBlender() const
{
	// Handle legacy row compiler rules
	TInlineValue<FMovieSceneSegmentCompilerRules> LegacyRules = GetRowCompilerRules();
	if (LegacyRules.IsValid())
	{
		return TLegacyTrackRowSegmentBlender<FMovieSceneTrackRowSegmentBlender>(MoveTemp(LegacyRules));
	}
	else
	{
		return FDefaultTrackRowSegmentBlender();
	}
}

FMovieSceneTrackSegmentBlenderPtr UMovieSceneTrack::GetTrackSegmentBlender() const
{
	// Handle legacy track compiler rules
	TInlineValue<FMovieSceneSegmentCompilerRules> LegacyRules = GetTrackCompilerRules();
	if (LegacyRules.IsValid())
	{
		return TLegacyTrackRowSegmentBlender<FMovieSceneTrackSegmentBlender>(MoveTemp(LegacyRules));
	}
	else if (EvalOptions.bCanEvaluateNearestSection && EvalOptions.bEvalNearestSection)
	{
		return FEvaluateNearestSegmentBlender();
	}
	else
	{
		return FMovieSceneTrackSegmentBlenderPtr();
	}
}
PRAGMA_ENABLE_DEPRECATION_WARNINGS

void UMovieSceneTrack::GenerateTemplate(const FMovieSceneTrackCompilerArgs& Args) const
{
	FMovieSceneEvaluationTrack NewTrackTemplate(Args.ObjectBindingId);

	if (Compile(NewTrackTemplate, Args) != EMovieSceneCompileResult::Success)
	{
		return;
	}

	Args.Generator.AddOwnedTrack(MoveTemp(NewTrackTemplate), *this);
}

FMovieSceneEvaluationTrack UMovieSceneTrack::GenerateTrackTemplate() const
{
	FMovieSceneEvaluationTrack TrackTemplate = FMovieSceneEvaluationTrack(FGuid());

	// For this path, we don't have a generator, so we just pass through a stub
	struct FTemplateGeneratorStub : IMovieSceneTemplateGenerator
	{
		virtual void AddOwnedTrack(FMovieSceneEvaluationTrack&& InTrackTemplate, const UMovieSceneTrack& SourceTrack) override {}
	} Generator;

	Compile(TrackTemplate, FMovieSceneTrackCompilerArgs(Generator));

	return TrackTemplate;
}

EMovieSceneCompileResult UMovieSceneTrack::Compile(FMovieSceneEvaluationTrack& OutTrack, const FMovieSceneTrackCompilerArgs& Args) const
{
	OutTrack.SetPreAndPostrollConditions(EvalOptions.bEvaluateInPreroll, EvalOptions.bEvaluateInPostroll);

	EMovieSceneCompileResult Result = CustomCompile(OutTrack, Args);

	if (Result == EMovieSceneCompileResult::Unimplemented)
	{
		for (const UMovieSceneSection* Section : GetAllSections())
		{
			if (!Section->IsActive() || Section->GetRange().IsEmpty())
			{
				continue;
			}

			FMovieSceneEvalTemplatePtr NewTemplate = CreateTemplateForSection(*Section);
			if (NewTemplate.IsValid())
			{
				NewTemplate->SetCompletionMode(Section->EvalOptions.CompletionMode == EMovieSceneCompletionMode::ProjectDefault ? Args.DefaultCompletionMode : Section->EvalOptions.CompletionMode);
				NewTemplate->SetSourceSection(Section);

				int32 TemplateIndex = OutTrack.AddChildTemplate(MoveTemp(NewTemplate));

				const TRange<float> SectionRange = Section->IsInfinite() ? TRange<float>::All() : Section->GetRange();
				OutTrack.AddTreeData(SectionRange, FSectionEvaluationData(TemplateIndex, ESectionEvaluationFlags::None));

				if (!SectionRange.GetLowerBound().IsOpen() && Section->GetPreRollTime() > 0)
				{
					TRange<float> PreRollRange(SectionRange.GetLowerBoundValue() - Section->GetPreRollTime(), TRangeBound<float>::FlipInclusion(SectionRange.GetLowerBoundValue()));
					OutTrack.AddTreeData(PreRollRange, FSectionEvaluationData(TemplateIndex, ESectionEvaluationFlags::PreRoll));
				}

				if (!SectionRange.GetUpperBound().IsOpen() && Section->GetPostRollTime() > 0)
				{
					TRange<float> PostRollRange(TRangeBound<float>::FlipInclusion(SectionRange.GetUpperBoundValue()), SectionRange.GetUpperBoundValue() + Section->GetPostRollTime());
					OutTrack.AddTreeData(PostRollRange, FSectionEvaluationData(TemplateIndex, ESectionEvaluationFlags::PostRoll));
				}
			}
		}
		Result = EMovieSceneCompileResult::Success;
	}

	if (Result == EMovieSceneCompileResult::Success)
	{
		OutTrack.SetSourceTrack(this);
		PostCompile(OutTrack, Args);
	}

	return Result;
}

FMovieSceneEvalTemplatePtr UMovieSceneTrack::CreateTemplateForSection(const UMovieSceneSection& InSection) const
{
	return InSection.GenerateTemplate();
}

int32 UMovieSceneTrack::GetMaxRowIndex() const
{
	int32 MaxRowIndex = 0;
	for (UMovieSceneSection* Section : GetAllSections())
	{
		MaxRowIndex = FMath::Max(MaxRowIndex, Section->GetRowIndex());
	}

	return MaxRowIndex;
}

bool UMovieSceneTrack::FixRowIndices()
{
	bool bFixesMade = false;
	TArray<UMovieSceneSection*> Sections = GetAllSections();
	if (SupportsMultipleRows())
	{
		// remove any empty track rows by waterfalling down sections to be as compact as possible
		TArray<TArray<UMovieSceneSection*>> RowIndexToSectionsMap;
		RowIndexToSectionsMap.AddZeroed(GetMaxRowIndex() + 1);

		for (UMovieSceneSection* Section : Sections)
		{
			RowIndexToSectionsMap[Section->GetRowIndex()].Add(Section);
		}

		int32 NewIndex = 0;
		for (const TArray<UMovieSceneSection*>& SectionsForIndex : RowIndexToSectionsMap)
		{
			if (SectionsForIndex.Num() > 0)
			{
				for (UMovieSceneSection* SectionForIndex : SectionsForIndex)
				{
					if (SectionForIndex->GetRowIndex() != NewIndex)
					{
						SectionForIndex->Modify();
						SectionForIndex->SetRowIndex(NewIndex);
						bFixesMade = true;
					}
				}
				++NewIndex;
			}
		}
	}
	else
	{
		for (int32 i = 0; i < Sections.Num(); ++i)
		{
			if (Sections[i]->GetRowIndex() != 0)
			{
				Sections[i]->Modify();
				Sections[i]->SetRowIndex(0);
				bFixesMade = true;
			}
		}
	}
	return bFixesMade;
}
