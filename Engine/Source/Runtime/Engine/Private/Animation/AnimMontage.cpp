// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimMontage.cpp: Montage classes that contains slots
=============================================================================*/ 

#include "EnginePrivate.h"
#include "AnimationUtils.h"
#include "AnimationRuntime.h"

///////////////////////////////////////////////////////////////////////////
//
void FBranchingPoint::RefreshTriggerOffset(EAnimEventTriggerOffsets::Type PredictedOffsetType)
{
	if(PredictedOffsetType == EAnimEventTriggerOffsets::NoOffset || TriggerTimeOffset == 0.f)
	{
		TriggerTimeOffset = GetTriggerTimeOffsetForType(PredictedOffsetType);
	}
}

///////////////////////////////////////////////////////////////////////////
//
UAnimMontage::UAnimMontage(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bAnimBranchingPointNeedsSort = true;
	BlendInTime = 0.25f;
	BlendOutTime = 0.25f;
	RootMotionRootLock = ERootMotionRootLock::RefPose;
}

bool UAnimMontage::IsValidSlot(FName InSlotName) const
{
	for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
	{
		if ( SlotAnimTracks[I].SlotName == InSlotName )
		{
			// if data is there, return true. Otherwise, it doesn't matter
			return ( SlotAnimTracks[I].AnimTrack.AnimSegments.Num() >  0 );
		}
	}

	return false;
}

const FAnimTrack * UAnimMontage::GetAnimationData(FName InSlotName) const
{
	for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
	{
		if ( SlotAnimTracks[I].SlotName == InSlotName )
		{
			// if data is there, return true. Otherwise, it doesn't matter
			return &( SlotAnimTracks[I].AnimTrack );
		}
	}

	return NULL;
}

#if WITH_EDITOR
/*
void UAnimMontage::SetAnimationMontage(FName SectionName, FName SlotName, UAnimSequence * Sequence)
{
	// if valid section
	if ( IsValidSectionName(SectionName) )
	{
		bool bNeedToAddSlot=true;
		bool bNeedToAddSection=true;
		// see if we need to replace current one first
		for ( int32 I=0; I<AnimMontages.Num() ; ++I )
		{
			if (AnimMontages(I).SectionName == SectionName)
			{
				// found the section, now find slot name
				for ( int32 J=0; J<AnimMontages(I).Animations.Num(); ++J )
				{
					if (AnimMontages(I).Animations(J).SlotName == SlotName)
					{
						AnimMontages(I).Animations(J).SlotAnim = Sequence;
						bNeedToAddSlot = false;
						break;
					}
				}

				// we didn't find any slot that works, break it
				if (bNeedToAddSlot)
				{
					AnimMontages(I).Animations.Add(FSlotAnimTracks(SlotName, Sequence));
				}

				// we found section, no need to add section
				bNeedToAddSection = false;
				break;
			}
		}

		if (bNeedToAddSection)
		{
			int32 NewIndex = AnimMontages.Add(FMontageSlot(SectionName));
			AnimMontages(NewIndex).Animations.Add(FSlotAnimTracks(SlotName, Sequence));
		}

		// rebuild section data to sync runtime data
		BuildSectionData();
	}	
}*/
#endif

bool UAnimMontage::IsWithinPos(int32 FirstIndex, int32 SecondIndex, float CurrentTime) const
{
	float StartTime;
	float EndTime;
	if ( CompositeSections.IsValidIndex(FirstIndex) )
	{
		StartTime = CompositeSections[FirstIndex].StartTime;
	}
	else // if first index isn't valid, set to be 0.f, so it starts from reset
	{
		StartTime = 0.f;
	}

	if ( CompositeSections.IsValidIndex(SecondIndex) )
	{
		EndTime = CompositeSections[SecondIndex].StartTime;
	}
	else // if end index isn't valid, set to be BIG_NUMBER
	{
		// @todo anim, I don't know if using SequenceLength is better or BIG_NUMBER
		// I don't think that'd matter. 
		EndTime = SequenceLength;
	}

	return (StartTime <= CurrentTime && EndTime > CurrentTime);
}

float UAnimMontage::CalculatePos(FCompositeSection &Section, float PosWithinCompositeSection) const
{
	float Offset = Section.StartTime;
	Offset += PosWithinCompositeSection;
	// @todo anim
	return Offset;
}

int32 UAnimMontage::GetSectionIndexFromPosition(float Position) const
{
	for (int32 I=0; I<CompositeSections.Num(); ++I)
	{
		// if within
		if( IsWithinPos(I, I+1, Position) )
		{
			return I;
		}
	}

	return INDEX_NONE;
}

int32 UAnimMontage::GetAnimCompositeSectionIndexFromPos(float CurrentTime, float & PosWithinCompositeSection) const
{
	PosWithinCompositeSection = 0.f;

	for (int32 I=0; I<CompositeSections.Num(); ++I)
	{
		// if within
		if (IsWithinPos(I, I+1, CurrentTime))
		{
			PosWithinCompositeSection = CurrentTime - CompositeSections[I].StartTime;
			return I;
		}
	}

	return INDEX_NONE;
}

