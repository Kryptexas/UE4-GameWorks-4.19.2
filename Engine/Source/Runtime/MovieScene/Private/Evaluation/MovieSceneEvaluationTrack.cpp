// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Evaluation/MovieSceneEvaluationTrack.h"
#include "MovieSceneExecutionTokens.h"

#include "MovieSceneSegmentCompiler.h"
#include "MovieSceneCompilerRules.h"
#include "MovieSceneTrack.h"
#include "MovieSceneCommonHelpers.h"

#include "Algo/BinarySearch.h"


#if WITH_DEV_AUTOMATION_TESTS

	struct FGetTrackSegmentBlender
	{
		FMovieSceneTrackSegmentBlender* operator()(const UMovieSceneTrack* Track, FMovieSceneTrackSegmentBlenderPtr& OutOwnerPtr) const
		{
			if (Override.IsValid())
			{
				return &Override.GetValue();
			}
			else if (Track)
			{
				OutOwnerPtr = Track->GetTrackSegmentBlender();
			}

			return OutOwnerPtr.GetPtr();
		}

		static FMovieSceneTrackSegmentBlenderPtr Override;

	} GetTrackSegmentBlender;

	FMovieSceneTrackSegmentBlenderPtr FGetTrackSegmentBlender::Override;

	FScopedOverrideTrackSegmentBlender::FScopedOverrideTrackSegmentBlender(FMovieSceneTrackSegmentBlenderPtr&& InTrackSegmentBlender)
	{
		check(!GetTrackSegmentBlender.Override.IsValid());
		GetTrackSegmentBlender.Override = MoveTemp(InTrackSegmentBlender);
	}

	FScopedOverrideTrackSegmentBlender::~FScopedOverrideTrackSegmentBlender()
	{
		GetTrackSegmentBlender.Override.Reset();
	}

#else	// WITH_DEV_AUTOMATION_TESTS

	FMovieSceneTrackSegmentBlender* GetTrackSegmentBlender(const UMovieSceneTrack* Track, FMovieSceneTrackSegmentBlenderPtr& OutOwnerPtr)
	{
		if (Track)
		{
			OutOwnerPtr = Track->GetTrackSegmentBlender();
		}
		return OutOwnerPtr.GetPtr();
	}

#endif	// WITH_DEV_AUTOMATION_TESTS

FMovieSceneSegmentIdentifier FMovieSceneEvaluationTrackSegments::Add(FMovieSceneSegment&& Segment)
{
	int32 InsertIndex = 0;
	if (Segment.Range.GetLowerBound().IsClosed())
	{
		// Determine where to insert the segment
		auto GetLowerBound = [](const FMovieSceneSegment& In) {
			return In.Range.GetLowerBound();
		};

		// Find the index of the first segment with a lower bound > than this lower bound.
		InsertIndex = Algo::UpperBoundBy(SortedSegments, Segment.Range.GetLowerBound(), GetLowerBound, MovieSceneHelpers::SortLowerBounds);
		InsertIndex = FMath::Clamp(InsertIndex, 0, SortedSegments.Num());
	}

	if (!ensureMsgf(   ( !SortedSegments.IsValidIndex(InsertIndex  ) || !SortedSegments[InsertIndex  ].Range.Overlaps(Segment.Range) ) && 
			           ( !SortedSegments.IsValidIndex(InsertIndex+1) || !SortedSegments[InsertIndex+1].Range.Overlaps(Segment.Range) ), TEXT("Attempting to add overlapping segment to segment array. This is invalid.")))
	{
		return FMovieSceneSegmentIdentifier();
	}

	// Attempt to combine into the previous or next segment
	if (SortedSegments.IsValidIndex(InsertIndex-1) && SortedSegments[InsertIndex-1].CombineWith(Segment))
	{
		return FMovieSceneSegmentIdentifier(SegmentIdentifierToIndex.Add(InsertIndex-1));
	}
	if (SortedSegments.IsValidIndex(InsertIndex  ) && SortedSegments[InsertIndex  ].CombineWith(Segment))
	{
		return FMovieSceneSegmentIdentifier(SegmentIdentifierToIndex.Add(InsertIndex  ));
	}

	// Fixup any indices that occur after this index
	for (int32& Index : SegmentIdentifierToIndex)
	{
		if (Index >= InsertIndex)
		{
			++Index;
		}
	}

	FMovieSceneSegmentIdentifier ID(SegmentIdentifierToIndex.Num());
	Segment.ID = ID;

	SegmentIdentifierToIndex.Add(InsertIndex);
	SortedSegments.Insert(MoveTemp(Segment), InsertIndex);

	return ID;
}

