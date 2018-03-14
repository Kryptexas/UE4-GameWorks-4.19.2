// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneEvaluationTemplate.h"
#include "MovieSceneSequence.h"
#include "Sections/MovieSceneSubSection.h"

FMovieSceneSubSectionData::FMovieSceneSubSectionData(UMovieSceneSubSection& InSubSection, const FGuid& InObjectBindingId, ESectionEvaluationFlags InFlags)
	: Section(&InSubSection), ObjectBindingId(InObjectBindingId), Flags(InFlags)
{}

FMovieSceneTrackIdentifier FMovieSceneTemplateGenerationLedger::FindTrack(const FGuid& InSignature) const
{
	return TrackSignatureToTrackIdentifier.FindRef(InSignature);
}

void FMovieSceneTemplateGenerationLedger::AddTrack(const FGuid& InSignature, FMovieSceneTrackIdentifier Identifier)
{
	ensure(!TrackSignatureToTrackIdentifier.Contains(InSignature));
	TrackSignatureToTrackIdentifier.Add(InSignature, Identifier);
}

void FMovieSceneEvaluationTemplate::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading())
	{
		for (auto& Pair : Tracks)
		{
			if (TemplateLedger.LastTrackIdentifier == FMovieSceneTrackIdentifier::Invalid() || TemplateLedger.LastTrackIdentifier.Value < Pair.Key.Value)
			{
				// Reset previously serialized, invalid data
				*this = FMovieSceneEvaluationTemplate();
				break;
			}
		}
	}
}

void FMovieSceneEvaluationTemplate::ResetFieldData()
{
	TrackFieldData.Field.Reset();
	SubSectionFieldData.Field.Reset();
}

const TMovieSceneEvaluationTree<FMovieSceneTrackIdentifier>& FMovieSceneEvaluationTemplate::GetTrackField() const
{
	return TrackFieldData.Field;
}

const TMovieSceneEvaluationTree<FMovieSceneSubSectionData>& FMovieSceneEvaluationTemplate::GetSubSectionField() const
{
	return SubSectionFieldData.Field;
}

void FMovieSceneEvaluationTemplate::AddSubSectionRange(UMovieSceneSubSection& InSubSection, const FGuid& InObjectBindingId, const TRange<float>& InRange, ESectionEvaluationFlags InFlags)
{
	if (!ensure(!InSubSection.IsInfinite()))
	{
		// @todo: Infinite sub sections??
		return;
	}

	// Add the sub section to the field, but we don't invalidate the evaluation field unless we know the section has actually changed
	SubSectionFieldData.Field.Add(InRange, FMovieSceneSubSectionData(InSubSection, InObjectBindingId, InFlags));

	// Don't need to do anything else if the section was already generated
	if (!TemplateLedger.ContainsSubSection(InSubSection.GetSignature()))
	{
		if (InSubSection.GetSequence())
		{
			FMovieSceneSequenceID SubSequenceID = InSubSection.GetSequenceID();
			FSubSequenceInstanceDataParams InstanceParams{ SubSequenceID, FMovieSceneEvaluationOperand(MovieSceneSequenceID::Root, InObjectBindingId) };

			FMovieSceneSubSequenceData NewSubData = InSubSection.GenerateSubSequenceData(InstanceParams);

			Hierarchy.Add(NewSubData, SubSequenceID, MovieSceneSequenceID::Root);
		}

		TRange<float> EntireSectionRange(InSubSection.GetStartTime() - InSubSection.GetPreRollTime(), TRangeBound<float>::Inclusive(InSubSection.GetEndTime() + InSubSection.GetPostRollTime()));

		// Add the section to the ledger
		TemplateLedger.SubSectionRanges.Add(InSubSection.GetSignature(), EntireSectionRange);

		// Invalidate the overlapping field
		EvaluationField.Invalidate(EntireSectionRange);
	}
}

FMovieSceneTrackIdentifier FMovieSceneEvaluationTemplate::AddTrack(const FGuid& InSignature, FMovieSceneEvaluationTrack&& InTrack)
{
	FMovieSceneTrackIdentifier NewIdentifier = ++TemplateLedger.LastTrackIdentifier;

	InTrack.SetupOverrides();
	Tracks.Add(NewIdentifier, MoveTemp(InTrack));
	TemplateLedger.AddTrack(InSignature, NewIdentifier);

	// Add this track's segments to the unsorted track field, invalidating anything in the compiled evaluation field
	DefineTrackStructure(NewIdentifier, true);

	return NewIdentifier;
}

