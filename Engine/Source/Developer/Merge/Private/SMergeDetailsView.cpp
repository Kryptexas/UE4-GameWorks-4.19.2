// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "DetailsDiff.h"
#include "ObjectEditorUtils.h"
#include "SBlueprintDiff.h"
#include "SMergeDetailsView.h"

void SMergeDetailsView::Construct(const FArguments InArgs, const FBlueprintMergeData& InData )
{
	Data = InData;

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

	// EMergeParticipant::MERGE_PARTICIPANT_REMOTE
	{
		DetailsViews.Add(
			FDetailsDiff(RemoteCDO, DiffUtils::ResolveAll( RemoteCDO, RemoteDifferingProperties), FDetailsDiff::FOnDisplayedPropertiesChanged() )
		);
	}
	// EMergeParticipant::MERGE_PARTICIPANT_BASE
	{
		DetailsViews.Add(
			FDetailsDiff(BaseCDO, DiffUtils::ResolveAll( BaseCDO, BaseDifferingPropertiesSet.Array()), FDetailsDiff::FOnDisplayedPropertiesChanged())
		);
	}
	// EMergeParticipant::MERGE_PARTICIPANT_LOCAL
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

FDetailsDiff& SMergeDetailsView::GetRemoteDetails()
{
	return DetailsViews[EMergeParticipant::MERGE_PARTICIPANT_REMOTE];
}

FDetailsDiff& SMergeDetailsView::GetBaseDetails()
{
	return DetailsViews[EMergeParticipant::MERGE_PARTICIPANT_BASE];
}

FDetailsDiff& SMergeDetailsView::GetLocalDetails()
{
	return DetailsViews[EMergeParticipant::MERGE_PARTICIPANT_LOCAL];
}

