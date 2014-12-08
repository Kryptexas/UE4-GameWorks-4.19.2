// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "SMergeTreeView.h"

void SMergeTreeView::Construct(const FArguments InArgs, const FBlueprintMergeData& InData)
{
	Data = InData;
	CurrentDifference = -1;
	CurrentMergeConflict = -1;

	// generate controls:
	// EMergeParticipant::Remote
	{
		SCSViews.Push(
			FSCSDiff(InData.BlueprintRemote)
			);
	}
	// EMergeParticipant::Base
	{
		SCSViews.Push(
			FSCSDiff(InData.BlueprintBase)
			);
	}
	// EMergeParticipant::Local
	{
		SCSViews.Push(
			FSCSDiff(InData.BlueprintLocal)
			);
	}

	TArray< FSCSResolvedIdentifier > RemoteHierarchy = GetRemoteView().GetDisplayedHierarchy();
	TArray< FSCSResolvedIdentifier > BaseHierarchy = GetBaseView().GetDisplayedHierarchy();
	TArray< FSCSResolvedIdentifier > LocalHierarchy = GetLocalView().GetDisplayedHierarchy();

	FSCSDiffRoot RemoteDifferingProperties;
	DiffUtils::CompareUnrelatedSCS(InData.BlueprintBase, BaseHierarchy, InData.BlueprintRemote, RemoteHierarchy, RemoteDifferingProperties );
	FSCSDiffRoot LocalDifferingProperties;
	DiffUtils::CompareUnrelatedSCS(InData.BlueprintBase, BaseHierarchy, InData.BlueprintLocal, LocalHierarchy, LocalDifferingProperties);

	DifferingProperties = RemoteDifferingProperties;
	DifferingProperties.Entries.Append( LocalDifferingProperties.Entries );

	// Remove duplicates and detect conflicts, there is opportunity for performance improvement here, but I doubt this represents a 
	// significant cost:
	for( int32 Iter = 0; Iter != DifferingProperties.Entries.Num(); ++Iter )
	{
		const FSCSDiffEntry& IterEntry = DifferingProperties.Entries[Iter];
		for( int32 Jter = Iter + 1; Jter < DifferingProperties.Entries.Num(); ++Jter )
		{
			const FSCSDiffEntry& JterEntry = DifferingProperties.Entries[Jter];
			// If the change is to a specific property, then it's only a conflict if the other change
			// is also to that same specific property. All 'whole object' (non property specific changes)
			// are conflicting:
			if( IterEntry.TreeIdentifier == JterEntry.TreeIdentifier &&
				( ( IterEntry.PropertyIdentifier == JterEntry.PropertyIdentifier &&
				  IterEntry.PropertyIdentifier != FPropertySoftPath() ) 
				||
				( ( IterEntry.PropertyIdentifier == FPropertySoftPath() ||
					JterEntry.PropertyIdentifier == FPropertySoftPath() ) ) ))
			{
				// for now just flag it as a conflict:
				MergeConflicts.Entries.Push( IterEntry );

				// and remove the redundant difference:
				DifferingProperties.Entries.RemoveAtSwap(Jter--);
			}
		}
	}

	/* This predicate sorts the list of differing properties so that those that are 'earlier' in the tree appear first.
		For example, if we get the following two trees back:
			
			B added at position (3, 2, 1)
			C removed at position (1, 2)

			and 

			D added at position (4, 2, 1)

			the resulting list will be
			C, B, aand D last: */
	const auto SortTreePredicate = []( const FSCSDiffEntry& A, const FSCSDiffEntry& B )
	{
		int32 Idx = 0;
		const TArray<int32>& ATreeAddress = A.TreeIdentifier.TreeLocation;
		const TArray<int32>& BTreeAddress = B.TreeIdentifier.TreeLocation;
		while(true)
		{
			if( !ATreeAddress.IsValidIndex(Idx) )
			{
				// A has a shorter address, show it first:
				return true;
			}
			else if( !BTreeAddress.IsValidIndex(Idx) )
			{
				// B has a shorter address, show it first:
				return false;
			}
			else if( ATreeAddress[Idx] < BTreeAddress[Idx] )
			{
				// A has a lower index, show it first:
				return true;
			}
			else if( ATreeAddress[Idx] > BTreeAddress[Idx] )
			{
				// B has a lower index, show it first:
				return false;
			}
			else
			{
				// tie, go to the next level of the tree:
				++Idx;
			}
		}

		// fall back, just let diff type win:
		return A.DiffType < B.DiffType;
	};


	DifferingProperties.Entries.Sort(SortTreePredicate);

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
	return DifferingProperties.Entries.IsValidIndex(CurrentDifference + 1);
}

bool SMergeTreeView::HasPrevDifference() const
{
	return DifferingProperties.Entries.IsValidIndex(CurrentDifference - 1);
}

void SMergeTreeView::HighlightNextConflict()
{
	if (CurrentMergeConflict + 1 < MergeConflicts.Entries.Num())
	{
		++CurrentMergeConflict;
	}
	HighlightCurrentConflict();
}

void SMergeTreeView::HighlightPrevConflict()
{
	if (CurrentMergeConflict - 1 >= 0)
	{
		--CurrentMergeConflict;
	}
	HighlightCurrentConflict();
}

bool SMergeTreeView::HasNextConflict() const
{
	// return true if we have one conflict so that users can reselect the conflict if they desire. If we 
	// return false when we already have selected this one and only conflict then there will be no way
	// to reselect it if the user wants to.
	return MergeConflicts.Entries.Num() != 0 && (MergeConflicts.Entries.Num() == 1 || CurrentMergeConflict + 1 < MergeConflicts.Entries.Num());
}

bool SMergeTreeView::HasPrevConflict() const
{
	// note in HasNextConflict applies here as well.
	return MergeConflicts.Entries.Num() == 1 || CurrentMergeConflict > 0;
}

void SMergeTreeView::HighlightCurrentConflict()
{
	const FSCSDiffEntry& Conflict = MergeConflicts.Entries[CurrentMergeConflict];
	for (auto& View : SCSViews)
	{
		View.HighlightProperty(Conflict.TreeIdentifier.Name, Conflict.PropertyIdentifier);
	}
}

void SMergeTreeView::HighlightCurrentDifference()
{
	const FSCSDiffEntry& Difference = DifferingProperties.Entries[CurrentDifference];
	for (auto& View : SCSViews)
	{
		View.HighlightProperty(Difference.TreeIdentifier.Name, Difference.PropertyIdentifier);
	}
}

FSCSDiff& SMergeTreeView::GetRemoteView()
{
	return SCSViews[EMergeParticipant::Remote];
}

FSCSDiff& SMergeTreeView::GetBaseView()
{
	return SCSViews[EMergeParticipant::Base];
}

FSCSDiff& SMergeTreeView::GetLocalView()
{
	return SCSViews[EMergeParticipant::Local];
}