FMovieSceneEvaluationTrack::FMovieSceneEvaluationTrack()
	: EvaluationPriority(1000)
	, EvaluationMethod(EEvaluationMethod::Static)
	, SourceTrack(nullptr)
	, bEvaluateInPreroll(true)
	, bEvaluateInPostroll(true)
{
}

FMovieSceneEvaluationTrack::FMovieSceneEvaluationTrack(const FGuid& InObjectBindingID)
	: ObjectBindingID(InObjectBindingID)
	, EvaluationPriority(1000)
	, EvaluationMethod(EEvaluationMethod::Static)
	, SourceTrack(nullptr)
	, bEvaluateInPreroll(true)
	, bEvaluateInPostroll(true)
{
}

void FMovieSceneEvaluationTrack::PostSerialize(const FArchive& Ar)
{
	if (Ar.IsLoading())
	{
		// Guard against serialization mismatches where structs have been removed
		TArray<int32, TInlineAllocator<2>> ImplsToRemove;
		for (int32 Index = 0; Index < ChildTemplates.Num(); ++Index)
		{
			FMovieSceneEvalTemplatePtr& Child = ChildTemplates[Index];
			if (!Child.IsValid() || &Child->GetScriptStruct() == FMovieSceneEvalTemplate::StaticStruct())
			{
				ImplsToRemove.Add(Index);
			}
		}

		if (ImplsToRemove.Num())
		{
			for (FMovieSceneSegment& Segment : Segments.GetSorted())
			{
				Segment.Impls.RemoveAll([&](const FSectionEvaluationData& In) { return ImplsToRemove.Contains(In.ImplIndex); });
			}
		}
	}
	SetupOverrides();
}

void FMovieSceneEvaluationTrack::DefineAsSingleTemplate(FMovieSceneEvalTemplatePtr&& InTemplate)
{
	ChildTemplates.Reset(1);
	Segments.Reset(1);

	ChildTemplates.Add(MoveTemp(InTemplate));

	FSectionEvaluationData EvalData(0);
	Segments.Add(FMovieSceneSegment(TRange<float>::All(), TArrayView<FSectionEvaluationData>(&EvalData, 1)));
}

int32 FMovieSceneEvaluationTrack::AddChildTemplate(FMovieSceneEvalTemplatePtr&& InTemplate)
{
	return ChildTemplates.Add(MoveTemp(InTemplate));
}

