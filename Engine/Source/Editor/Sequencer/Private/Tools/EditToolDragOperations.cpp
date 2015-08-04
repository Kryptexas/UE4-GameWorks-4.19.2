// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "SequencerPrivatePCH.h"
#include "EditToolDragOperations.h"
#include "Sequencer.h"
#include "MovieSceneSection.h"
#include "ISequencerSection.h"
#include "IKeyArea.h"
#include "CommonMovieSceneTools.h"
#include "MovieScene.h"
#include "MovieSceneTrack.h"
#include "MovieSceneShotTrack.h"
#include "VirtualTrackArea.h"

struct FDefaultKeySnappingCandidates : ISequencerSnapCandidate
{
	FDefaultKeySnappingCandidates(const TSet<FSelectedKey>& InSelectedKeys)
		: SelectedKeys(InSelectedKeys)
	{}

	bool IsKeyApplicable(FKeyHandle KeyHandle, const TSharedPtr<IKeyArea>& KeyArea, UMovieSceneSection* Section)
	{
		return !SelectedKeys.Contains(FSelectedKey(*Section, KeyArea, KeyHandle));
	}

	bool AreSectionBoundsApplicable(UMovieSceneSection* Section) { return true; }
	bool AreGridLinesApplicable() { return true; }

	const TSet<FSelectedKey>& SelectedKeys;
};


TOptional<FSequencerSnapField::FSnapResult> SnapToInterval(const TArray<float>& InTimes, float Threshold, const USequencerSettings& Settings)
{
	TOptional<FSequencerSnapField::FSnapResult> Result;

	float SnapAmount = 0.f;
	for (float Time : InTimes)
	{
		float IntervalSnap = Settings.SnapTimeToInterval(Time);
		float ThisSnapAmount = IntervalSnap - Time;
		if (FMath::Abs(ThisSnapAmount) <= Threshold)
		{
			if (!Result.IsSet() || FMath::Abs(ThisSnapAmount) < SnapAmount)
			{
				Result = FSequencerSnapField::FSnapResult{Time, IntervalSnap};
				SnapAmount = ThisSnapAmount;
			}
		}
	}

	return Result;
}

/** How many pixels near the mouse has to be before snapping occurs */
const float PixelSnapWidth = 10.f;

TRange<float> GetSectionBoundaries(UMovieSceneSection* Section, TSharedPtr<FTrackNode> SequencerNode)
{
	// Find the borders of where you can drag to
	float LowerBound = -FLT_MAX, UpperBound = FLT_MAX;

	// Also get the closest borders on either side
	const TArray< TSharedRef<ISequencerSection> >& Sections = SequencerNode->GetSections();
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
	{
		const UMovieSceneSection* InSection = Sections[SectionIndex]->GetSectionObject();
		if (Section != InSection && Section->GetRowIndex() == InSection->GetRowIndex())
		{
			if (InSection->GetEndTime() <= Section->GetStartTime() && InSection->GetEndTime() > LowerBound)
			{
				LowerBound = InSection->GetEndTime();
			}
			if (InSection->GetStartTime() >= Section->GetEndTime() && InSection->GetStartTime() < UpperBound)
			{
				UpperBound = InSection->GetStartTime();
			}
		}
	}

	return TRange<float>(LowerBound, UpperBound);
}

void GetSectionSnapTimes(TArray<float>& OutSnapTimes, UMovieSceneSection* Section, TSharedPtr<FTrackNode> SequencerNode, bool bIgnoreOurSectionCustomSnaps)
{
	// @todo Sequencer handle dilation snapping better

	// Collect all the potential snap times from other section borders
	const TArray< TSharedRef<ISequencerSection> >& Sections = SequencerNode->GetSections();
	for (int32 SectionIndex = 0; SectionIndex < Sections.Num(); ++SectionIndex)
	{
		const UMovieSceneSection* InSection = Sections[SectionIndex]->GetSectionObject();
		bool bIsThisSection = Section == InSection;
		if (!bIgnoreOurSectionCustomSnaps || !bIsThisSection)
		{
			InSection->GetSnapTimes(OutSnapTimes, Section != InSection);
		}
	}

	// snap to director track if it exists, and we are not the director track
	UMovieSceneTrack* OuterTrack = Cast<UMovieSceneTrack>(Section->GetOuter());
	UMovieScene* MovieScene = Cast<UMovieScene>(OuterTrack->GetOuter());
	UMovieSceneTrack* ShotTrack = MovieScene->FindMasterTrack(UMovieSceneShotTrack::StaticClass());
	if (ShotTrack && OuterTrack != ShotTrack)
	{
		const TArray<UMovieSceneSection*>& ShotSections = ShotTrack->GetAllSections();
		for (int32 SectionIndex = 0; SectionIndex < ShotSections.Num(); ++SectionIndex)
		{
			auto Shot = ShotSections[SectionIndex];
			Shot->GetSnapTimes(OutSnapTimes, true);
		}
	}
}

