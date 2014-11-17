// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "SMergeTreeView.h"

void SMergeTreeView::Construct(const FArguments InArgs, const FBlueprintMergeData& InData)
{
	Data = InData;
	CurrentDifference = -1;

	TArray< FSCSDiffEntry > RemoteDifferingProperties;
	TArray< int > RemoteDiffsSortKeys;
	TArray< FSCSDiffEntry > LocalDifferingProperties;
	TArray< int > LocalDiffsSortKeys;

	DiffUtils::CompareUnrelatedSCS(InData.BlueprintBase, InData.BlueprintRemote, RemoteDifferingProperties, &RemoteDiffsSortKeys);
	DiffUtils::CompareUnrelatedSCS(InData.BlueprintBase, InData.BlueprintLocal, LocalDifferingProperties, &LocalDiffsSortKeys);

	check(RemoteDiffsSortKeys.Num() == RemoteDifferingProperties.Num() && LocalDifferingProperties.Num() == LocalDiffsSortKeys.Num());

	// Order the differing properties, and eliminate dupes. Eventually dupes should be treated as collisions when
	// attempting to complete a merge automatically:
	int RemoteIter = 0;
	int LocalIter = 0;
	while (RemoteDifferingProperties.IsValidIndex(RemoteIter) || LocalDifferingProperties.IsValidIndex(LocalIter))
	{
		const bool bStillHaveLocalDiffs = LocalDifferingProperties.IsValidIndex(LocalIter);
		const bool bStillHaveRemoteDiffs = RemoteDifferingProperties.IsValidIndex(RemoteIter);
		FSCSDiffEntry Entry;

		if (bStillHaveLocalDiffs && bStillHaveRemoteDiffs)
		{
			if (LocalDiffsSortKeys[LocalIter] <= RemoteDiffsSortKeys[RemoteIter])
			{
				Entry = LocalDifferingProperties[LocalIter++];
			}
			else
			{
				Entry = RemoteDifferingProperties[RemoteIter++];
			}
		}
		else if (bStillHaveLocalDiffs)
		{
			Entry = LocalDifferingProperties[LocalIter++];
		}
		else
		{
			check(bStillHaveRemoteDiffs);
			Entry = RemoteDifferingProperties[RemoteIter++];
		}

		if (!DifferingProperties.Contains(Entry))
		{
			DifferingProperties.Push(Entry);
		}
	}

	// generate controls:
	// EMergeParticipant::MERGE_PARTICIPANT_REMOTE
	{
		SCSViews.Push(
			FSCSDiff(InData.BlueprintRemote)
			);
	}
	// EMergeParticipant::MERGE_PARTICIPANT_BASE
	{
		SCSViews.Push(
			FSCSDiff(InData.BlueprintBase)
			);
	}
	// EMergeParticipant::MERGE_PARTICIPANT_LOCAL
	{
		SCSViews.Push(
			FSCSDiff(InData.BlueprintLocal)
			);
	}

	ChildSlot[
		SNew(SSplitter)
			+ SSplitter::Slot()
			[
				GetRemoteView().TreeWidget()
			]
		+ SSplitter::Slot()
			[
				GetBaseView().TreeWidget()
			]
		+ SSplitter::Slot()
			[
				GetLocalView().TreeWidget()
			]
	];
}

void SMergeTreeView::NextDiff()
{
	++CurrentDifference;
	HighlightCurrentDifference();
}

void SMergeTreeView::PrevDiff()
{
	--CurrentDifference;
	HighlightCurrentDifference();
}

bool SMergeTreeView::HasNextDifference() const
{
	return DifferingProperties.IsValidIndex(CurrentDifference + 1);
}

bool SMergeTreeView::HasPrevDifference() const
{
	return DifferingProperties.IsValidIndex(CurrentDifference - 1);
}

void SMergeTreeView::HighlightCurrentDifference()
{
	for (auto& View : SCSViews)
	{
		View.HighlightProperty(DifferingProperties[CurrentDifference]);
	}
}

FSCSDiff& SMergeTreeView::GetRemoteView()
{
	return SCSViews[EMergeParticipant::MERGE_PARTICIPANT_REMOTE];
}

FSCSDiff& SMergeTreeView::GetBaseView()
{
	return SCSViews[EMergeParticipant::MERGE_PARTICIPANT_BASE];
}

FSCSDiff& SMergeTreeView::GetLocalView()
{
	return SCSViews[EMergeParticipant::MERGE_PARTICIPANT_LOCAL];
}

