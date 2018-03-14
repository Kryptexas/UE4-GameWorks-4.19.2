// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MovieSceneSequenceHierarchy.h"
#include "Sections/MovieSceneSubSection.h"
#include "MovieSceneSequence.h"
#include "MovieScene.h"

FMovieSceneSubSequenceData::FMovieSceneSubSequenceData()
	: Sequence(nullptr)
{}

FMovieSceneSubSequenceData::FMovieSceneSubSequenceData(const UMovieSceneSubSection& InSubSection)
	: Sequence(InSubSection.GetSequence())
	, DeterministicSequenceID(InSubSection.GetSequenceID())
	, PreRollRange(TRange<float>::Empty())
	, PostRollRange(TRange<float>::Empty())
	, HierarchicalBias(InSubSection.Parameters.HierarchicalBias)
#if WITH_EDITORONLY_DATA
	, SectionPath(*InSubSection.GetPathNameInMovieScene())
#endif
{
	UMovieSceneSequence* SequencePtr = GetSequence();

	checkf(SequencePtr, TEXT("Attempting to construct sub sequence data with a null sequence."));

	const float StartTime = InSubSection.GetStartTime();
	const float EndTime = InSubSection.GetEndTime();

	RootToSequenceTransform = 
		FMovieSceneSequenceTransform(SequencePtr->GetMovieScene()->GetPlaybackRange().GetLowerBoundValue() + InSubSection.Parameters.StartOffset) *		// Inner play offset
		FMovieSceneSequenceTransform(0.f, InSubSection.Parameters.TimeScale) *		// Inner play rate
		FMovieSceneSequenceTransform(-StartTime);					// Outer section start time

	PlayRange = InSubSection.GetRange() * RootToSequenceTransform;

	// Make sure pre/postroll ranges are in the inner sequence's time space
	if (InSubSection.GetPreRollTime() > 0)
	{
		PreRollRange = TRange<float>(StartTime - InSubSection.GetPreRollTime(), TRangeBound<float>::Exclusive(StartTime)) * RootToSequenceTransform;
	}
	if (InSubSection.GetPostRollTime() > 0)
	{
		PostRollRange = TRange<float>(TRangeBound<float>::Exclusive(EndTime), EndTime + InSubSection.GetPostRollTime()) * RootToSequenceTransform;
	}
}

UMovieSceneSequence* FMovieSceneSubSequenceData::GetSequence() const
{
	UMovieSceneSequence* ResolvedSequence = GetLoadedSequence();
	if (!ResolvedSequence)
	{
		ResolvedSequence = Cast<UMovieSceneSequence>(Sequence.ResolveObject());
		CachedSequence = ResolvedSequence;
	}
	return ResolvedSequence;
}


UMovieSceneSequence* FMovieSceneSubSequenceData::GetLoadedSequence() const
{
	return CachedSequence.Get();
}

void FMovieSceneSequenceHierarchy::Add(const FMovieSceneSubSequenceData& Data, FMovieSceneSequenceIDRef ThisSequenceID, FMovieSceneSequenceIDRef ParentID)
{
	check(ParentID != MovieSceneSequenceID::Invalid);

	// Add (or update) the sub sequence data
	SubSequences.Add(ThisSequenceID, Data);

	// Set up the hierarchical information if we don't have any, or its wrong
	FMovieSceneSequenceHierarchyNode* ExistingHierarchyNode = Hierarchy.Find(ThisSequenceID);
	if (!ExistingHierarchyNode || ExistingHierarchyNode->ParentID != ParentID)
	{
		if (!ExistingHierarchyNode)
		{
			// The node doesn't yet exist - create it
			FMovieSceneSequenceHierarchyNode Node(ParentID);
			Hierarchy.Add(ThisSequenceID, Node);
		}
		else
		{
			// The node exists already but under the wrong parent - we need to move it
			FMovieSceneSequenceHierarchyNode* Parent = Hierarchy.Find(ExistingHierarchyNode->ParentID);
			check(Parent);
			// Remove it from its parent's children
			Parent->Children.Remove(ThisSequenceID);

			// Set the parent ID
			ExistingHierarchyNode->ParentID = ParentID;
		}

		// Add the node to its parent's children array
		FMovieSceneSequenceHierarchyNode* Parent = Hierarchy.Find(ParentID);
		check(Parent);
		ensure(!Parent->Children.Contains(ThisSequenceID));
		Parent->Children.Add(ThisSequenceID);
	}
}

void FMovieSceneSequenceHierarchy::Remove(TArrayView<const FMovieSceneSequenceID> SequenceIDs)
{
	TArray<FMovieSceneSequenceID, TInlineAllocator<16>> IDsToRemove;
	IDsToRemove.Append(SequenceIDs.GetData(), SequenceIDs.Num());

	while (IDsToRemove.Num())
	{
		int32 NumRemaining = IDsToRemove.Num();
		for (int32 Index = 0; Index < NumRemaining; ++Index)
		{
			FMovieSceneSequenceID ID = IDsToRemove[Index];

			SubSequences.Remove(ID);

			// Remove all children too
			if (const FMovieSceneSequenceHierarchyNode* Node = FindNode(ID))
			{
				FMovieSceneSequenceHierarchyNode* Parent = Hierarchy.Find(Node->ParentID);
				if (Parent)
				{
					Parent->Children.Remove(ID);
				}

				IDsToRemove.Append(Node->Children);
				Hierarchy.Remove(ID);
			}
		}

		IDsToRemove.RemoveAt(0, NumRemaining);
	}
}