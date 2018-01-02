// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Compilation/MovieSceneEvaluationTemplateGenerator.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"
#include "Algo/Sort.h"
#include "UObject/ObjectKey.h"
#include "IMovieSceneModule.h"
#include "MovieSceneSubTrack.h"
#include "MovieSceneSubSection.h"
#include "MovieSceneSegmentCompiler.h"

FMovieSceneEvaluationTemplateGenerator::FMovieSceneEvaluationTemplateGenerator(UMovieSceneSequence& InSequence, FMovieSceneEvaluationTemplate& OutTemplate)
	: SourceSequence(InSequence), Template(OutTemplate)
{

}

void FMovieSceneEvaluationTemplateGenerator::AddOwnedTrack(FMovieSceneEvaluationTrack&& InTrackTemplate, const UMovieSceneTrack& SourceTrack)
{
	CompiledSignatures.Add(SourceTrack.GetSignature());
	Template.AddTrack(SourceTrack.GetSignature(), MoveTemp(InTrackTemplate));
}


void FMovieSceneEvaluationTemplateGenerator::Generate()
{
	// We always regenerate this
	Template.ResetFieldData();

	CompiledSignatures.Reset();

	UMovieScene* MovieScene = SourceSequence.GetMovieScene();

	if (UMovieSceneTrack* Track = MovieScene->GetCameraCutTrack())
	{
		ProcessTrack(*Track);
	}

	for (UMovieSceneTrack* Track : MovieScene->GetMasterTracks())
	{
		ProcessTrack(*Track);
	}

	for (const FMovieSceneBinding& ObjectBinding : MovieScene->GetBindings())
	{
		for (UMovieSceneTrack* Track : ObjectBinding.GetTracks())
		{
			ProcessTrack(*Track, ObjectBinding.GetObjectGuid());
		}
	}

	Template.RemoveStaleData(CompiledSignatures);

	Template.SequenceSignature = SourceSequence.GetSignature();
}

void FMovieSceneEvaluationTemplateGenerator::ProcessTrack(const UMovieSceneTrack& Track, const FGuid& ObjectBindingId)
{
	FMovieSceneTrackSegmentBlenderPtr TrackBlender = Track.GetTrackSegmentBlender();

	// Deal with sub tracks specifically
	if (const UMovieSceneSubTrack* SubTrack = Cast<const UMovieSceneSubTrack>(&Track))
	{
		ProcessSubTrack(*SubTrack, ObjectBindingId);
	}

	// If the ledger already contains this track, just update the uncompiled track field
	else if (FMovieSceneTrackIdentifier TrackID = Template.GetLedger().FindTrack(Track.GetSignature()))
	{
		CompiledSignatures.Add(Track.GetSignature());

		// Don't invalidate the evaluation field if for this track as this track hasn't changed
		const bool bInvalidateEvaluationField = false;
		Template.DefineTrackStructure(TrackID, bInvalidateEvaluationField);
	}

	// Else the track doesn't exist - we need to generate it from scratch
	else
	{
		FMovieSceneTrackCompilerArgs Args(*this);
		Args.ObjectBindingId = ObjectBindingId;
		Args.DefaultCompletionMode = SourceSequence.DefaultCompletionMode;
		Track.GenerateTemplate(Args);
	}
}


