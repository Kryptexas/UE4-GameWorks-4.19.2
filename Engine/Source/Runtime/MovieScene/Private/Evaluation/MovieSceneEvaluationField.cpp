// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneEvaluationField.h"
#include "Evaluation/MovieSceneEvaluationTemplateInstance.h"
#include "Evaluation/MovieSceneSequenceTemplateStore.h"
#include "MovieSceneCommonHelpers.h"
#include "Algo/Sort.h"

#include "MovieSceneSequence.h"

int32 FMovieSceneEvaluationField::GetSegmentFromTime(float Time) const
{
	// Linear search
	// @todo: accelerated search based on the last evaluated index?
	for (int32 Index = 0; Index < Ranges.Num(); ++Index)
	{
		if (Ranges[Index].Contains(Time))
		{
			return Index;
		}
	}

	return INDEX_NONE;
}

TRange<int32> FMovieSceneEvaluationField::OverlapRange(TRange<float> Range) const
{
	int32 StartIndex = 0, Num = 0;
	for (int32 Index = 0; Index < Ranges.Num(); ++Index)
	{
		if (Ranges[Index].Overlaps(Range))
		{
			if (Num == 0)
			{
				StartIndex = Index;
			}
			++Num;
		}
		else if (Num != 0)
		{
			break;
		}
	}

	return Num != 0 ? TRange<int32>(StartIndex, StartIndex + Num) : TRange<int32>::Empty();
}

void FMovieSceneEvaluationField::Invalidate(TRange<float> Range)
{
	TRange<int32> OverlappingRange = OverlapRange(Range);
	if (!OverlappingRange.IsEmpty())
	{
		Ranges.RemoveAt(OverlappingRange.GetLowerBoundValue(), OverlappingRange.Size<int32>(), false);
		Groups.RemoveAt(OverlappingRange.GetLowerBoundValue(), OverlappingRange.Size<int32>(), false);
		MetaData.RemoveAt(OverlappingRange.GetLowerBoundValue(), OverlappingRange.Size<int32>(), false);

		Signature = FGuid::NewGuid();
	}
}

int32 FMovieSceneEvaluationField::Insert(float InsertTime, TRange<float> InRange, FMovieSceneEvaluationGroup&& InGroup, FMovieSceneEvaluationMetaData&& InMetaData)
{
	const int32 InsertIndex = Algo::UpperBoundBy(Ranges, TRangeBound<float>(InsertTime), &TRange<float>::GetLowerBound, MovieSceneHelpers::SortLowerBounds);

	// Intersect the supplied range with the allowable space between adjacent existing ranges
	TRange<float> InsertSpace(
		Ranges.IsValidIndex(InsertIndex-1) ? TRangeBound<float>::FlipInclusion(Ranges[InsertIndex-1].GetUpperBound()) : TRangeBound<float>::Open(),
		Ranges.IsValidIndex(InsertIndex  ) ? TRangeBound<float>::FlipInclusion(Ranges[InsertIndex  ].GetLowerBound()) : TRangeBound<float>::Open()
		);

	InRange = TRange<float>::Intersection(InRange, InsertSpace);

	if (!ensure(!InRange.IsEmpty()))
	{
		return INDEX_NONE;
	}

	const bool bOverlapping = 
		(Ranges.IsValidIndex(InsertIndex  ) && Ranges[InsertIndex  ].Overlaps(InRange)) ||
		(Ranges.IsValidIndex(InsertIndex-1) && Ranges[InsertIndex-1].Overlaps(InRange));

	if (!ensureAlwaysMsgf(!bOverlapping, TEXT("Attempting to insert an overlapping range into the evaluation field.")))
	{
		return INDEX_NONE;
	}

	Ranges.Insert(InRange, InsertIndex);
	MetaData.Insert(MoveTemp(InMetaData), InsertIndex);
	Groups.Insert(MoveTemp(InGroup), InsertIndex);

	Signature = FGuid::NewGuid();

	return InsertIndex;
}

void FMovieSceneEvaluationField::Add(TRange<float> InRange, FMovieSceneEvaluationGroup&& InGroup, FMovieSceneEvaluationMetaData&& InMetaData)
{
	if (ensureAlwaysMsgf(!Ranges.Num() || !Ranges.Last().Overlaps(InRange), TEXT("Attempting to add overlapping ranges to sequence evaluation field.")))
	{
		Ranges.Add(InRange);
		MetaData.Add(MoveTemp(InMetaData));
		Groups.Add(MoveTemp(InGroup));

		Signature = FGuid::NewGuid();
	}
}