float UAnimMontage::GetSectionTimeLeftFromPos(float Position)
{
	const int32 SectionID = GetSectionIndexFromPosition(Position);
	if( SectionID != INDEX_NONE )
	{
		if( IsValidSectionIndex(SectionID+1) )
		{
			return (GetAnimCompositeSection(SectionID+1).StartTime - Position);
		}
		else
		{
			return (SequenceLength - Position);
		}
	}

	return -1.f;
}

const FCompositeSection& UAnimMontage::GetAnimCompositeSection(int32 SectionIndex) const
{
	check ( CompositeSections.IsValidIndex(SectionIndex) );
	return CompositeSections[SectionIndex];
}

FCompositeSection& UAnimMontage::GetAnimCompositeSection(int32 SectionIndex)
{
	check ( CompositeSections.IsValidIndex(SectionIndex) );
	return CompositeSections[SectionIndex];
}

int32 UAnimMontage::GetSectionIndex(FName InSectionName) const
{
	// I can have operator== to check SectionName, but then I have to construct
	// empty FCompositeSection all the time whenever I search :(
	for (int32 I=0; I<CompositeSections.Num(); ++I)
	{
		if ( CompositeSections[I].SectionName == InSectionName ) 
		{
			return I;
		}
	}

	return INDEX_NONE;
}

FName UAnimMontage::GetSectionName(int32 SectionIndex) const
{
	if ( CompositeSections.IsValidIndex(SectionIndex) )
	{
		return CompositeSections[SectionIndex].SectionName;
	}

	return NAME_None;
}

bool UAnimMontage::IsValidSectionName(FName InSectionName) const
{
	return GetSectionIndex(InSectionName) != INDEX_NONE;
}

bool UAnimMontage::IsValidSectionIndex(int32 SectionIndex) const
{
	return (CompositeSections.IsValidIndex(SectionIndex));
}

void UAnimMontage::GetSectionStartAndEndTime(int32 SectionIndex, float& OutStartTime, float& OutEndTime) const
{
	OutStartTime = 0.f;
	OutEndTime = SequenceLength;
	if ( IsValidSectionIndex(SectionIndex) )
	{
		OutStartTime = GetAnimCompositeSection(SectionIndex).StartTime;		
	}

	if ( IsValidSectionIndex(SectionIndex + 1))
	{
		OutEndTime = GetAnimCompositeSection(SectionIndex + 1).StartTime;		
	}
}

float UAnimMontage::GetSectionLength(int32 SectionIndex) const
{
	float StartTime = 0.f;
	float EndTime = SequenceLength;
	if ( IsValidSectionIndex(SectionIndex) )
	{
		StartTime = GetAnimCompositeSection(SectionIndex).StartTime;		
	}

	if ( IsValidSectionIndex(SectionIndex + 1))
	{
		EndTime = GetAnimCompositeSection(SectionIndex + 1).StartTime;		
	}

	return EndTime - StartTime;
}

#if WITH_EDITOR
int32 UAnimMontage::AddAnimCompositeSection(FName InSectionName, float StartTime)
{
	FCompositeSection NewSection;

	// make sure same name doesn't exists
	if ( InSectionName != NAME_None )
	{
		NewSection.SectionName = InSectionName;
	}
	else
	{
		// just give default name
		NewSection.SectionName = FName(*FString::Printf(TEXT("Section%d"), CompositeSections.Num()+1));
	}

	// we already have that name
	if ( GetSectionIndex(InSectionName)!=INDEX_NONE )
	{
		UE_LOG(LogAnimation, Warning, TEXT("AnimCompositeSection : %s(%s) already exists. Choose different name."), 
			*NewSection.SectionName.ToString(), *InSectionName.ToString());
		return INDEX_NONE;
	}

	NewSection.StartTime = StartTime;

	// we'd like to sort them in the order of time
	int32 NewSectionIndex = CompositeSections.Add(NewSection);

	// when first added, just make sure to link previous one to add me as next if previous one doesn't have any link
	// it's confusing first time when you add this data
	int32 PrevSectionIndex = NewSectionIndex-1;
	if ( CompositeSections.IsValidIndex(PrevSectionIndex) )
	{
		if (CompositeSections[PrevSectionIndex].NextSectionName == NAME_None)
		{
			CompositeSections[PrevSectionIndex].NextSectionName = InSectionName;
		}
	}

	return NewSectionIndex;
}

bool UAnimMontage::DeleteAnimCompositeSection(int32 SectionIndex)
{
	if ( CompositeSections.IsValidIndex(SectionIndex) )
	{
		CompositeSections.RemoveAt(SectionIndex);
		return true;
	}

	return false;
}
void UAnimMontage::SortAnimCompositeSectionByPos()
{
	// sort them in the order of time
	struct FCompareFCompositeSection
	{
		FORCEINLINE bool operator()( const FCompositeSection &A, const FCompositeSection &B ) const
		{
			return A.StartTime < B.StartTime;
		}
	};
	CompositeSections.Sort( FCompareFCompositeSection() );
}

#endif	//WITH_EDITOR

