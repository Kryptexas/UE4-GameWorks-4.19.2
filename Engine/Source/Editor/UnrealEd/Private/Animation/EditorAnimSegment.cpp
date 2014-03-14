// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimMontage.cpp: Montage classes that contains slots
=============================================================================*/ 

#include "UnrealEd.h"



UEditorAnimSegment::UEditorAnimSegment(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	AnimSlotIndex = 0;
	AnimSegmentIndex = 0;
}

void UEditorAnimSegment::InitAnimSegment(int AnimSlotIndexIn,int AnimSegmentIndexIn)
{
	AnimSlotIndex = AnimSlotIndexIn;
	AnimSegmentIndex = AnimSegmentIndexIn;
	if(UAnimMontage* Montage = Cast<UAnimMontage>(AnimObject))
	{
		if(Montage->SlotAnimTracks.IsValidIndex(AnimSlotIndex) && Montage->SlotAnimTracks[AnimSlotIndex].AnimTrack.AnimSegments.IsValidIndex(AnimSegmentIndex))
		{
			AnimSegment = Montage->SlotAnimTracks[AnimSlotIndex].AnimTrack.AnimSegments[AnimSegmentIndex];
		}
	}
}

bool UEditorAnimSegment::ApplyChangesToMontage()
{
	if(UAnimMontage* Montage = Cast<UAnimMontage>(AnimObject))
	{
		if(Montage->SlotAnimTracks.IsValidIndex(AnimSlotIndex) && Montage->SlotAnimTracks[AnimSlotIndex].AnimTrack.AnimSegments.IsValidIndex(AnimSegmentIndex) )
		{
			if (AnimSegment.AnimReference && AnimSegment.AnimReference->GetSkeleton() == Montage->GetSkeleton())
			{
				Montage->SlotAnimTracks[AnimSlotIndex].AnimTrack.AnimSegments[AnimSegmentIndex] = AnimSegment;
				return true;
			}
			else
			{
				AnimSegment.AnimReference = Montage->SlotAnimTracks[AnimSlotIndex].AnimTrack.AnimSegments[AnimSegmentIndex].AnimReference;
				return false;
			}
		}
	}

	return false;
}

bool UEditorAnimSegment::PropertyChangeRequiresRebuild(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if( PropertyName == FName(TEXT("AnimEndTime")) || PropertyName == FName(TEXT("AnimStartTime")) || PropertyName == FName(TEXT("AnimPlayRate")) || PropertyName == FName(TEXT("LoopingCount")))
	{
		// Changing the end or start time of the segment length can't change the order of the montage segments.
		// Return false so that the montage editor does not fully rebuild its UI and we can keep this UEditorAnimSegment object 
		// in the details view. (A better solution would be handling the rebuilding in a way that didn't possibly invalidate the UEditorMontageObj in the details view)
		return false;
	}

	return true;
}