bool SnapToTimes(TArray<float> InitialTimes, const TArray<float>& SnapTimes, const FTimeToPixel& TimeToPixelConverter, float& OutInitialTime, float& OutSnapTime)
{
	bool bSuccess = false;
	float ClosestTimePixelDistance = PixelSnapWidth;
	
	for (int32 InitialTimeIndex = 0; InitialTimeIndex < InitialTimes.Num(); ++InitialTimeIndex)
	{
		float InitialTime = InitialTimes[InitialTimeIndex];
		float PixelXOfTime = TimeToPixelConverter.TimeToPixel(InitialTime);

		for (int32 SnapTimeIndex = 0; SnapTimeIndex < SnapTimes.Num(); ++SnapTimeIndex)
		{
			float SnapTime = SnapTimes[SnapTimeIndex];
			float PixelXOfSnapTime = TimeToPixelConverter.TimeToPixel(SnapTime);

			float PixelDistance = FMath::Abs(PixelXOfTime - PixelXOfSnapTime);
			if (PixelDistance < ClosestTimePixelDistance)
			{
				ClosestTimePixelDistance = PixelDistance;
				OutInitialTime = InitialTime;
				OutSnapTime = SnapTime;
				bSuccess = true;
			}
		}
	}

	return bSuccess;
}

TOptional<float> SnapToTimes(float InitialTime, const TArray<float>& SnapTimes, const FTimeToPixel& TimeToPixelConverter)
{
	TArray<float> InitialTimes;
	InitialTimes.Add(InitialTime);

	float OutSnapTime = 0.f;
	float OutInitialTime = 0.f;
	bool bSuccess = SnapToTimes(InitialTimes, SnapTimes, TimeToPixelConverter, OutInitialTime, OutSnapTime);
	if (bSuccess)
	{
		return TOptional<float>(OutSnapTime);
	}
	return TOptional<float>();
}


FEditToolDragOperation::FEditToolDragOperation( FSequencer& InSequencer )
	: Sequencer(InSequencer)
{
	Settings = Sequencer.GetSettings();
}

void FEditToolDragOperation::BeginTransaction( const TArray< FSectionHandle >& Sections, const FText& TransactionDesc )
{
	// Begin an editor transaction and mark the section as transactional so it's state will be saved
	Transaction.Reset( new FScopedTransaction(TransactionDesc) );

	for (auto& Handle : Sections)
	{
		UMovieSceneSection* SectionObj = Handle.GetSectionObject();

		SectionObj->SetFlags( RF_Transactional );
		// Save the current state of the section
		SectionObj->Modify();
	}
}

void FEditToolDragOperation::EndTransaction()
{
	Transaction.Reset();
	Sequencer.UpdateRuntimeInstances();
}

FResizeSection::FResizeSection( FSequencer& InSequencer, TArray<FSectionHandle> InSections, TOptional<FSectionHandle> InCardinalSection, bool bInDraggingByEnd )
	: FEditToolDragOperation( InSequencer )
	, Sections( MoveTemp(InSections) )
	, CardinalSection(InCardinalSection)
	, bDraggingByEnd(bInDraggingByEnd)
	, MouseDownTime(ForceInit)
{
}

void FResizeSection::OnBeginDrag(const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea)
{
	BeginTransaction( Sections, NSLOCTEXT("Sequencer", "DragSectionEdgeTransaction", "Resize section") );

	MouseDownTime = VirtualTrackArea.PixelToTime(LocalMousePos.X);

	DraggedKeyHandles.Empty();
	SectionInitTimes.Empty();

	for (auto& Handle : Sections)
	{
		UMovieSceneSection* Section = Handle.GetSectionObject();

		Section->GetKeyHandles(DraggedKeyHandles);
		SectionInitTimes.Add(Section, bDraggingByEnd ? Section->GetEndTime() : Section->GetStartTime());
	}
}