FMovieSceneSegmentIdentifier FMovieSceneEvaluationTrack::CompileSegment(FMovieSceneEvaluationTreeRangeIterator CurrentNode)
{
	// Define a new segment for the specified iterator's range
	FMovieSceneSegment NewSegment(CurrentNode.Range());

	// Get the segment blender required to blend at the track level for this segment
	FMovieSceneTrackSegmentBlenderPtr TrackSegmentBlenderValue;
	FMovieSceneTrackSegmentBlender* TrackSegmentBlender = GetTrackSegmentBlender(SourceTrack, TrackSegmentBlenderValue);

	// Iterate all the section evaluation data for the current segment range
	TMovieSceneEvaluationTreeDataIterator<FSectionEvaluationData> DataIterator = GetData(CurrentNode.Node());
	if (DataIterator)
	{
		// Track section data
		FSegmentBlendData TrackBlendData;

		FMovieSceneTrackRowSegmentBlenderPtr RowSegmentBlender = SourceTrack ? SourceTrack->GetRowSegmentBlender() : FMovieSceneTrackRowSegmentBlenderPtr();
		// Compile at the row level first
		if (RowSegmentBlender.IsValid())
		{
			TMap<int32, FSegmentBlendData> RowBlendData;

			// Add every FSectionEvaluationData for the current time range to the section data
			for (const FSectionEvaluationData& EvalData : DataIterator)
			{
				const UMovieSceneSection* Section = ChildTemplates[EvalData.ImplIndex]->GetSourceSection();
				if (Section)
				{
					RowBlendData.FindOrAdd(Section->GetRowIndex()).Add(FMovieSceneSectionData(Section, EvalData.ImplIndex, EvalData.Flags));
				}
			}

			for (TPair<int32, FSegmentBlendData>& Pair : RowBlendData)
			{
				RowSegmentBlender->Blend(Pair.Value);
				TrackBlendData.Append(Pair.Value);
			}
		}
		else for (const FSectionEvaluationData& EvalData : DataIterator)
		{
			const UMovieSceneSection* Section = ChildTemplates.IsValidIndex(EvalData.ImplIndex) ? ChildTemplates[EvalData.ImplIndex]->GetSourceSection() : nullptr;
			TrackBlendData.Add(FMovieSceneSectionData(Section, EvalData.ImplIndex, EvalData.Flags));
		}

		// Compile at the track level
		if (TrackSegmentBlender)
		{
			TrackSegmentBlender->Blend(TrackBlendData);
		}

		TrackBlendData.AddToSegment(NewSegment);
	}

	// Insert an empty segment
	if (NewSegment.Impls.Num() == 0)
	{
		if (TrackSegmentBlender && TrackSegmentBlender->CanFillEmptySpace())
		{
			FMovieSceneEvaluationTreeRangeIterator PrevSegmentIt = CurrentNode.Previous();
			FMovieSceneEvaluationTreeRangeIterator NextSegmentIt = CurrentNode.Next();

			ensure(
				( !PrevSegmentIt || !PrevSegmentIt.Range().Overlaps(CurrentNode.Range()) ) &&
				( !NextSegmentIt || !NextSegmentIt.Range().Overlaps(CurrentNode.Range()) )
			);

			FMovieSceneSegmentIdentifier PrevSegmentID = PrevSegmentIt ? GetSegmentFromIterator(PrevSegmentIt) : FMovieSceneSegmentIdentifier();
			FMovieSceneSegmentIdentifier NextSegmentID = NextSegmentIt ? GetSegmentFromIterator(NextSegmentIt) : FMovieSceneSegmentIdentifier();

			FMovieSceneSegment* PrevSegment = PrevSegmentID.IsValid() ? &Segments[PrevSegmentID] : nullptr;
			FMovieSceneSegment* NextSegment = NextSegmentID.IsValid() ? &Segments[NextSegmentID] : nullptr;

			TOptional<FMovieSceneSegment> EmptySegment = TrackSegmentBlender->InsertEmptySpace(NewSegment.Range, PrevSegment, NextSegment);
			if (EmptySegment.IsSet())
			{
				NewSegment.Range = EmptySegment->Range;
				NewSegment.Impls = EmptySegment->Impls;
			}
		}
	}

	NewSegment.bAllowEmpty = TrackSegmentBlender ? TrackSegmentBlender->AllowEmptySegments() : false;
	if (!NewSegment.bAllowEmpty && NewSegment.Impls.Num() == 0)
	{
		return FMovieSceneSegmentIdentifier();
	}

	return Segments.Add(MoveTemp(NewSegment));
}