void UAnimMontage::PostLoad()
{
	Super::PostLoad();

	// copy deprecated variable to new one, temporary code to keep data copied. Am deleting it right after this
	for ( auto SlotIter = SlotAnimTracks.CreateIterator() ; SlotIter ; ++SlotIter)
	{
		FAnimTrack & Track = (*SlotIter).AnimTrack;
		for ( auto SegIter = Track.AnimSegments.CreateIterator() ; SegIter ; ++SegIter )
		{
			FAnimSegment & Segment = (*SegIter);
			if ( Segment.AnimStartOffset_DEPRECATED!=0.f )
			{
				Segment.AnimStartTime = Segment.AnimStartOffset_DEPRECATED;
				Segment.AnimStartOffset_DEPRECATED = 0.f;
			}
			if ( Segment.AnimEndOffset_DEPRECATED!=0.f )
			{
				Segment.AnimEndTime = Segment.AnimEndOffset_DEPRECATED;
				Segment.AnimEndOffset_DEPRECATED = 0.f;
			}
		}
			}

	for ( auto CompositeIter = CompositeSections.CreateIterator(); CompositeIter; ++CompositeIter )
	{
		FCompositeSection & Composite = (*CompositeIter);
		if (Composite.StarTime_DEPRECATED!=0.f)
		{
			Composite.StartTime = Composite.StarTime_DEPRECATED;
			Composite.StarTime_DEPRECATED = 0.f;
		}
	}

	SortAnimBranchingPointByTime();

	// find preview base pose if it can
#if WITH_EDITORONLY_DATA
	if ( IsValidAdditive() && PreviewBasePose == NULL )
	{
		for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
		{
			if ( SlotAnimTracks[I].AnimTrack.AnimSegments.Num() > 0 )
			{
				UAnimSequence * Sequence = Cast<UAnimSequence>(SlotAnimTracks[I].AnimTrack.AnimSegments[0].AnimReference);
				if ( Sequence && Sequence->RefPoseSeq )
				{
					PreviewBasePose = Sequence->RefPoseSeq;
					MarkPackageDirty();
					break;
				}
			}
		}
	}

	// verify if skeleton matches, otherwise clear it, this can happen if anim sequence has been modified when this hasn't been loaded. 
	USkeleton * MySkeleton = GetSkeleton();
	for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
	{
		if ( SlotAnimTracks[I].AnimTrack.AnimSegments.Num() > 0 )
		{
			UAnimSequence * Sequence = Cast<UAnimSequence>(SlotAnimTracks[I].AnimTrack.AnimSegments[0].AnimReference);
			if ( Sequence && Sequence->GetSkeleton() != MySkeleton )
			{
				SlotAnimTracks[I].AnimTrack.AnimSegments[0].AnimReference = 0;
				MarkPackageDirty();
				break;
			}
		}
	}
#endif // WITH_EDITORONLY_DATA
}

void UAnimMontage::SortAnimBranchingPointByTime()
{
	struct FCompareFBranchingPointTime
	{
		FORCEINLINE bool operator()( const FBranchingPoint &A, const FBranchingPoint &B ) const
		{
			return A.GetTriggerTime() < B.GetTriggerTime();
		}
	};

	BranchingPoints.Sort( FCompareFBranchingPointTime() );

	bAnimBranchingPointNeedsSort = false;
}

#if WITH_EDITOR
void UAnimMontage::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if( PropertyName == FName(TEXT("BranchingPoints")) || PropertyName == FName(TEXT("Time")) )
	{
		bAnimBranchingPointNeedsSort = true;
	}
}
#endif // WITH_EDITOR

const FBranchingPoint * UAnimMontage::FindFirstBranchingPoint(float StartTrackPos, float EndTrackPos)
{
	if( bAnimBranchingPointNeedsSort )
	{
		SortAnimBranchingPointByTime();
	}

	const bool bPlayingBackwards = (EndTrackPos < StartTrackPos);
	if( !bPlayingBackwards )
	{
		for( int32 Index=0; Index<BranchingPoints.Num(); Index++)
		{
			const FBranchingPoint & BranchingPoint = BranchingPoints[Index];
			if( BranchingPoint.GetTriggerTime() <= StartTrackPos )
			{
				continue;
			}
			if( BranchingPoint.GetTriggerTime() > EndTrackPos )
			{
				break;
			}
			return &BranchingPoint;
		}
	}
	else
	{
		for( int32 Index=BranchingPoints.Num()-1; Index>=0; Index--)
		{
			const FBranchingPoint & BranchingPoint = BranchingPoints[Index];
			if( BranchingPoint.GetTriggerTime() >= StartTrackPos )
			{
				continue;
			}
			if( BranchingPoint.GetTriggerTime() < EndTrackPos )
			{
				break;
			}
			return &BranchingPoint;
		}
	}
	return NULL;
}

bool UAnimMontage::IsValidAdditive() const
{
	// if first one is additive, this is additive
	if ( SlotAnimTracks.Num() > 0 )
	{
		for (int32 I=0; I<SlotAnimTracks.Num(); ++I)
		{
			if (!SlotAnimTracks[I].AnimTrack.IsAdditive())
			{
				return false;
			}
		}

		return true;
	}

	return false;
}

EAnimEventTriggerOffsets::Type UAnimMontage::CalculateOffsetFromSections(float Time) const
{
	for(auto Iter = CompositeSections.CreateConstIterator(); Iter; ++Iter)
	{
		float SectionTime = Iter->StartTime;
		if(SectionTime == Time)
		{
			return EAnimEventTriggerOffsets::OffsetBefore;
		}
	}
	return EAnimEventTriggerOffsets::NoOffset;
}