void FMovieSceneEvaluationMetaData::DiffSequences(const FMovieSceneEvaluationMetaData& LastFrame, TArray<FMovieSceneSequenceID>* NewSequences, TArray<FMovieSceneSequenceID>* ExpiredSequences) const
{
	// This algorithm works on the premise that each array is sorted, and each ID can only appear once
	auto ThisFrameIDs = ActiveSequences.CreateConstIterator();
	auto LastFrameIDs = LastFrame.ActiveSequences.CreateConstIterator();

	// Iterate both arrays together
	while (ThisFrameIDs && LastFrameIDs)
	{
		FMovieSceneSequenceID ThisID = *ThisFrameIDs;
		FMovieSceneSequenceID LastID = *LastFrameIDs;

		// If they're the same, skip
		if (ThisID == LastID)
		{
			++ThisFrameIDs;
			++LastFrameIDs;
			continue;
		}

		if (LastID < ThisID)
		{
			// Last frame iterator is less than this frame's, which means the last ID is no longer evaluated
			if (ExpiredSequences)
			{
				ExpiredSequences->Add(LastID);
			}
			++LastFrameIDs;
		}
		else
		{
			// Last frame iterator is greater than this frame's, which means this ID is new
			if (NewSequences)
			{
				NewSequences->Add(ThisID);
			}

			++ThisFrameIDs;
		}
	}

	// Add any remaining expired sequences
	if (ExpiredSequences)
	{
		while (LastFrameIDs)
		{
			ExpiredSequences->Add(*LastFrameIDs);
			++LastFrameIDs;
		}
	}

	// Add any remaining new sequences
	if (NewSequences)
	{
		while (ThisFrameIDs)
		{
			NewSequences->Add(*ThisFrameIDs);
			++ThisFrameIDs;
		}
	}
}

void FMovieSceneEvaluationMetaData::DiffEntities(const FMovieSceneEvaluationMetaData& LastFrame, TArray<FMovieSceneOrderedEvaluationKey>* NewKeys, TArray<FMovieSceneOrderedEvaluationKey>* ExpiredKeys) const
{
	// This algorithm works on the premise that each array is sorted, and each ID can only appear once
	auto ThisFrameKeys = ActiveEntities.CreateConstIterator();
	auto LastFrameKeys = LastFrame.ActiveEntities.CreateConstIterator();

	// Iterate both arrays together
	while (ThisFrameKeys && LastFrameKeys)
	{
		FMovieSceneOrderedEvaluationKey ThisKey = *ThisFrameKeys;
		FMovieSceneOrderedEvaluationKey LastKey = *LastFrameKeys;

		// If they're the same, skip
		if (ThisKey.Key == LastKey.Key)
		{
			++ThisFrameKeys;
			++LastFrameKeys;
			continue;
		}

		if (LastKey.Key < ThisKey.Key)
		{
			// Last frame iterator is less than this frame's, which means the last entity is no longer evaluated
			if (ExpiredKeys)
			{
				ExpiredKeys->Add(LastKey);
			}
			++LastFrameKeys;
		}
		else
		{
			// Last frame iterator is greater than this frame's, which means this entity is new
			if (NewKeys)
			{
				NewKeys->Add(ThisKey);
			}

			++ThisFrameKeys;
		}
	}

	// Add any remaining expired entities
	if (ExpiredKeys)
	{
		while (LastFrameKeys)
		{
			ExpiredKeys->Add(*LastFrameKeys);
			++LastFrameKeys;
		}

		// Expired keys are torn down in reverse
		Algo::SortBy(*ExpiredKeys, [](const FMovieSceneOrderedEvaluationKey& In){ return uint32(-1) - In.EvaluationIndex; });
	}

	// Add any remaining new entities
	if (NewKeys)
	{
		while (ThisFrameKeys)
		{
			NewKeys->Add(*ThisFrameKeys);
			++ThisFrameKeys;
		}

		Algo::SortBy(*NewKeys, &FMovieSceneOrderedEvaluationKey::EvaluationIndex);
	}
}

bool FMovieSceneEvaluationMetaData::IsDirty(UMovieSceneSequence& RootSequence, const FMovieSceneSequenceHierarchy& RootHierarchy, IMovieSceneSequenceTemplateStore& TemplateStore, TRange<float>* OutSubRangeToInvalidate) const
{
	bool bDirty = false;

	for (const TTuple<FMovieSceneSequenceID, FGuid>& Pair : SubSequenceSignatures)
	{
		// Sequence IDs at this point are relative to the root override template
		const FMovieSceneSubSequenceData* SubData = RootHierarchy.FindSubData(Pair.Key);
		UMovieSceneSequence* SubSequence = SubData ? SubData->GetSequence() : nullptr;

		bool bThisSequenceIsDirty = true;
		if (SubSequence)
		{
			FGuid SequenceSignature = SubSequence->GetSignature();
			bThisSequenceIsDirty = SequenceSignature != Pair.Value || TemplateStore.AccessTemplate(*SubSequence).SequenceSignature != SequenceSignature;
		}

		if (bThisSequenceIsDirty)
		{
			bDirty = true;

			if (OutSubRangeToInvalidate)
			{
				TRange<float> DirtyRange = SubData ? TRange<float>::Hull(TRange<float>::Hull(SubData->PreRollRange, SubData->PlayRange), SubData->PostRollRange) * SubData->RootToSequenceTransform.Inverse() : TRange<float>::All();
				*OutSubRangeToInvalidate = TRange<float>::Hull(*OutSubRangeToInvalidate, DirtyRange);
			}
		}
	}

	return bDirty;
}