void FMovieSceneEvaluationTemplateGenerator::ProcessSubTrack(const UMovieSceneSubTrack& SubTrack, const FGuid& ObjectBindingId)
{
	// Get the segment blender required to blend at the track level for this segment
	FMovieSceneTrackSegmentBlenderPtr    TrackSegmentBlender = SubTrack.GetTrackSegmentBlender();
	FMovieSceneTrackRowSegmentBlenderPtr RowSegmentBlender   = SubTrack.GetRowSegmentBlender();

	// If the track has no blenders at all, we can just add all the sub section ranges directly
	const bool bRequiresSegmentBlending = TrackSegmentBlender.IsValid() || RowSegmentBlender.IsValid();

	TMovieSceneEvaluationTree<FSectionEvaluationData> SubSectionBlendTree;

	// Iterate all the sub track's sections, adding them to the template field
	const TArray<UMovieSceneSection*>& AllSections = SubTrack.GetAllSections();
	for (int32 SectionIndex = 0; SectionIndex < AllSections.Num(); ++SectionIndex)
	{
		UMovieSceneSubSection* SubSection = CastChecked<UMovieSceneSubSection>(AllSections[SectionIndex]);
		if (!SubSection || !SubSection->IsActive())
		{
			continue;
		}

		CompiledSignatures.Add(SubSection->GetSignature());

		if (bRequiresSegmentBlending)
		{
			const TRange<float> SectionRange = SubSection->GetRange();

			// Process the actual section range
			if (bRequiresSegmentBlending)
			{
				SubSectionBlendTree.Add(SectionRange, FSectionEvaluationData(SectionIndex, ESectionEvaluationFlags::None));
			}
			else
			{
				Template.AddSubSectionRange(*SubSection, ObjectBindingId, SectionRange, ESectionEvaluationFlags::None);
			}

			// Process the section preroll range
			if (!SectionRange.GetLowerBound().IsOpen() && SubSection->GetPreRollTime() > 0)
			{
				TRange<float> PreRollRange(SectionRange.GetLowerBoundValue() - SubSection->GetPreRollTime(), TRangeBound<float>::FlipInclusion(SectionRange.GetLowerBoundValue()));

				if (bRequiresSegmentBlending)
				{
					SubSectionBlendTree.Add(PreRollRange, FSectionEvaluationData(SectionIndex, ESectionEvaluationFlags::PreRoll));
				}
				else
				{
					Template.AddSubSectionRange(*SubSection, ObjectBindingId, PreRollRange, ESectionEvaluationFlags::PreRoll);
				}
			}

			// Process the section postroll range
			if (!SectionRange.GetUpperBound().IsOpen() && SubSection->GetPostRollTime() > 0)
			{
				TRange<float> PostRollRange(TRangeBound<float>::FlipInclusion(SectionRange.GetUpperBoundValue()), SectionRange.GetUpperBoundValue() + SubSection->GetPostRollTime());

				if (bRequiresSegmentBlending)
				{
					SubSectionBlendTree.Add(PostRollRange, FSectionEvaluationData(SectionIndex, ESectionEvaluationFlags::PostRoll));
				}
				else
				{
					Template.AddSubSectionRange(*SubSection, ObjectBindingId, PostRollRange, ESectionEvaluationFlags::PostRoll);
				}
			}
		}
	}

	// Nothing more to do if we don't need to blend the segments
	if (!bRequiresSegmentBlending)
	{
		return;
	}

	// Keep accumulating similar segments to reduce fragmentation in the field
	TOptional<FMovieSceneSegment> AccumulateSegment;

	TMap<int32, FSegmentBlendData> RowBlendData;

	// Go through each unique range within SubSectionBlendTree, blending any entities present appropriately, then adding them to the tree
	for (FMovieSceneEvaluationTreeRangeIterator SubSectionIt(SubSectionBlendTree); SubSectionIt; ++SubSectionIt)
	{
		TMovieSceneEvaluationTreeDataIterator<FSectionEvaluationData> DataIt = SubSectionBlendTree.GetAllData(SubSectionIt.Node());

		// If there are no sub sections here, continue to the next range
		if (!DataIt)
		{
			continue;
		}

		FSegmentBlendData TrackBlendData;

		// Compile at the row level first if necessary
		if (RowSegmentBlender.IsValid())
		{
			RowBlendData.Reset();

			// Add every FSectionEvaluationData for the current time range to the section data
			for (const FSectionEvaluationData& EvalData : DataIt)
			{
				const UMovieSceneSection* Section = AllSections[EvalData.ImplIndex];
				check(Section);
				RowBlendData.FindOrAdd(Section->GetRowIndex()).Add(FMovieSceneSectionData(Section, EvalData.ImplIndex, EvalData.Flags));
			}

			// Blend and append
			for (TPair<int32, FSegmentBlendData>& Pair : RowBlendData)
			{
				RowSegmentBlender->Blend(Pair.Value);
				TrackBlendData.Append(Pair.Value);
			}
		}

		// Otherwise just add everything to the track blend data
		else for (const FSectionEvaluationData& EvalData : DataIt)
		{
			const UMovieSceneSection* Section = AllSections[EvalData.ImplIndex];
			check(Section);
			TrackBlendData.Add(FMovieSceneSectionData(Section, EvalData.ImplIndex, EvalData.Flags));
		}

		// Compile at the track level
		if (TrackSegmentBlender.IsValid())
		{
			TrackSegmentBlender->Blend(TrackBlendData);
		}

		FMovieSceneSegment NextSegment(SubSectionIt.Range());
		TrackBlendData.AddToSegment(NextSegment);

		if (!AccumulateSegment.IsSet())
		{
			AccumulateSegment = MoveTemp(NextSegment);
		}
		else if (!AccumulateSegment->CombineWith(NextSegment))
		{
			// This segment is different from the previous one - add the previous one and start accumulating into this one
			for (FSectionEvaluationData EvalData : AccumulateSegment->Impls)
			{
				Template.AddSubSectionRange(*CastChecked<UMovieSceneSubSection>(AllSections[EvalData.ImplIndex]), ObjectBindingId, AccumulateSegment->Range, EvalData.Flags);
			}
			AccumulateSegment = MoveTemp(NextSegment);
		}
	}

	// If there is any segment outstanding, add it to the template
	if (AccumulateSegment.IsSet())
	{
		for (FSectionEvaluationData EvalData : AccumulateSegment->Impls)
		{
			Template.AddSubSectionRange(*CastChecked<UMovieSceneSubSection>(AllSections[EvalData.ImplIndex]), ObjectBindingId, AccumulateSegment->Range, EvalData.Flags);
		}
	}
}