#if WITH_EDITOR
EAnimEventTriggerOffsets::Type UAnimMontage::CalculateOffsetForBranchingPoint(float BranchingPointDisplayTime) const
{
	EAnimEventTriggerOffsets::Type Offset = Super::CalculateOffsetForNotify(BranchingPointDisplayTime);
	if(Offset == EAnimEventTriggerOffsets::NoOffset)
	{
		Offset = CalculateOffsetFromSections(BranchingPointDisplayTime);
	}
	return Offset;
}

EAnimEventTriggerOffsets::Type UAnimMontage::CalculateOffsetForNotify(float NotifyDisplayTime) const
{
	EAnimEventTriggerOffsets::Type Offset = Super::CalculateOffsetForNotify(NotifyDisplayTime);
	if(Offset == EAnimEventTriggerOffsets::NoOffset)
	{
		Offset = CalculateOffsetFromSections(NotifyDisplayTime);
		if(Offset == EAnimEventTriggerOffsets::NoOffset)
		{
			for(auto Iter = BranchingPoints.CreateConstIterator(); Iter; ++Iter)
			{
				float BranchTime = Iter->DisplayTime;
				if(BranchTime == NotifyDisplayTime)
				{
					return EAnimEventTriggerOffsets::OffsetBefore;
				}
			}
		}
	}
	return Offset;
}
#endif

/** Extract RootMotion Transform from a contiguous Track position range.
 * *CONTIGUOUS* means that if playing forward StartTractPosition < EndTrackPosition.
 * No wrapping over if looping. No jumping across different sections.
 * So the AnimMontage has to break the update into contiguous pieces to handle those cases.
 *
 * This does handle Montage playing backwards (StartTrackPosition > EndTrackPosition).
 *
 * It will break down the range into steps if needed to handle looping animations, or different animations.
 * These steps will be processed sequentially, and output the RootMotion transform in component space.
 */
FTransform UAnimMontage::ExtractRootMotionFromTrackRange(float StartTrackPosition, float EndTrackPosition) const
{
	FRootMotionMovementParams RootMotion;

	// For now assume Root Motion only comes from first track.
	if( SlotAnimTracks.Num() > 0 )
	{
		const FAnimTrack & SlotAnimTrack = SlotAnimTracks[0].AnimTrack;

		// Get RootMotion pieces from this track.
		// We can deal with looping animations, or multiple animations. So we break those up into sequential operations.
		// (Animation, StartFrame, EndFrame) so we can then extract root motion sequentially.
		TArray<FRootMotionExtractionStep> RootMotionExtractionSteps;
		SlotAnimTrack.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, StartTrackPosition, EndTrackPosition);

		UE_LOG(LogRootMotion, Log,  TEXT("\tUAnimMontage::ExtractRootMotionForTrackRange, NumSteps: %d, StartTrackPosition: %.3f, EndTrackPosition: %.3f"), 
			RootMotionExtractionSteps.Num(), StartTrackPosition, EndTrackPosition);

		// Process root motion steps if any
		if( RootMotionExtractionSteps.Num() > 0 )
		{
			FTransform InitialTransform, StartTransform, EndTransform;

			// Go through steps sequentially, extract root motion, and accumulate it.
			// This has to be done in order so root motion translation & rotation is applied properly (as translation is relative to rotation)
			for(int32 StepIndex=0; StepIndex<RootMotionExtractionSteps.Num(); StepIndex++)
			{
				const FRootMotionExtractionStep & CurrentStep = RootMotionExtractionSteps[StepIndex];
				CurrentStep.AnimSequence->ExtractRootTrack(0.f, InitialTransform, NULL);
				CurrentStep.AnimSequence->ExtractRootTrack(CurrentStep.StartPosition, StartTransform, NULL);
				CurrentStep.AnimSequence->ExtractRootTrack(CurrentStep.EndPosition, EndTransform, NULL);
				
				// Transform to Component Space Rotation (inverse root transform from first frame)
				const FTransform RootToComponentRot = FTransform(InitialTransform.GetRotation().Inverse());
				StartTransform = RootToComponentRot * StartTransform;
				EndTransform = RootToComponentRot * EndTransform;

				FTransform DeltaTransform = EndTransform.GetRelativeTransform(StartTransform);

				// Filter out rotation 
				if( !bEnableRootMotionRotation )
				{
					DeltaTransform.SetRotation(FQuat::Identity);
				}
				// Filter out translation
				if( !bEnableRootMotionTranslation )
				{
					DeltaTransform.SetTranslation(FVector::ZeroVector);
				}

				RootMotion.Accumulate(DeltaTransform);

				UE_LOG(LogRootMotion, Log,  TEXT("\t\tCurrentStep: %d, StartPos: %.3f, EndPos: %.3f, Anim: %s DeltaTransform Translation: %s, Rotation: %s"),
					StepIndex, CurrentStep.StartPosition, CurrentStep.EndPosition, *CurrentStep.AnimSequence->GetName(), 
					*DeltaTransform.GetTranslation().ToCompactString(), *DeltaTransform.GetRotation().Rotator().ToCompactString() );
			}
		}
	}

	UE_LOG(LogRootMotion, Log,  TEXT("\tUAnimMontage::ExtractRootMotionForTrackRange RootMotionTransform: Translation: %s, Rotation: %s"),
		*RootMotion.RootMotionTransform.GetTranslation().ToCompactString(), *RootMotion.RootMotionTransform.GetRotation().Rotator().ToCompactString() );

	return RootMotion.RootMotionTransform;
}

