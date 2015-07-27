// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "GroupedKeyArea.h"
#include "MovieSceneSection.h"

FIndexKey::FIndexKey(FString InNodePath, UMovieSceneSection* InSection)
	: NodePath(MoveTemp(InNodePath))
	, Section(InSection)
	, CachedHash(HashCombine(GetTypeHash(NodePath), GetTypeHash(InSection)))
{}

namespace 
{
	/** Structure that defines the index for a particular section */
	struct FIndexEntry
	{
		TMap<FKeyHandle, int32> HandleToGroup;
		TArray<FKeyHandle> GroupHandles;
		TArray<float> RepresentativeTimes;
	};

	/** A persistent index is required to ensure that generated key handles are maintained for the lifetime of specific display nodes */
	TMap<FIndexKey, FIndexEntry> GlobalIndex;
}


FGroupedKeyArea::FGroupedKeyArea(FSequencerDisplayNode& InNode, int32 InSectionIndex)
	: Section(nullptr)
	, IndexKey(FString(), nullptr)
{
	TArray<TSharedRef<FSectionKeyAreaNode>> AllKeyAreaNodes;
	AllKeyAreaNodes.Reserve(36);
	InNode.GetChildKeyAreaNodesRecursively(AllKeyAreaNodes);

	for (const auto& Node : AllKeyAreaNodes)
	{
		TSharedRef<IKeyArea> KeyArea = Node->GetKeyArea(InSectionIndex);

		const int32 KeyAreaIndex = KeyAreas.Num();
		KeyAreas.Add(KeyArea);

		{
			auto* OwningSection = KeyArea->GetOwningSection();
			// Ensure they all belong to the same section
			ensure(!Section || Section == OwningSection);
			Section = OwningSection;
		}

		for (const FKeyHandle& KeyHandle : KeyArea->GetUnsortedKeyHandles())
		{
			Groups.Emplace(KeyArea->GetKeyTime(KeyHandle), KeyAreaIndex, KeyHandle);
		}
	}

	Groups.Sort([](const FKeyGrouping& A, const FKeyGrouping& B){
		return A.RepresentativeTime < B.RepresentativeTime;
	});

	// Remove duplicates
	for (int32 Index = 0; Index < Groups.Num(); ++Index)
	{
		const float CurrentTime = Groups[Index].RepresentativeTime;

		int32 NumToMerge = 0;
		for (int32 DuplIndex = Index + 1; DuplIndex < Groups.Num(); ++DuplIndex)
		{
			if (!FMath::IsNearlyEqual(CurrentTime, Groups[DuplIndex].RepresentativeTime))
			{
				break;
			}
			++NumToMerge;
		}

		if (NumToMerge)
		{
			const int32 Start = Index + 1, End = Start + NumToMerge;
			for (int32 MergeIndex = Start; MergeIndex < End; ++MergeIndex)
			{
				Groups[Index].Keys.Append(MoveTemp(Groups[MergeIndex].Keys));
			}

			Groups.RemoveAt(Start, NumToMerge, false);
		}
	}

	if (Section)
	{
		IndexKey = FIndexKey(InNode.GetPathName(), Section);
		UpdateIndex();
	}
}

void FGroupedKeyArea::UpdateIndex() const
{
	auto& IndexEntry = GlobalIndex.FindOrAdd(IndexKey);

	TArray<FKeyHandle> NewKeyHandles;
	TArray<float> NewRepresentativeTimes;

	IndexEntry.HandleToGroup.Reset();

	for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		float RepresentativeTime = Groups[GroupIndex].RepresentativeTime;

		// Find a key handle we can recycle
		int32 RecycledIndex = IndexEntry.RepresentativeTimes.IndexOfByPredicate([&](float Time){
			// Must be an *exact* match to recycle
			return Time == RepresentativeTime;
		});
		
		if (RecycledIndex != INDEX_NONE)
		{
			NewKeyHandles.Add(IndexEntry.GroupHandles[RecycledIndex]);
			NewRepresentativeTimes.Add(IndexEntry.RepresentativeTimes[RecycledIndex]);

			IndexEntry.GroupHandles.RemoveAt(RecycledIndex, 1, false);
			IndexEntry.RepresentativeTimes.RemoveAt(RecycledIndex, 1, false);
		}
		else
		{
			NewKeyHandles.Add(FKeyHandle());
			NewRepresentativeTimes.Add(RepresentativeTime);
		}

		IndexEntry.HandleToGroup.Add(NewKeyHandles.Last(), GroupIndex);
	}

	IndexEntry.GroupHandles = MoveTemp(NewKeyHandles);
	IndexEntry.RepresentativeTimes = MoveTemp(NewRepresentativeTimes);
}