void FResizeSection::OnEndDrag()
{
	EndTransaction();

	DraggedKeyHandles.Empty();
}

void FResizeSection::OnDrag( const FPointerEvent& MouseEvent, const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea )
{
	bool bIsDilating = MouseEvent.IsControlDown();

	for (auto& Handle : Sections)
	{
		UMovieSceneSection* Section = Handle.GetSectionObject();

		// Convert the current mouse position to a time
		float DeltaTime = VirtualTrackArea.PixelToTime(LocalMousePos.X) - MouseDownTime;
		float NewTime = SectionInitTimes[Section] + DeltaTime;

		// Find the borders of where you can drag to
		TRange<float> SectionBoundaries = GetSectionBoundaries(Section, Handle.TrackNode);
		
		// Snapping
		if ( Settings->GetIsSnapEnabled() )
		{
			bool bSnappedToSection = false;
			if ( Settings->GetSnapSectionTimesToSections() )
			{
				TArray<float> TimesToSnapTo;
				GetSectionSnapTimes(TimesToSnapTo, Section, Handle.TrackNode, bIsDilating);

				TOptional<float> NewSnappedTime = SnapToTimes(NewTime, TimesToSnapTo, VirtualTrackArea);

				if (NewSnappedTime.IsSet())
				{
					NewTime = NewSnappedTime.GetValue();
					bSnappedToSection = true;
				}
			}

			if ( bSnappedToSection == false && Settings->GetSnapSectionTimesToInterval() )
			{
				NewTime = Settings->SnapTimeToInterval(NewTime);
			}
		}

		if( bDraggingByEnd )
		{
			// Dragging the end of a section
			// Ensure we aren't shrinking past the start time
			NewTime = FMath::Clamp( NewTime, Section->GetStartTime(), SectionBoundaries.GetUpperBoundValue() );

			if (bIsDilating)
			{
				float NewSize = NewTime - Section->GetStartTime();
				float DilationFactor = NewSize / Section->GetTimeSize();
				Section->DilateSection(DilationFactor, Section->GetStartTime(), DraggedKeyHandles);
			}
			else
			{
				Section->SetEndTime( NewTime );
			}
		}
		else if( !bDraggingByEnd )
		{
			// Dragging the start of a section
			// Ensure we arent expanding past the end time
			NewTime = FMath::Clamp( NewTime, SectionBoundaries.GetLowerBoundValue(), Section->GetEndTime() );
			
			if (bIsDilating)
			{
				float NewSize = Section->GetEndTime() - NewTime;
				float DilationFactor = NewSize / Section->GetTimeSize();
				Section->DilateSection(DilationFactor, Section->GetEndTime(), DraggedKeyHandles);
			}
			else
			{
				Section->SetStartTime( NewTime );
			}
		}
	}
}

FMoveSection::FMoveSection( FSequencer& InSequencer, TArray<FSectionHandle> InSections )
	: FEditToolDragOperation( InSequencer )
	, Sections( MoveTemp(InSections) )
{
}

void FMoveSection::OnBeginDrag(const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea)
{
	BeginTransaction( Sections, NSLOCTEXT("Sequencer", "MoveSectionTransaction", "Move Section") );

	MouseDownTime = VirtualTrackArea.PixelToTime(LocalMousePos.X);

	DraggedKeyHandles.Empty();

	for (auto Section : Sections)
	{
		Section.GetSectionObject()->GetKeyHandles(DraggedKeyHandles);
	}
}

void FMoveSection::OnEndDrag()
{
	DraggedKeyHandles.Empty();

	for (auto& Handle : Sections)
	{
		Handle.TrackNode->FixRowIndices();

		UMovieSceneSection* Section = Handle.GetSectionObject();
		UMovieSceneTrack* OuterTrack = Cast<UMovieSceneTrack>(Section->GetOuter());

		if (OuterTrack)
		{
			OuterTrack->Modify();
			OuterTrack->OnSectionMoved(*Section);
		}
	}

	EndTransaction();
}