#if WITH_EDITOR
void UAnimMontage::EvaluateCurveData(class UAnimInstance* Instance, float CurrentTime, float BlendWeight ) const
{
	Super::EvaluateCurveData(Instance, CurrentTime, BlendWeight);

	// I also need to evaluate curve of the animation
	// for now we only get the first slot
	// in the future, we'll need to do based on highest weight?
	// first get all the montage instance weight this slot node has
	if ( SlotAnimTracks.Num() > 0 )
	{
		const FAnimTrack & Track = SlotAnimTracks[0].AnimTrack;
		for (int32 I=0; I<Track.AnimSegments.Num(); ++I)
		{
			const FAnimSegment & AnimSegment = Track.AnimSegments[I];

			float PositionInAnim = 0.f;
			float Weight = 0.f;
			UAnimSequenceBase* AnimRef = AnimSegment.GetAnimationData(CurrentTime, PositionInAnim, Weight);
			// make this to be 1 function
			if ( AnimRef && Weight > ZERO_ANIMWEIGHT_THRESH )
			{
				// todo anim: hack - until we fix animcomposite
				UAnimSequence * Sequence = Cast<UAnimSequence>(AnimRef);
				if ( Sequence )
				{
					Sequence->EvaluateCurveData(Instance, PositionInAnim, BlendWeight*Weight);
				}
			}
		}	
	}
}
#endif	//WITH_EDITOR

//////////////////////////////////////////////////////////////////////////////////////////////
// MontageInstance
/////////////////////////////////////////////////////////////////////////////////////////////

void FAnimMontageInstance::Play(float InPlayRate)
{
	bPlaying = true;
	PlayRate = InPlayRate;
	if( Montage )
	{
		BlendTime = Montage->BlendInTime * DefaultBlendTimeMultiplier;
	}
	DesiredWeight = 1.f;
}

void FAnimMontageInstance::Stop(float BlendOut, bool bInterrupt)
{
	// overwrite bInterrupted if it hasn't already interrupted
	// once interrupted, you don't go back to non-interrupted
	if (!bInterrupted && bInterrupt)
	{
		bInterrupted = bInterrupt;
	}

	// It is already stopped, but if BlendOut < BlendTime, that means 
	// we need to readjust BlendTime, we don't want old animation to blend longer than
	// new animation
	if (DesiredWeight == 0.f)
	{
		// it is already stopped, but new montage blendtime is shorter than what 
		// I'm blending out, that means this needs to readjust blendtime
		// that way we don't accumulate old longer blendtime for newer montage to play
		if (BlendOut < BlendTime)
		{
			BlendTime = BlendOut;
		}

		return;
	}

	DesiredWeight = 0.f;

	if( Montage )
	{
		// do not use default Montage->BlendOut  Time
		// depending on situation, the BlendOut time changes
		// check where this function gets called and see how we calculate BlendTime
		BlendTime = BlendOut;

		if (OnMontageBlendingOutStarted.IsBound())
		{
			OnMontageBlendingOutStarted.Execute(Montage, bInterrupted);
		}
	}

	if (BlendTime == 0.0f)
	{
		bPlaying = false;
	}
}

void FAnimMontageInstance::Pause()
{
	bPlaying = false;
}

void FAnimMontageInstance::Initialize(class UAnimMontage * InMontage)
{
	if (InMontage)
	{
		Montage = InMontage;
		Position = 0.f;
		DesiredWeight = 1.f;

		RefreshNextPrevSections();
	}
}

void FAnimMontageInstance::RefreshNextPrevSections()
{
	// initialize next section
	if ( Montage->CompositeSections.Num() > 0 )
	{
		NextSections.Empty(Montage->CompositeSections.Num());
		NextSections.AddUninitialized(Montage->CompositeSections.Num());
		PrevSections.Empty(Montage->CompositeSections.Num());
		PrevSections.AddUninitialized(Montage->CompositeSections.Num());

		for (int32 I=0; I<Montage->CompositeSections.Num(); ++I)
		{
			PrevSections[I] = INDEX_NONE;
		}

		for (int32 I=0; I<Montage->CompositeSections.Num(); ++I)
		{
			FCompositeSection & Section = Montage->CompositeSections[I];
			int32 NextSectionIdx = Montage->GetSectionIndex(Section.NextSectionName);
			NextSections[I] = NextSectionIdx;
			if (NextSections.IsValidIndex(NextSectionIdx))
			{
				PrevSections[NextSectionIdx] = I;
			}
		}
	}
}

void FAnimMontageInstance::AddReferencedObjects( FReferenceCollector& Collector )
{
	if (Montage && Montage->GetOuter() == GetTransientPackage())
	{
		Collector.AddReferencedObject(Montage);
	}
}

void FAnimMontageInstance::Terminate()
{
	if ( Montage == NULL )
	{
		return;
	}
	UAnimMontage* OldMontage = Montage;
	Montage = NULL;

	// terminating, trigger end
	if (OnMontageEnded.IsBound())
	{
		OnMontageEnded.Execute(OldMontage, bInterrupted);
	}
}