const FKeyGrouping* FGroupedKeyArea::FindGroup(FKeyHandle InHandle) const
{
	if (auto* IndexEntry = GlobalIndex.Find(IndexKey))
	{
		if (int32* GroupIndex = IndexEntry->HandleToGroup.Find(InHandle))
		{
			return Groups.IsValidIndex(*GroupIndex) ? &Groups[*GroupIndex] : nullptr;
		}
	}
	return nullptr;
}

FKeyGrouping* FGroupedKeyArea::FindGroup(FKeyHandle InHandle)
{
	if (auto* IndexEntry = GlobalIndex.Find(IndexKey))
	{
		if (int32* GroupIndex = IndexEntry->HandleToGroup.Find(InHandle))
		{
			return Groups.IsValidIndex(*GroupIndex) ? &Groups[*GroupIndex] : nullptr;
		}
	}
	return nullptr;
}

FLinearColor FGroupedKeyArea::GetKeyTint(FKeyHandle InHandle) const
{
	// Everything is untinted for now
	return FLinearColor::White;
}

const FSlateBrush* FGroupedKeyArea::GetBrush(FKeyHandle InHandle) const
{
	static const FSlateBrush* KeyBrush = FEditorStyle::GetBrush("Sequencer.Key");
	static const FSlateBrush* PartialKeyBrush = FEditorStyle::GetBrush("Sequencer.PartialKey");

	const auto* Group = FindGroup(InHandle);

	bool bPartialKey = false;

	// Ensure that each key area is represented at least once for it to be considered a 'complete key'
	for (int32 AreaIndex = 0; Group && AreaIndex < KeyAreas.Num(); ++AreaIndex)
	{
		if (!Group->Keys.ContainsByPredicate([&](const FKeyGrouping::FKeyIndex& Key){ return Key.AreaIndex == AreaIndex; }))
		{
			bPartialKey = true;
			break;
		}
	}

	return bPartialKey ? PartialKeyBrush : KeyBrush;
}

TArray<FKeyHandle> FGroupedKeyArea::GetUnsortedKeyHandles() const
{
	TArray<FKeyHandle> Array;
	if (auto* IndexEntry = GlobalIndex.Find(IndexKey))
	{
		IndexEntry->HandleToGroup.GenerateKeyArray(Array);
	}
	return Array;
}

float FGroupedKeyArea::GetKeyTime( FKeyHandle KeyHandle ) const
{
	auto* Group = FindGroup(KeyHandle);
	return Group ? Group->RepresentativeTime : 0.f;
}

FKeyHandle FGroupedKeyArea::MoveKey( FKeyHandle KeyHandle, float DeltaPosition )
{
	int32* GroupIndex = nullptr;

	auto* IndexEntry = GlobalIndex.Find(IndexKey);
	if (IndexEntry)
	{
		GroupIndex = IndexEntry->HandleToGroup.Find(KeyHandle);
	}

	if (!GroupIndex || !Groups.IsValidIndex(*GroupIndex))
	{
		return KeyHandle;
	}

	bool bIsTimeUpdated = false;

	FKeyGrouping& Group = Groups[*GroupIndex];

	// Move all the keys
	for (auto& Key : Group.Keys)
	{
		Key.KeyHandle = KeyAreas[Key.AreaIndex]->MoveKey(Key.KeyHandle, DeltaPosition);

		const float KeyTime = KeyAreas[Key.AreaIndex]->GetKeyTime(Key.KeyHandle);
		Group.RepresentativeTime = bIsTimeUpdated ? FMath::Min(Group.RepresentativeTime, KeyTime) : KeyTime;
		bIsTimeUpdated = true;
	}

	// Update the representative time to the smallest of all the keys (so it will deterministically get the same key handle on regeneration)
	IndexEntry->RepresentativeTimes[*GroupIndex] = Group.RepresentativeTime;

	return KeyHandle;
}

void FGroupedKeyArea::DeleteKey(FKeyHandle KeyHandle)
{
	if (auto* Group = FindGroup(KeyHandle))
	{
		for (auto& Key : Group->Keys)
		{
			KeyAreas[Key.AreaIndex]->DeleteKey(Key.KeyHandle);
		}
	}
}

void FGroupedKeyArea::AddKeyUnique(float Time)
{
	for (auto& Area : KeyAreas)
	{
		Area->AddKeyUnique(Time);
	}
}

FRichCurve* FGroupedKeyArea::GetRichCurve()
{
	return nullptr;
}

UMovieSceneSection* FGroupedKeyArea::GetOwningSection()
{
	return Section;
}

bool FGroupedKeyArea::CanCreateKeyEditor()
{
	return false;
}

TSharedRef<SWidget> FGroupedKeyArea::CreateKeyEditor(ISequencer* Sequencer)
{
	return SNullWidget::NullWidget;
}