void FMoveSection::OnDrag( const FPointerEvent& MouseEvent, const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea )
{
	float NewMouseDownTime = VirtualTrackArea.PixelToTime(LocalMousePos.X);
	float DistanceMoved =  NewMouseDownTime - MouseDownTime;
	float DeltaTime = DistanceMoved;

	for (auto& Handle : Sections)
	{
		UMovieSceneSection* Section = Handle.GetSectionObject();

		auto SequencerNodeSections = Handle.TrackNode->GetSections();
		
		TArray<UMovieSceneSection*> MovieSceneSections;
		for (int32 i = 0; i < SequencerNodeSections.Num(); ++i)
		{
			MovieSceneSections.Add(SequencerNodeSections[i]->GetSectionObject());
		}
		
		if ( Settings->GetIsSnapEnabled() )
		{
			bool bSnappedToSection = false;
			if ( Settings->GetSnapSectionTimesToSections() )
			{
				TArray<float> TimesToSnapTo;
				GetSectionSnapTimes(TimesToSnapTo, Section, Handle.TrackNode, true);

				TArray<float> TimesToSnap;
				TimesToSnap.Add(DistanceMoved + Section->GetStartTime());
				TimesToSnap.Add(DistanceMoved + Section->GetEndTime());

				float OutSnappedTime = 0.f;
				float OutNewTime = 0.f;
				if (SnapToTimes(TimesToSnap, TimesToSnapTo, VirtualTrackArea, OutSnappedTime, OutNewTime))
				{
					DeltaTime = OutNewTime - (OutSnappedTime - DistanceMoved);
					bSnappedToSection = true;
				}
			}

			if ( bSnappedToSection == false && Settings->GetSnapSectionTimesToInterval() )
			{
				float NewStartTime = DistanceMoved + Section->GetStartTime();
				DeltaTime = Settings->SnapTimeToInterval( NewStartTime ) - Section->GetStartTime();
			}
		}

		int32 TargetRowIndex = Section->GetRowIndex();

		// vertical dragging - master tracks only
		if (Handle.TrackNode->GetTrack()->SupportsMultipleRows() && SequencerNodeSections.Num() > 1)
		{
			float TrackHeight = SequencerNodeSections[0]->GetSectionHeight();

			if (LocalMousePos.Y < 0.f || LocalMousePos.Y > TrackHeight)
			{
				int32 MaxRowIndex = 0;
				for (int32 i = 0; i < SequencerNodeSections.Num(); ++i)
				{
					if (SequencerNodeSections[i]->GetSectionObject() != Section)
					{
						MaxRowIndex = FMath::Max(MaxRowIndex, SequencerNodeSections[i]->GetSectionObject()->GetRowIndex());
					}
				}

				TargetRowIndex = FMath::Clamp(Section->GetRowIndex() + FMath::FloorToInt(LocalMousePos.Y / TrackHeight),
					0, MaxRowIndex + 1);
			}
		}
		
		bool bDeltaX = !FMath::IsNearlyZero(DeltaTime);
		bool bDeltaY = TargetRowIndex != Section->GetRowIndex();

		if (bDeltaX && bDeltaY &&
			!Section->OverlapsWithSections(MovieSceneSections, TargetRowIndex - Section->GetRowIndex(), DeltaTime))
		{
			Section->MoveSection(DeltaTime, DraggedKeyHandles);
			Section->SetRowIndex(TargetRowIndex);
		}
		else
		{
			if (bDeltaY &&
				!Section->OverlapsWithSections(MovieSceneSections, TargetRowIndex - Section->GetRowIndex(), 0.f))
			{
				Section->SetRowIndex(TargetRowIndex);
			}

			if (bDeltaX)
			{
				if (!Section->OverlapsWithSections(MovieSceneSections, 0, DeltaTime))
				{
					Section->MoveSection(DeltaTime, DraggedKeyHandles);
				}
				else
				{
					// Find the borders of where you can move to
					TRange<float> SectionBoundaries = GetSectionBoundaries(Section, Handle.TrackNode);

					float LeftMovementMaximum = SectionBoundaries.GetLowerBoundValue() - Section->GetStartTime();
					float RightMovementMaximum = SectionBoundaries.GetUpperBoundValue() - Section->GetEndTime();

					// Tell the section to move itself and any data it has
					Section->MoveSection( FMath::Clamp(DeltaTime, LeftMovementMaximum, RightMovementMaximum), DraggedKeyHandles );
				}
			}
		}
	}

	MouseDownTime = NewMouseDownTime;
}

