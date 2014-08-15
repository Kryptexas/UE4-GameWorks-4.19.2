// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MergePrivatePCH.h"

#include "DetailsDiff.h"
#include "SBlueprintDiff.h"
#include "SMergeDetailsView.h"

void SMergeDetailsView::Construct(const FArguments InArgs, const FBlueprintMergeData& InData)
{
	Data = InData;

	const UObject* RemoteCDO = DiffUtils::GetCDO(InData.BlueprintRemote);
	TMap< FName, const UProperty* > RemotePropertyMap = DiffUtils::GetProperties(RemoteCDO);
	const UObject* BaseCDO = DiffUtils::GetCDO(InData.BlueprintBase);
	TMap< FName, const UProperty* > BasePropertyMap = DiffUtils::GetProperties(BaseCDO);
	const UObject* LocalCDO = DiffUtils::GetCDO(InData.BlueprintLocal);
	TMap< FName, const UProperty* > LocalPropertyMap = DiffUtils::GetProperties(LocalCDO);

	TArray<FName> RemoteIdenticalProperties, RemoteDifferingProperties;
	DiffUtils::CompareUnrelatedObjects(BaseCDO, BasePropertyMap, RemoteCDO, RemotePropertyMap, RemoteIdenticalProperties, RemoteDifferingProperties);
	TArray<FName> LocalIdenticalProperties, LocalDifferingProperties;
	DiffUtils::CompareUnrelatedObjects(BaseCDO, BasePropertyMap, LocalCDO, LocalPropertyMap, LocalIdenticalProperties, LocalDifferingProperties);

	TSet<FName> RemoteIdenticalPropertiesSet(RemoteIdenticalProperties);
	TSet<FName> LocalIdenticalPropertiesSet(LocalIdenticalProperties);
	TSet<FName> BaseIdenticalPropertiesSet(RemoteIdenticalPropertiesSet.Intersect(LocalIdenticalPropertiesSet));
	
	/*	
		DifferingProperties is an ordered list of all differing properties (properties added, removed or changed) in remote or local.
		Strictly speaking it's impossible to guarantee that we'll traverse remote and local differences in the same order (for instance
		because property layout could somehow change between revisions) but in practice the following works:

			1. Iterate properties in base, add any properties that differ in remote or local to DifferingProperties.
			2. Iterate properties in remote, add any new properties to DifferingProperties
			3. Iterate properties in local, add any new properties to DifferingProperties, if they have not already been added
	*/
	const auto AddPropertiesOrdered = []( UProperty const* Property, const TSet<FName>& IdenticalProperties, TArray<FName>& ResultingProperties )
	{
		if ((!Property->HasAnyPropertyFlags(CPF_Parm) && Property->HasAnyPropertyFlags(CPF_Edit)))
		{
			// contains check here is O(n), so we're needlessly n^2:
			if (!ResultingProperties.Contains(Property->GetFName()))
			{
				if (!IdenticalProperties.Contains(Property->GetFName()))
				{
					ResultingProperties.Add(Property->GetFName());
				}
			}
		}
	};

	if( BaseCDO )
	{
		const UClass* Class = BaseCDO->GetClass();
		for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			AddPropertiesOrdered( *PropertyIt, BaseIdenticalPropertiesSet, DifferingProperties );
		}
	}
	if( RemoteCDO )
	{
		const UClass* Class = RemoteCDO->GetClass();
		for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			AddPropertiesOrdered(*PropertyIt, RemoteIdenticalPropertiesSet, DifferingProperties);
		}
	}
	if (LocalCDO)
	{
		const UClass* Class = LocalCDO->GetClass();
		for (TFieldIterator<UProperty> PropertyIt(Class, EFieldIteratorFlags::IncludeSuper); PropertyIt; ++PropertyIt)
		{
			AddPropertiesOrdered(*PropertyIt, LocalIdenticalPropertiesSet, DifferingProperties);
		}
	}
	CurrentDifference = -1;

	// EMergeParticipant::MERGE_PARTICIPANT_REMOTE
	{
		DetailsViews.Add(
			FDetailsDiff( RemoteCDO, RemotePropertyMap, RemoteIdenticalPropertiesSet)
		);
	}
	// EMergeParticipant::MERGE_PARTICIPANT_BASE
	{
		DetailsViews.Add(
			FDetailsDiff(BaseCDO, BasePropertyMap, BaseIdenticalPropertiesSet)
		);
	}
	// EMergeParticipant::MERGE_PARTICIPANT_LOCAL
	{
		DetailsViews.Add(
			FDetailsDiff(LocalCDO, LocalPropertyMap, LocalIdenticalPropertiesSet)
		);
	}

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
	const FName PropertyName = DifferingProperties[CurrentDifference];

	for( auto& DetailDiff : DetailsViews )
	{
		DetailDiff.HighlightProperty(PropertyName);
	}
}

bool SMergeDetailsView::HasDifferences() const
{
	return DifferingProperties.Num() != 0;
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