FMovieSceneSegmentIdentifier FMovieSceneEvaluationTrack::FindFirstSegment(TRange<float> InLocalRange)
{
	// Binary search for an existing compiled segment in the sorted array
	auto GetLowerBound = [](const FMovieSceneSegment& In) {
		return In.Range.GetLowerBound();
	};

	// Find a segment with a lowerbound >= to this iterator range
	TArrayView<const FMovieSceneSegment> SortedSegments = Segments.GetSorted();
	int32 SegmentIndex = Algo::LowerBoundBy(SortedSegments, InLocalRange.GetLowerBound(), GetLowerBound, MovieSceneHelpers::SortLowerBounds);
	if (SortedSegments.IsValidIndex(SegmentIndex) && SortedSegments[SegmentIndex].Range.GetLowerBound() == InLocalRange.GetLowerBound())
	{
		return SortedSegments[SegmentIndex].ID;
	}
	else if (SortedSegments.IsValidIndex(SegmentIndex-1) && SortedSegments[SegmentIndex-1].Range.Overlaps(InLocalRange))
	{
		return SortedSegments[SegmentIndex-1].ID;
	}

	return FMovieSceneSegmentIdentifier();
}

TArray<FMovieSceneSegmentIdentifier> FMovieSceneEvaluationTrack::GetSegmentsInRange(TRange<float> InLocalRange)
{
	TArray<FMovieSceneSegmentIdentifier> SegmentsInRange;

	while (!InLocalRange.IsEmpty())
	{
		FMovieSceneSegmentIdentifier SegmentID = FindFirstSegment(InLocalRange);

		if (!SegmentID.IsValid())
		{
			FMovieSceneEvaluationTreeRangeIterator NodeAtTime = EvaluationTree.Tree.IterateFromLowerBound(InLocalRange.GetLowerBound());
			check(NodeAtTime)
			SegmentID = CompileSegment(NodeAtTime);

			if (!SegmentID.IsValid())
			{
				if (NodeAtTime.Range().GetUpperBound().IsOpen())
				{
					break;
				}

				InLocalRange = TRange<float>(TRangeBound<float>::FlipInclusion(NodeAtTime.Range().GetUpperBound()), InLocalRange.GetUpperBound());
				continue;
			}
		}

		SegmentsInRange.Add(SegmentID);
		if (Segments[SegmentID].Range.GetUpperBound().IsOpen())
		{
			break;
		}

		InLocalRange = TRange<float>(TRangeBound<float>::FlipInclusion(Segments[SegmentID].Range.GetUpperBound()), InLocalRange.GetUpperBound());
	}

	return SegmentsInRange;
}

FMovieSceneSegmentIdentifier FMovieSceneEvaluationTrack::GetSegmentFromTime(float InTime)
{
	FMovieSceneSegmentIdentifier Existing = FindFirstSegment(TRange<float>::Inclusive(InTime, InTime));
	if (Existing.IsValid())
	{
		return Existing;
	}

	FMovieSceneEvaluationTreeRangeIterator NodeAtTime = EvaluationTree.Tree.IterateFromTime(InTime);
	check(NodeAtTime)
	return CompileSegment(NodeAtTime);
}

FMovieSceneSegmentIdentifier FMovieSceneEvaluationTrack::GetSegmentFromIterator(FMovieSceneEvaluationTreeRangeIterator Iterator)
{
	check(Iterator);

	FMovieSceneSegmentIdentifier Existing = FindFirstSegment(Iterator.Range());
	if (Existing.IsValid())
	{
		ensure(Segments[Existing].Range.Contains(Iterator.Range()));
		return Existing;
	}

	return CompileSegment(Iterator);
}

TRange<float> FMovieSceneEvaluationTrack::GetUniqueRangeFromLowerBound(TRangeBound<float> InLowerBound) const
{
	return EvaluationTree.Tree.IterateFromLowerBound(InLowerBound).Range();
}

void FMovieSceneEvaluationTrack::ValidateSegments()
{
	// We don't remove segments as this may break ptrs that have been set up in the evaluation field. Instead we just remove invalid track indices.

	for (FMovieSceneSegment& Segment : Segments.GetSorted())
	{
		for (int32 Index = Segment.Impls.Num() - 1; Index >= 0; --Index)
		{
			int32 TemplateIndex = Segment.Impls[Index].ImplIndex;
			if (!HasChildTemplate(TemplateIndex))
			{
				Segment.Impls.RemoveAt(Index, 1, false);
			}
		}
	}
}

