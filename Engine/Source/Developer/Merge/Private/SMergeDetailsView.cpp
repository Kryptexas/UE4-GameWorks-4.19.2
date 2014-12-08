// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "DetailsDiff.h"
#include "ObjectEditorUtils.h"
#include "SBlueprintDiff.h"
#include "SMergeDetailsView.h"

void SMergeDetailsView::Construct(const FArguments InArgs, const FBlueprintMergeData& InData )
{
	Data = InData;
	CurrentMergeConflict = INDEX_NONE;

	const UObject* RemoteCDO = DiffUtils::GetCDO(InData.BlueprintRemote);
	const UObject* BaseCDO = DiffUtils::GetCDO(InData.BlueprintBase);
	const UObject* LocalCDO = DiffUtils::GetCDO(InData.BlueprintLocal);

	TArray<FPropertySoftPath> RemoteDifferingProperties;
	DiffUtils::CompareUnrelatedObjects(BaseCDO, RemoteCDO, RemoteDifferingProperties);
	TArray<FPropertySoftPath> LocalDifferingProperties;
	DiffUtils::CompareUnrelatedObjects(BaseCDO, LocalCDO, LocalDifferingProperties);

	FPropertySoftPathSet RemoteDifferingPropertiesSet(RemoteDifferingProperties);
	FPropertySoftPathSet LocalDifferingPropertiesSet(LocalDifferingProperties);
	FPropertySoftPathSet BaseDifferingPropertiesSet(RemoteDifferingPropertiesSet.Union(LocalDifferingPropertiesSet));
	MergeConflicts = RemoteDifferingPropertiesSet.Intersect(LocalDifferingPropertiesSet).Array();

	/*	
		DifferingProperties is an ordered list of all differing properties (properties added, removed or changed) in remote or local.
		Strictly speaking it's impossible to guarantee that we'll traverse remote and local differences in the same order (for instance
		because property layout could somehow change between revisions) but in practice the following works:

			1. Iterate properties in base, add any properties that differ in remote or local to DifferingProperties.
			2. Iterate properties in remote, add any new properties to DifferingProperties
			3. Iterate properties in local, add any new properties to DifferingProperties, if they have not already been added
	*/
	const auto AddPropertiesOrdered = [](FPropertySoftPath const& Property, const FPropertySoftPathSet& DifferingProperties, TArray<FPropertySoftPath>& ResultingProperties)
	{
		// contains check here is O(n), so we're needlessly n^2:
		if (!ResultingProperties.Contains(Property))
		{
			if (DifferingProperties.Contains(Property))
			{
				ResultingProperties.Add(Property);
			}
		}
	};

	// EMergeParticipant::Remote
	{
		DetailsViews.Add(
			FDetailsDiff(RemoteCDO, DiffUtils::ResolveAll( RemoteCDO, RemoteDifferingProperties), FDetailsDiff::FOnDisplayedPropertiesChanged() )
		);
	}
	// EMergeParticipant::Base
	{
		DetailsViews.Add(
			FDetailsDiff(BaseCDO, DiffUtils::ResolveAll( BaseCDO, BaseDifferingPropertiesSet.Array()), FDetailsDiff::FOnDisplayedPropertiesChanged())
		);
	}
	// EMergeParticipant::Local
	{
		DetailsViews.Add(
			FDetailsDiff(LocalCDO, DiffUtils::ResolveAll( LocalCDO, LocalDifferingProperties), FDetailsDiff::FOnDisplayedPropertiesChanged())
		);
	}

	TArray<FPropertySoftPath> RemoteVisibleProperties = GetRemoteDetails().GetDisplayedProperties();
	TArray<FPropertySoftPath> BaseVisibleProperties = GetBaseDetails().GetDisplayedProperties();
	TArray<FPropertySoftPath> LocalVisibleProperties = GetLocalDetails().GetDisplayedProperties();
	if (BaseCDO)
	{
		for (const auto& Property : BaseVisibleProperties)
		{
			AddPropertiesOrdered(Property, BaseDifferingPropertiesSet, DifferingProperties);
		}
	}
	if (RemoteCDO)
	{
		for (const auto& Property : RemoteVisibleProperties)
		{
			AddPropertiesOrdered(Property, RemoteDifferingPropertiesSet, DifferingProperties);
		}
	}
	if (LocalCDO)
	{
		for (const auto& Property : LocalVisibleProperties)
		{
			AddPropertiesOrdered(Property, LocalDifferingPropertiesSet, DifferingProperties);
		}
	}
	CurrentDifference = -1;

	ChildSlot[
		SNew(SSplitter)
		+ SSplitter::Slot()
		[
			GetRemoteDetails().DetailsWidget()
		] 
		+ SSplitter::Slot()
		[
			GetBaseDetails().DetailsWidget()
		]
		+ SSplitter::Slot()
		[
			GetLocalDetails().DetailsWidget()
		]
	];
}

void SMergeDetailsView::NextDiff()
{
	if (DifferingProperties.Num() == 0)
	{
		return;
	}

	CurrentDifference = (CurrentDifference + 1) % DifferingProperties.Num();

	HighlightCurrentDifference();
}

void SMergeDetailsView::PrevDiff()
{
	if (DifferingProperties.Num() == 0)
	{
		return;
	}

	--CurrentDifference;
	if (CurrentDifference < 0)
	{
		CurrentDifference = DifferingProperties.Num() - 1;
	}

	HighlightCurrentDifference();
}

void SMergeDetailsView::HighlightCurrentDifference()
{
	for( auto& DetailDiff : DetailsViews )
	{
		DetailDiff.HighlightProperty(DifferingProperties[CurrentDifference]);
	}
}

bool SMergeDetailsView::HasNextDifference() const
{
	return DifferingProperties.IsValidIndex( CurrentDifference + 1 );
}

bool SMergeDetailsView::HasPrevDifference() const
{
	return DifferingProperties.IsValidIndex(CurrentDifference - 1);
}

void SMergeDetailsView::HighlightNextConflict()
{
	if (CurrentMergeConflict + 1 < MergeConflicts.Num())
	{
		++CurrentMergeConflict;
	}
	for (auto& DetailDiff : DetailsViews)
	{
		DetailDiff.HighlightProperty(MergeConflicts[CurrentMergeConflict]);
	}
}

void SMergeDetailsView::HighlightPrevConflict()
{
	if (CurrentMergeConflict - 1 >= 0)
	{
		--CurrentMergeConflict;
	}
	for (auto& DetailDiff : DetailsViews)
	{
		DetailDiff.HighlightProperty(MergeConflicts[CurrentMergeConflict]);
	}
}

bool SMergeDetailsView::HasNextConflict() const
{
	// return true if we have one conflict so that users can reselect the conflict if they desire. If we 
	// return false when we already have selected this one and only conflict then there will be no way
	// to reselect it if the user wants to.
	return MergeConflicts.Num() != 0 && (MergeConflicts.Num() == 1 || CurrentMergeConflict + 1 < MergeConflicts.Num());
}

bool SMergeDetailsView::HasPrevConflict() const
{
	// note in HasNextConflict applies here as well.
	return MergeConflicts.Num() == 1 || CurrentMergeConflict > 0;
}

FDetailsDiff& SMergeDetailsView::GetRemoteDetails()
{
	return DetailsViews[EMergeParticipant::Remote];
}

FDetailsDiff& SMergeDetailsView::GetBaseDetails()
{
	return DetailsViews[EMergeParticipant::Base];
}

FDetailsDiff& SMergeDetailsView::GetLocalDetails()
{
	return DetailsViews[EMergeParticipant::Local];
}