void FMoveKeys::OnBeginDrag(const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea)
{
	check( SelectedKeys.Num() > 0 )

	SSection::DisableLayoutRegeneration();

	FDefaultKeySnappingCandidates SnapCandidates(SelectedKeys);
	SnapField = FSequencerSnapField(Sequencer, SnapCandidates);

	// Begin an editor transaction and mark the section as transactional so it's state will be saved
	BeginTransaction( TArray< FSectionHandle >(), NSLOCTEXT("Sequencer", "MoveKeysTransaction", "Move Keys") );

	const float MouseTime = VirtualTrackArea.PixelToTime(LocalMousePos.X);

	TSet<UMovieSceneSection*> ModifiedSections;
	for( FSelectedKey SelectedKey : SelectedKeys )
	{
		UMovieSceneSection* OwningSection = SelectedKey.Section;

		RelativeOffsets.Add(SelectedKey, SelectedKey.KeyArea->GetKeyTime(SelectedKey.KeyHandle.GetValue()) - MouseTime);

		// Only modify sections once
		if( !ModifiedSections.Contains( OwningSection ) )
		{
			OwningSection->SetFlags( RF_Transactional );

			// Save the current state of the section
			OwningSection->Modify();

			// Section has been modified
			ModifiedSections.Add( OwningSection );
		}
	}
}

void FMoveKeys::OnDrag( const FPointerEvent& MouseEvent, const FVector2D& LocalMousePos, const FVirtualTrackArea& VirtualTrackArea )
{
	float MouseTime = VirtualTrackArea.PixelToTime(LocalMousePos.X);
	float DistanceMoved = MouseTime - VirtualTrackArea.PixelToTime(LocalMousePos.X - MouseEvent.GetCursorDelta().X);

	if( DistanceMoved == 0.0f )
	{
		return;
	}

	// Snapping
	if ( Settings->GetIsSnapEnabled() )
	{
		TArray<float> KeyTimes;
		for( FSelectedKey SelectedKey : SelectedKeys )
		{
			KeyTimes.Add(MouseTime + RelativeOffsets.FindRef(SelectedKey));
		}

		float SnapThresold = VirtualTrackArea.PixelToTime(10.f) - VirtualTrackArea.PixelToTime(0.f);

		TOptional<FSequencerSnapField::FSnapResult> SnappedTime;

		if (Settings->GetSnapKeyTimesToKeys())
		{
			SnappedTime = SnapField->Snap(KeyTimes, SnapThresold);
		}

		if (!SnappedTime.IsSet() && Settings->GetSnapKeyTimesToInterval())
		{
			SnappedTime = SnapToInterval(KeyTimes, SnapThresold, *Settings);
		}

		if (SnappedTime.IsSet())
		{
			MouseTime += SnappedTime->Snapped - SnappedTime->Original;
		}
	}

	float PrevNewKeyTime = FLT_MAX;

	for( FSelectedKey SelectedKey : SelectedKeys )
	{
		UMovieSceneSection* Section = SelectedKey.Section;

		TSharedPtr<IKeyArea>& KeyArea = SelectedKey.KeyArea;

		float NewKeyTime = MouseTime + RelativeOffsets.FindRef(SelectedKey);
		float CurrentTime = KeyArea->GetKeyTime( SelectedKey.KeyHandle.GetValue() );

		// Tell the key area to move the key.  We reset the key index as a result of the move because moving a key can change it's internal index 
		KeyArea->MoveKey( SelectedKey.KeyHandle.GetValue(), NewKeyTime - CurrentTime );

		// If the key moves outside of the section resize the section to fit the key
		// @todo Sequencer - Doesn't account for hitting other sections 
		if( NewKeyTime > Section->GetEndTime() )
		{
			Section->SetEndTime( NewKeyTime );
		}
		else if( NewKeyTime < Section->GetStartTime() )
		{
			Section->SetStartTime( NewKeyTime );
		}

		if (PrevNewKeyTime == FLT_MAX)
		{
			PrevNewKeyTime = NewKeyTime;
		}
		else if (!FMath::IsNearlyEqual(NewKeyTime, PrevNewKeyTime))
		{
			PrevNewKeyTime = -FLT_MAX;
		}
	}

	// Snap the play time to the new dragged key time if all the keyframes were dragged to the same time
	if (Settings->GetSnapPlayTimeToDraggedKey() && PrevNewKeyTime != FLT_MAX && PrevNewKeyTime != -FLT_MAX)
	{
		Sequencer.SetGlobalTime(PrevNewKeyTime);
	}
}

void FMoveKeys::OnEndDrag()
{
	EndTransaction();
	SSection::EnableLayoutRegeneration();
}