void FMovieSceneEvaluationTrack::SetupOverrides()
{
	for (FMovieSceneEvalTemplatePtr& ChildTemplate : ChildTemplates)
	{
		if (ChildTemplate.IsValid())
		{
			ChildTemplate->SetupOverrides();
		}
	}

	if (TrackTemplate.IsValid())
	{
		TrackTemplate->SetupOverrides();
	}
}

void FMovieSceneEvaluationTrack::Initialize(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	if (!TrackTemplate.IsValid() || !TrackTemplate->HasCustomInitialize())
	{
		DefaultInitialize(SegmentID, Operand, Context, PersistentData, Player);
	}
	else
	{
		TrackTemplate->Initialize(*this, SegmentID, Operand, Context, PersistentData, Player);
	}
}

void FMovieSceneEvaluationTrack::DefaultInitialize(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, FMovieSceneContext Context, FPersistentEvaluationData& PersistentData, IMovieScenePlayer& Player) const
{
	const FMovieSceneSegment& Segment = Segments[SegmentID];
	for (const FSectionEvaluationData& EvalData : Segment.Impls)
	{
		const FMovieSceneEvalTemplate& Template = GetChildTemplate(EvalData.ImplIndex);

		if (Template.RequiresInitialization())
		{
			PersistentData.DeriveSectionKey(EvalData.ImplIndex);

			Context.OverrideTime(EvalData.ForcedTime);
			Context.ApplySectionPrePostRoll(EvalData.IsPreRoll(), EvalData.IsPostRoll());

			Template.Initialize(Operand, Context, PersistentData, Player);
		}
	}
}

void FMovieSceneEvaluationTrack::Evaluate(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	if (!TrackTemplate.IsValid() || !TrackTemplate->HasCustomEvaluate())
	{
		DefaultEvaluate(SegmentID, Operand, Context, PersistentData, ExecutionTokens);
	}
	else
	{
		TrackTemplate->Evaluate(*this, SegmentID, Operand, Context, PersistentData, ExecutionTokens);
	}
}

void FMovieSceneEvaluationTrack::DefaultEvaluate(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	switch (EvaluationMethod)
	{
	case EEvaluationMethod::Static:
		EvaluateStatic(SegmentID, Operand, Context, PersistentData, ExecutionTokens);
		break;
	case EEvaluationMethod::Swept:
		EvaluateSwept(SegmentID, Operand, Context, PersistentData, ExecutionTokens);
		break;
	}
}

void FMovieSceneEvaluationTrack::EvaluateStatic(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, FMovieSceneContext Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	int32 SortedIndex = Segments.GetSortedIndex(SegmentID);
	if (SortedIndex == INDEX_NONE)
	{
		return;
	}

	for (const FSectionEvaluationData& EvalData : GetSegment(SegmentID).Impls)
	{
		const FMovieSceneEvalTemplate& Template = GetChildTemplate(EvalData.ImplIndex);

		Context.OverrideTime(EvalData.ForcedTime);
		Context.ApplySectionPrePostRoll(EvalData.IsPreRoll(), EvalData.IsPostRoll());

		PersistentData.DeriveSectionKey(EvalData.ImplIndex);
		ExecutionTokens.SetCurrentScope(FMovieSceneEvaluationScope(PersistentData.GetSectionKey(), Template.GetCompletionMode()));
		ExecutionTokens.SetContext(Context);

		Template.Evaluate(Operand, Context, PersistentData, ExecutionTokens);
	}
}

namespace
{
	bool IntersectSegmentRanges(const FMovieSceneSegment& Segment, TRange<float> TraversedRange, TMap<int32, TRange<float>>& ImplToAccumulatedRange)
	{
		TRange<float> Intersection = TRange<float>::Intersection(Segment.Range, TraversedRange);
		if (Intersection.IsEmpty())
		{
			return false;
		}

		for (const FSectionEvaluationData& EvalData : Segment.Impls)
		{
			TRange<float>* AccumulatedRange = ImplToAccumulatedRange.Find(EvalData.ImplIndex);
			if (!AccumulatedRange)
			{
				ImplToAccumulatedRange.Add(EvalData.ImplIndex, Intersection);
			}
			else
			{
				*AccumulatedRange = TRange<float>::Hull(*AccumulatedRange, Intersection);
			}
		}

		return true;
	}