bool FAnimMontageInstance::ChangeNextSection(FName SectionName, FName NextSectionName)
{
	int32 SectionID = Montage->GetSectionIndex(SectionName);
	int32 NextSectionID = Montage->GetSectionIndex(NextSectionName);
	bool bHasValidNextSection = NextSections.IsValidIndex(SectionID);
	
	// disconnect prev section
	if ( bHasValidNextSection && 
		NextSections[SectionID] != INDEX_NONE &&
		 PrevSections.IsValidIndex(NextSections[SectionID]) )
	{
		PrevSections[NextSections[SectionID]] = INDEX_NONE;
	}
	// update in-reverse next section
	if ( PrevSections.IsValidIndex(NextSectionID) )
	{
		PrevSections[NextSectionID] = SectionID;
	}
	// update next section for the SectionID
	// NextSection can be invalid
	if ( bHasValidNextSection )
	{
		NextSections[SectionID] = NextSectionID;
		return true;
	}

	return false;
}

FName FAnimMontageInstance::GetCurrentSection() const
{
	if ( Montage )
	{
		float CurrentPosition;
		const int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(Position, CurrentPosition);
		if ( Montage->IsValidSectionIndex(CurrentSectionIndex) )
		{
			FCompositeSection& CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);
			return CurrentSection.SectionName;
		}
	}

	return NAME_None;
}

bool FAnimMontageInstance::ChangePositionToSection(FName SectionName, bool bEndOfSection)
{
	const int32 SectionID = Montage->GetSectionIndex(SectionName);
	if ( Montage->IsValidSectionIndex(SectionID) )
	{
		FCompositeSection & CurSection = Montage->GetAnimCompositeSection(SectionID);
		Position = Montage->CalculatePos(CurSection, bEndOfSection? Montage->GetSectionLength(SectionID) - KINDA_SMALL_NUMBER : 0.0f);
		return true;
	}

	return false;
}

void FAnimMontageInstance::UpdateWeight(float DeltaTime)
{
	if ( IsValid() )
	{
		PreviousWeight = Weight;

		// update weight
		FAnimationRuntime::TickBlendWeight(DeltaTime, DesiredWeight, Weight, BlendTime);
	}
}

bool FAnimMontageInstance::SimulateAdvance(float DeltaTime, float & InOutPosition, FRootMotionMovementParams & OutRootMotionParams) const
{
	if( !IsValid() )
	{
		return false;
	}

	const float CombinedPlayRate = PlayRate * Montage->RateScale;
	const bool bPlayingForward = (CombinedPlayRate > 0.f);

	const bool bExtractRootMotion = (Montage->bEnableRootMotionRotation || Montage->bEnableRootMotionTranslation);

	float DesiredDeltaMove = CombinedPlayRate * DeltaTime;
	float OriginalMoveDelta = DesiredDeltaMove;

	while( (FMath::Abs(DesiredDeltaMove) > KINDA_SMALL_NUMBER) && ((OriginalMoveDelta * DesiredDeltaMove) > 0.f) )
	{
		// Get position relative to current montage section.
		float PosInSection;
		int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(InOutPosition, PosInSection);

		if( Montage->IsValidSectionIndex(CurrentSectionIndex) )
		{
			const FCompositeSection & CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);

			// we need to advance within section
			check(NextSections.IsValidIndex(CurrentSectionIndex));
			const int32 NextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];

			// Find end of current section. We only update one section at a time.
			const float CurrentSectionLength = Montage->GetSectionLength(CurrentSectionIndex);
			float StoppingPosInSection = CurrentSectionLength;

			// Also look for a branching point. If we have one, stop there first to handle it.
			const float CurrentSectionEndPos = CurrentSection.StartTime + CurrentSectionLength;

			// Update position within current section.
			// Note that we explicitly disallow looping there, we handle it ourselves to simplify code by not having to handle time wrapping around in multiple places.
			const float PrevPosInSection = PosInSection;
			const ETypeAdvanceAnim AdvanceType = FAnimationRuntime::AdvanceTime(false, DesiredDeltaMove, PosInSection, StoppingPosInSection);
			// ActualDeltaPos == ActualDeltaMove since we break down looping in multiple steps. So we don't have to worry about time wrap around here.
			const float ActualDeltaPos = PosInSection - PrevPosInSection;
			const float ActualDeltaMove = ActualDeltaPos;

			// Decrease actual move from desired move. We'll take another iteration if there is anything left.
			DesiredDeltaMove -= ActualDeltaMove;

			float PrevPosition = Position;
			InOutPosition = FMath::Clamp<float>(InOutPosition + ActualDeltaPos, CurrentSection.StartTime, CurrentSectionEndPos);

			if( FMath::Abs(ActualDeltaMove) > 0.f )
			{
				// Extract Root Motion for this time slice, and accumulate it.
				if( bExtractRootMotion )
				{
					OutRootMotionParams.Accumulate(Montage->ExtractRootMotionFromTrackRange(PrevPosition, InOutPosition));
				}

				// if we reached end of section, and we were not processing a branching point, and no events has messed with out current position..
				// .. Move to next section.
				// (this also handles looping, the same as jumping to a different section).
				if( AdvanceType != ETAA_Default )
				{
					// Get recent NextSectionIndex in case it's been changed by previous events.
					const int32 RecentNextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];
					if( RecentNextSectionIndex != INDEX_NONE )
					{
						float LatestNextSectionStartTime;
						float LatestNextSectionEndTime;
						Montage->GetSectionStartAndEndTime(RecentNextSectionIndex, LatestNextSectionStartTime, LatestNextSectionEndTime);
						// Jump to next section's appropriate starting point (start or end).
						InOutPosition = bPlayingForward ? LatestNextSectionStartTime : (LatestNextSectionEndTime - KINDA_SMALL_NUMBER); // remain within section
					}
				}
			}
			else
			{
				break;
			}
		}
		else
		{
			// stop and leave this loop
			break;
		}
	}

	return true;
}