void FMovieSceneEvaluationTemplate::DefineTrackStructure(FMovieSceneTrackIdentifier TrackIdentifier, bool bInvalidateEvaluationField)
{
	const FMovieSceneEvaluationTrack* Track = FindTrack(TrackIdentifier);
	if (!ensure(Track))
	{
		return;
	}

	FMovieSceneTrackSegmentBlenderPtr TrackBlender = Track->GetSourceTrack()->GetTrackSegmentBlender();
	const bool bAddEmptySpace = TrackBlender.IsValid() && TrackBlender->CanFillEmptySpace();

	if (bAddEmptySpace && bInvalidateEvaluationField)
	{
		// Optimization - when tracks can add empty space, we just invalidate the entire field once
		EvaluationField.Invalidate(TRange<float>::All());
		bInvalidateEvaluationField = false;
	}

	// Add each range
	for (FMovieSceneEvaluationTreeRangeIterator It(Track->Iterate()); It; ++It)
	{
		const bool bShouldAddEntry = bAddEmptySpace || Track->GetData(It.Node());
		if (!bShouldAddEntry)
		{
			continue;
		}

		TrackFieldData.Field.Add(It.Range(), TrackIdentifier);

		if (bInvalidateEvaluationField)
		{
			EvaluationField.Invalidate(It.Range());
		}
	}
}

void FMovieSceneEvaluationTemplate::RemoveTrack(const FGuid& InSignature)
{
	FMovieSceneTrackIdentifier TrackIdentifier = TemplateLedger.FindTrack(InSignature);
	if (TrackIdentifier == FMovieSceneTrackIdentifier::Invalid())
	{
		return;
	}

	FMovieSceneEvaluationTrack* Track = Tracks.Find(TrackIdentifier);
	if (Track)
	{
		// Invalidate any ranges occupied by this track
		for (FMovieSceneEvaluationTreeRangeIterator It = Track->Iterate(); It; ++It)
		{
			if (Track->GetData(It.Node()))
			{
				EvaluationField.Invalidate(It.Range());
			}
		}

		StaleTracks.Add(TrackIdentifier, MoveTemp(*Track));
	}

	Tracks.Remove(TrackIdentifier);
	TemplateLedger.TrackSignatureToTrackIdentifier.Remove(InSignature);
}

void FMovieSceneEvaluationTemplate::RemoveStaleData(const TSet<FGuid>& ActiveSignatures)
{
	{
		TArray<FGuid> SignaturesToRemove;

		// Go through the template ledger, and remove anything that is no longer referenced
		for (auto& Pair : TemplateLedger.TrackSignatureToTrackIdentifier)
		{
			if (!ActiveSignatures.Contains(Pair.Key))
			{
				SignaturesToRemove.Add(Pair.Key);
			}
		}

		// Remove the signatures, updating entries in the evaluation field as we go
		for (const FGuid& Signature : SignaturesToRemove)
		{
			RemoveTrack(Signature);
		}
	}

	// Remove stale sub sections
	{
		TArray<TTuple<FGuid, FFloatRange>> SubSectionsToRemove;

		for (const TTuple<FGuid, FFloatRange>& Pair : TemplateLedger.SubSectionRanges)
		{
			if (!ActiveSignatures.Contains(Pair.Key))
			{
				SubSectionsToRemove.Add(Pair);
			}
		}

		for (const TTuple<FGuid, FFloatRange>& Pair : SubSectionsToRemove)
		{
			TemplateLedger.SubSectionRanges.Remove(Pair.Key);
			EvaluationField.Invalidate(Pair.Value);
		}
	}
}

const TMap<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack>& FMovieSceneEvaluationTemplate::GetTracks() const
{
	return Tracks;
}

TMap<FMovieSceneTrackIdentifier, FMovieSceneEvaluationTrack>& FMovieSceneEvaluationTemplate::GetTracks()
{
	return Tracks;
}