	void GatherSweptSegments(TRange<float> TraversedRange, int32 CurrentSegmentIndex, TArrayView<const FMovieSceneSegment> Segments, TMap<int32, TRange<float>>& ImplToAccumulatedRange)
	{
		// Search backwards from the current segment for any segments intersecting the traversed range
		{
			int32 PreviousIndex = CurrentSegmentIndex;
			while(--PreviousIndex >= 0 && IntersectSegmentRanges(Segments[PreviousIndex], TraversedRange, ImplToAccumulatedRange));
		}

		// Obviously the current segment intersects otherwise we wouldn't be in here
		IntersectSegmentRanges(Segments[CurrentSegmentIndex], TraversedRange, ImplToAccumulatedRange);

		// Search forwards from the current segment for any segments intersecting the traversed range
		{
			int32 NextIndex = CurrentSegmentIndex;
			while(++NextIndex < Segments.Num() && IntersectSegmentRanges(Segments[NextIndex], TraversedRange, ImplToAccumulatedRange));
		}
	}
}

void FMovieSceneEvaluationTrack::EvaluateSwept(FMovieSceneSegmentIdentifier SegmentID, const FMovieSceneEvaluationOperand& Operand, const FMovieSceneContext& Context, const FPersistentEvaluationData& PersistentData, FMovieSceneExecutionTokens& ExecutionTokens) const
{
	// Accumulate the relevant ranges that each section intersects with the evaluated range
	TMap<int32, TRange<float>> ImplToAccumulatedRange;

	int32 SortedIndex = Segments.GetSortedIndex(SegmentID);
	if (SortedIndex == INDEX_NONE)
	{
		return;
	}

	GatherSweptSegments(Context.GetRange(), SortedIndex, Segments.GetSorted(), ImplToAccumulatedRange);

	for (auto& Pair : ImplToAccumulatedRange)
	{
		const int32 SectionIndex = Pair.Key;
		TRange<float> EvaluationRange = Pair.Value;
		const FMovieSceneEvalTemplate& Template = GetChildTemplate(SectionIndex);

		PersistentData.DeriveSectionKey(SectionIndex);
		ExecutionTokens.SetCurrentScope(FMovieSceneEvaluationScope(PersistentData.GetSectionKey(), Template.GetCompletionMode()));
		ExecutionTokens.SetContext(Context);
		
		Template.EvaluateSwept(
			Operand,
			Context.Clamp(EvaluationRange),
			PersistentData,
			ExecutionTokens);
	}
}

void FMovieSceneEvaluationTrack::Interrogate(const FMovieSceneContext& Context, FMovieSceneInterrogationData& Container, UObject* BindingOverride)
{
	if (TrackTemplate.IsValid() && TrackTemplate->Interrogate(Context, Container, BindingOverride))
	{
		return;
	}

	FMovieSceneSegmentIdentifier SegmentID = GetSegmentFromTime(Context.GetTime());
	if (!SegmentID.IsValid())
	{
		return;
	}

	if (EvaluationMethod == EEvaluationMethod::Static)
	{
		for (const FSectionEvaluationData& EvalData : GetSegment(SegmentID).Impls)
		{
			GetChildTemplate(EvalData.ImplIndex).Interrogate(Context, Container, BindingOverride);
		}
	}
	else
	{
		// Accumulate the relevant ranges that each section intersects with the evaluated range
		TMap<int32, TRange<float>> ImplToAccumulatedRange;

		GatherSweptSegments(Context.GetRange(), Segments.GetSortedIndex(SegmentID), Segments.GetSorted(), ImplToAccumulatedRange);
		for (auto& Pair : ImplToAccumulatedRange)
		{
			GetChildTemplate(Pair.Key).Interrogate(Context, Pair.Value, Container, BindingOverride);
		}
	}

	// @todo: this should live higher up the callstack when whole template interrogation is supported
	Container.Finalize(Context, BindingOverride);
}