void FAnimMontageInstance::Advance(float DeltaTime, FRootMotionMovementParams & OutRootMotionParams)
{
	if( IsValid() )
	{
#if WITH_EDITOR
		// this is necessary and it is not easy to do outside of here
		// since undo also can change composite sections
		if( (Montage->CompositeSections.Num() != NextSections.Num()) || (Montage->CompositeSections.Num() != PrevSections.Num()))
		{
			RefreshNextPrevSections();
		}
#endif
		// If this Montage has no weight, it should be terminated.
		if( (Weight <= ZERO_ANIMWEIGHT_THRESH) && (DesiredWeight <= ZERO_ANIMWEIGHT_THRESH) )
		{
			// nothing else to do
			Terminate();
			return;
		}
		
		// if no weight, no reason to update, and if not playing, we don't need to advance
		// this portion is to advance position
		// If we just reached zero weight, still tick this frame to fire end of animation events.
		if( bPlaying && (Weight > ZERO_ANIMWEIGHT_THRESH || PreviousWeight > ZERO_ANIMWEIGHT_THRESH) )
		{
			const float CombinedPlayRate = PlayRate * Montage->RateScale;
			const bool bPlayingForward = (CombinedPlayRate > 0.f);

			const bool bExtractRootMotion = (Montage->bEnableRootMotionRotation || Montage->bEnableRootMotionTranslation);
			
			float DesiredDeltaMove = CombinedPlayRate * DeltaTime;
			float OriginalMoveDelta = DesiredDeltaMove;

			while( bPlaying && (FMath::Abs(DesiredDeltaMove) > KINDA_SMALL_NUMBER) && ((OriginalMoveDelta * DesiredDeltaMove) > 0.f) )
			{
				// Get position relative to current montage section.
				float PosInSection;
				int32 CurrentSectionIndex = Montage->GetAnimCompositeSectionIndexFromPos(Position, PosInSection);
				
				if( Montage->IsValidSectionIndex(CurrentSectionIndex) )
				{
					const FCompositeSection & CurrentSection = Montage->GetAnimCompositeSection(CurrentSectionIndex);

					// we need to advance within section
					check(NextSections.IsValidIndex(CurrentSectionIndex));
					const int32 NextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];

					// Find end of current section. We only update one section at a time.
					const float CurrentSectionLength = Montage->GetSectionLength(CurrentSectionIndex);
					float StoppingPosInSection = CurrentSectionLength;

					// Also look for a branching point. If we have one, stop there first to handle it.
					const float CurrentSectionEndPos = CurrentSection.StartTime + CurrentSectionLength;
					const FBranchingPoint * NextBranchingPoint = Montage->FindFirstBranchingPoint(Position, FMath::Clamp(Position + DesiredDeltaMove, CurrentSection.StartTime, CurrentSectionEndPos));
					if( NextBranchingPoint )
					{
						// get the first one to see if it's less than AnimEnd
						StoppingPosInSection = NextBranchingPoint->GetTriggerTime() - CurrentSection.StartTime;
					}

					// Update position within current section.
					// Note that we explicitly disallow looping there, we handle it ourselves to simplify code by not having to handle time wrapping around in multiple places.
					const float PrevPosInSection = PosInSection;
					const ETypeAdvanceAnim AdvanceType = FAnimationRuntime::AdvanceTime(false, DesiredDeltaMove, PosInSection, StoppingPosInSection);
					// ActualDeltaPos == ActualDeltaMove since we break down looping in multiple steps. So we don't have to worry about time wrap around here.
					const float ActualDeltaPos = PosInSection - PrevPosInSection;
					const float ActualDeltaMove = ActualDeltaPos;

					// Decrease actual move from desired move. We'll take another iteration if there is anything left.
					DesiredDeltaMove -= ActualDeltaMove;

					float PrevPosition = Position;
					Position = FMath::Clamp<float>(Position + ActualDeltaPos, CurrentSection.StartTime, CurrentSectionEndPos);

					if( FMath::Abs(ActualDeltaMove) > 0.f )
					{
						// Extract Root Motion for this time slice, and accumulate it.
						if( bExtractRootMotion && AnimInstance.IsValid() )
						{
							OutRootMotionParams.Accumulate( Montage->ExtractRootMotionFromTrackRange(PrevPosition, Position) );
						}

						// If about to reach the end of the montage...
						if( NextSectionIndex == INDEX_NONE )
						{
							// ... trigger blend out if within blend out time window.
							const float DeltaPosToEnd = bPlayingForward ? (CurrentSectionLength - PosInSection) : PosInSection;
							const float DeltaTimeToEnd = DeltaPosToEnd / CombinedPlayRate;
							const float BlendOutTime = FMath::Max<float>(Montage->BlendOutTime * DefaultBlendTimeMultiplier, KINDA_SMALL_NUMBER);
							if( DeltaTimeToEnd <= BlendOutTime )
							{
								Stop(DeltaTimeToEnd, false);
							}
						}

						// Delegate has to be called last in this loop
						// so that if this changes position, the new position will be applied in the next loop
						// first need to have event handler to handle it

						// Save position before firing events.
						const float PositionBeforeFiringEvents = Position;
						if( !bInterrupted )
						{
							HandleEvents(PrevPosition, Position, NextBranchingPoint);
						}

						// if we reached end of section, and we were not processing a branching point, and no events has messed with out current position..
						// .. Move to next section.
						// (this also handles looping, the same as jumping to a different section).
						if( (AdvanceType != ETAA_Default) && !NextBranchingPoint && (PositionBeforeFiringEvents == Position) )
						{
							// Get recent NextSectionIndex in case it's been changed by previous events.
							const int32 RecentNextSectionIndex = bPlayingForward ? NextSections[CurrentSectionIndex] : PrevSections[CurrentSectionIndex];

							if( RecentNextSectionIndex != INDEX_NONE )
							{
								float LatestNextSectionStartTime, LatestNextSectionEndTime;
								Montage->GetSectionStartAndEndTime(RecentNextSectionIndex, LatestNextSectionStartTime, LatestNextSectionEndTime);

								// Jump to next section's appropriate starting point (start or end).
								float EndOffset = KINDA_SMALL_NUMBER/2.f; //KINDA_SMALL_NUMBER/2 because we use KINDA_SMALL_NUMBER to offset notifies for triggering and SMALL_NUMBER is too small
								Position = bPlayingForward ? LatestNextSectionStartTime : (LatestNextSectionEndTime - EndOffset);
							}
						}
					}
					else
					{
						break;
					}
				}
				else
				{
					// stop and leave this loop
					Stop(Montage->BlendOutTime * DefaultBlendTimeMultiplier, false);
					break;
				}
			}
		}
	}

	// Update curves based on final position.
	if( (Weight > ZERO_ANIMWEIGHT_THRESH) && IsValid() )
	{
		Montage->EvaluateCurveData(AnimInstance.Get(), Position, Weight);
	}
}

void FAnimMontageInstance::TriggerEventHandler(FName EventName)
{
	if ( EventName!=NAME_None && AnimInstance.IsValid() )
	{
		FString FuncName = FString::Printf(TEXT("MontageBranchingPoint_%s"), *EventName.ToString());
		FName FuncFName = FName(*FuncName);

		UFunction* Function = AnimInstance.Get()->FindFunction(FuncFName);
		if( Function )
		{
			AnimInstance.Get()->ProcessEvent( Function, NULL );
		}
	}
}

void FAnimMontageInstance::HandleEvents(float PreviousTrackPos, float CurrentTrackPos, const FBranchingPoint * BranchingPoint)
{
	// Skip notifies and branching points if montage has been interrupted.
	if( bInterrupted )
	{
		return;
	}

	// now get active Notifies based on how it advanced
	if ( AnimInstance.IsValid() )
	{
		// first handle notifies
		{
			TArray<const FAnimNotifyEvent*> Notifies;

			// We already break up AnimMontage update to handle looping, so we guarantee that PreviousPos and CurrentPos are contiguous.
			Montage->GetAnimNotifiesFromDeltaPositions(PreviousTrackPos, CurrentTrackPos, Notifies);

			// now trigger notifies for all animations within montage
			// we'll do this for all slots for now
			for (auto SlotTrack = Montage->SlotAnimTracks.CreateIterator(); SlotTrack; ++SlotTrack)
			{
				if( AnimInstance->IsActiveSlotNode(SlotTrack->SlotName) )
				{
					for (auto AnimSegment = SlotTrack->AnimTrack.AnimSegments.CreateIterator() ; AnimSegment; ++AnimSegment)
					{
						AnimSegment->GetAnimNotifiesFromTrackPositions(PreviousTrackPos, CurrentTrackPos, Notifies);
					}
				}
			}

			// if Notifies 
			AnimInstance->AddAnimNotifies(Notifies, Weight);	
		}
	}

	// Trigger Branching Point event if we have one
	if( BranchingPoint )
	{
		TriggerEventHandler(BranchingPoint->EventName);
	}
}

#if WITH_EDITOR
bool UAnimMontage::GetAllAnimationSequencesReferred(TArray<UAnimSequence*>& AnimationSequences)
{
 	for (auto Iter=SlotAnimTracks.CreateConstIterator(); Iter; ++Iter)
 	{
 		const FSlotAnimationTrack& Track = (*Iter);
 		Track.AnimTrack.GetAllAnimationSequencesReferred(AnimationSequences);
 	}
 	return (AnimationSequences.Num() > 0);
}

void UAnimMontage::ReplaceReferredAnimations(const TMap<UAnimSequence*, UAnimSequence*>& ReplacementMap)
{
	for (auto Iter=SlotAnimTracks.CreateIterator(); Iter; ++Iter)
	{
		FSlotAnimationTrack& Track = (*Iter);
		Track.AnimTrack.ReplaceReferredAnimations(ReplacementMap);
	}
}
#endif