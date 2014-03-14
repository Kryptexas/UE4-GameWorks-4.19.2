// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	AnimCompositeBase.cpp: Anim Composite base class that contains AnimTrack data structure/interface
=============================================================================*/ 

#include "EnginePrivate.h"
#include "AnimationUtils.h"

///////////////////////////////////////////////////////
// FAnimSegment
///////////////////////////////////////////////////////

UAnimSequenceBase * FAnimSegment::GetAnimationData(float PositionInTrack, float & PositionInAnim, float & Weight) const
{
	if( IsInRange(PositionInTrack) )
	{
		if( AnimReference )
		{
			const float ValidPlayRate = GetValidPlayRate();

			// this result position should be pure position within animation
			float Delta = (PositionInTrack - StartPos);

			// LoopingCount should not be zero, and it should not get here, but just in case
			if (LoopingCount > 1)
			{
				// we need to consider looping count
				float AnimPlayLength = (AnimEndTime - AnimStartTime) / ValidPlayRate;
				Delta = FMath::Fmod(Delta, AnimPlayLength);
			}

			// no blending supported
			Weight = 1.f;
			PositionInAnim = AnimStartTime + Delta * ValidPlayRate;
			return AnimReference;
		}
	}

	return NULL;
}

/** Converts 'Track Position' to position on AnimSequence.
 * Note: doesn't check that position is in valid range, must do that before calling this function! */
float FAnimSegment::ConvertTrackPosToAnimPos(const float & TrackPosition) const
{
	const float AnimLength = (AnimEndTime - AnimStartTime);
	const float AnimPositionUnWrapped = (TrackPosition - StartPos) * GetValidPlayRate();

	// Figure out how many times animation is allowed to be looped.
	const float LoopCount = FMath::Min(FMath::Floor(AnimPositionUnWrapped / AnimLength), FMath::Max(LoopingCount-1, 0));
	// Position within AnimSequence
	const float AnimPosition = AnimStartTime + AnimPositionUnWrapped - float(LoopCount) * AnimLength;
	
	return AnimPosition;
}

void FAnimSegment::GetAnimNotifiesFromTrackPositions(const float & PreviousTrackPosition, const float & CurrentTrackPosition, TArray<const FAnimNotifyEvent *> & OutActiveNotifies) const
{
	if( PreviousTrackPosition == CurrentTrackPosition )
	{
		return;
	}

	const bool bPlayingBackwards = (PreviousTrackPosition > CurrentTrackPosition);
	const float SegmentStartPos = StartPos;
	const float SegmentEndPos = StartPos + GetLength();

	// if track range overlaps segment
	if( bPlayingBackwards 
		? ((CurrentTrackPosition < SegmentEndPos) && (PreviousTrackPosition > SegmentStartPos)) 
		: ((PreviousTrackPosition < SegmentEndPos) && (CurrentTrackPosition > SegmentStartPos)) 
		)
	{
		// Only allow AnimSequences for now. Other types will need additional support.
		UAnimSequence * AnimSequence = Cast<UAnimSequence>(AnimReference);
		if( AnimSequence )
		{
			const float ValidPlayRate = GetValidPlayRate();
			// Get starting position, closest overlap.
			float AnimStartPosition = ConvertTrackPosToAnimPos( bPlayingBackwards ? FMath::Min(PreviousTrackPosition, SegmentEndPos) : FMath::Max(PreviousTrackPosition, SegmentStartPos) );
			check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
			float TrackTimeToGo = CurrentTrackPosition - PreviousTrackPosition;

			// Abstract out end point since animation can be playing forward or backward.
			const float AnimEndPoint = bPlayingBackwards ? AnimStartTime : AnimEndTime;

			for(int32 IterationsLeft=FMath::Max(LoopingCount, 1); ((IterationsLeft > 0) && (TrackTimeToGo > 0.f)); --IterationsLeft)
			{
				// Track time left to reach end point of animation.
				const float TrackTimeToAnimEndPoint = (AnimEndPoint - AnimStartPosition) / ValidPlayRate;

				// If our time left is shorter than time to end point, no problem. End there.
				if( FMath::Abs(TrackTimeToGo) < FMath::Abs(TrackTimeToAnimEndPoint) )
				{
					const float AnimEndPosition = ConvertTrackPosToAnimPos(CurrentTrackPosition);
					// Make sure we have not wrapped around, positions should be contiguous.
					check( bPlayingBackwards ? (AnimEndPosition <= AnimStartPosition) : (AnimStartPosition <= AnimEndPosition) );
					AnimSequence->GetAnimNotifiesFromDeltaPositions(AnimStartPosition, AnimEndPosition, OutActiveNotifies);
					break;
				}
				// Otherwise we hit the end point of the animation first...
				else
				{
					// Add that piece for extraction.
					// Make sure we have not wrapped around, positions should be contiguous.
					check( bPlayingBackwards ? (AnimEndPoint <= AnimStartPosition) : (AnimStartPosition <= AnimEndPoint) );
					AnimSequence->GetAnimNotifiesFromDeltaPositions(AnimStartPosition, AnimEndPoint, OutActiveNotifies);

					// decrease our TrackTimeToGo if we have to do another iteration.
					// and put ourselves back at the beginning of the animation.
					TrackTimeToGo -= TrackTimeToAnimEndPoint;
					AnimStartPosition = bPlayingBackwards ? AnimEndTime : AnimStartTime;
				}
			}
		}
	}
}

/** 
 * Given a Track delta position [StartTrackPosition, EndTrackPosition]
 * See if this AnimSegment overlaps any of it, and if it does, break them up into a sequence of FRootMotionExtractionStep.
 * Supports animation playing forward and backward. Track range should be a contiguous range, not wrapping over due to looping.
 */
void FAnimSegment::GetRootMotionExtractionStepsForTrackRange(TArray<FRootMotionExtractionStep> & RootMotionExtractionSteps, const float StartTrackPosition, const float EndTrackPosition) const
{
	if( StartTrackPosition == EndTrackPosition )
	{
		return;
	}

	// Only allow AnimSequences for now. Other types will need additional support.
	UAnimSequence * AnimSequence = Cast<UAnimSequence>(AnimReference);
	if( AnimSequence )
	{
		const bool bPlayingBackwards = (StartTrackPosition > EndTrackPosition);

		const float SegmentStartPos = StartPos;
		const float SegmentEndPos = StartPos + GetLength();

		// if range overlaps segment
		if( bPlayingBackwards 
			? ((EndTrackPosition < SegmentEndPos) && (StartTrackPosition > SegmentStartPos)) 
			: ((StartTrackPosition < SegmentEndPos) && (EndTrackPosition > SegmentStartPos)) 
			)
		{
			const float ValidPlayRate = GetValidPlayRate();
			// Get starting position, closest overlap.
			float AnimStartPosition = ConvertTrackPosToAnimPos( bPlayingBackwards ? FMath::Min(StartTrackPosition, SegmentEndPos) : FMath::Max(StartTrackPosition, SegmentStartPos) );
			check( (AnimStartPosition >= AnimStartTime) && (AnimStartPosition <= AnimEndTime) );
			float TrackTimeToGo = EndTrackPosition - StartTrackPosition;

			// Abstract out end point since animation can be playing forward or backward.
			const float AnimEndPoint = bPlayingBackwards ? AnimStartTime : AnimEndTime;

			for(int32 IterationsLeft=FMath::Max(LoopingCount, 1); ((IterationsLeft > 0) && (TrackTimeToGo > 0.f)); --IterationsLeft)
			{
				// Track time left to reach end point of animation.
				const float TrackTimeToAnimEndPoint = (AnimEndPoint - AnimStartPosition) / ValidPlayRate;

				// If our time left is shorter than time to end point, no problem. End there.
				if( FMath::Abs(TrackTimeToGo) < FMath::Abs(TrackTimeToAnimEndPoint) )
				{
					const float AnimEndPosition = ConvertTrackPosToAnimPos(EndTrackPosition);
					// Make sure we have not wrapped around, positions should be contiguous.
					check( bPlayingBackwards ? (AnimEndPosition <= AnimStartPosition) : (AnimStartPosition <= AnimEndPosition) );
					RootMotionExtractionSteps.Add(FRootMotionExtractionStep(AnimSequence, AnimStartPosition, AnimEndPosition));
					break;
				}
				// Otherwise we hit the end point of the animation first...
				else
				{
					// Add that piece for extraction.
					// Make sure we have not wrapped around, positions should be contiguous.
					check( bPlayingBackwards ? (AnimEndPoint <= AnimStartPosition) : (AnimStartPosition <= AnimEndPoint) );
					RootMotionExtractionSteps.Add(FRootMotionExtractionStep(AnimSequence, AnimStartPosition, AnimEndPoint));

					// decrease our TrackTimeToGo if we have to do another iteration.
					// and put ourselves back at the beginning of the animation.
					TrackTimeToGo -= TrackTimeToAnimEndPoint;
					AnimStartPosition = bPlayingBackwards ? AnimEndTime : AnimStartTime;
				}
			}
		}
	}
}

/** 
 * Given a Track delta position [StartTrackPosition, EndTrackPosition]
 * See if any AnimSegment overlaps any of it, and if any do, break them up into a sequence of FRootMotionExtractionStep.
 * Supports animation playing forward and backward. Track range should be a contiguous range, not wrapping over due to looping.
 */
void FAnimTrack::GetRootMotionExtractionStepsForTrackRange(TArray<FRootMotionExtractionStep> & RootMotionExtractionSteps, const float StartTrackPosition, const float EndTrackPosition) const
{
	// must extract root motion in right order sequentially
	const bool bPlayingBackwards = (StartTrackPosition > EndTrackPosition);
	if( bPlayingBackwards )
	{
		for(int32 AnimSegmentIndex=AnimSegments.Num()-1; AnimSegmentIndex>=0; AnimSegmentIndex--)
		{
			const FAnimSegment & AnimSegment = AnimSegments[AnimSegmentIndex];
			AnimSegment.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, StartTrackPosition, EndTrackPosition);
		}
	}
	else
	{
		for(int32 AnimSegmentIndex=0; AnimSegmentIndex<AnimSegments.Num(); AnimSegmentIndex++)
		{
			const FAnimSegment & AnimSegment = AnimSegments[AnimSegmentIndex];
			AnimSegment.GetRootMotionExtractionStepsForTrackRange(RootMotionExtractionSteps, StartTrackPosition, EndTrackPosition);
		}
	}
}


///////////////////////////////////////////////////////
// FAnimTrack
///////////////////////////////////////////////////////
float FAnimTrack::GetLength() const
{
	float TotalLength = 0.f;

	// in the future, if we're more clear about exactly what requirement is for segments, 
	// this can be optimized. For now this is slow. 
	for ( int32 I=0; I<AnimSegments.Num(); ++I )
	{
		const struct FAnimSegment & Segment = AnimSegments[I];
		float EndFrame = Segment.StartPos + Segment.GetLength();
		if ( EndFrame > TotalLength )
		{
			TotalLength = EndFrame;
		}
	}

	return TotalLength;
}

bool FAnimTrack::IsAdditive() const
{
	// this function just checks first animation to verify if this is additive or not
	// if first one is additive, it returns true, 
	// the best way to handle isn't really practical. If I do go through all of them
	// and if they mismatch, what can I do? That should be another verification function when this is created
	// it will look visually wrong if something mismatches, but nothing really is better solution than that. 
	// in editor, when this is created, the test has to be done to verify all are matches. 
	for ( int32 I=0; I<AnimSegments.Num(); ++I )
	{
		const struct FAnimSegment & Segment = AnimSegments[I];
		return ( Segment.AnimReference && Segment.AnimReference->IsValidAdditive() );
	}

	return false;
}

bool FAnimTrack::IsRotationOffsetAdditive() const
{
	// this function just checks first animation to verify if this is additive or not
	// if first one is additive, it returns true, 
	// the best way to handle isn't really practical. If I do go through all of them
	// and if they mismatch, what can I do? That should be another verification function when this is created
	// it will look visually wrong if something mismatches, but nothing really is better solution than that. 
	// in editor, when this is created, the test has to be done to verify all are matches. 
	for ( int32 I=0; I<AnimSegments.Num(); ++I )
	{
		const struct FAnimSegment & Segment = AnimSegments[I];
		if ( Segment.AnimReference && Segment.AnimReference->IsValidAdditive() )
		{
			UAnimSequence * Sequence = Cast<UAnimSequence>(Segment.AnimReference);
			if ( Sequence )
			{
				return (Sequence->AdditiveAnimType == AAT_RotationOffsetMeshSpace);
			}
			else
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

	return false;
}

int32 FAnimTrack::GetTrackAdditiveType() const
{
	// this function just checks first animation to verify the type
	// the best way to handle isn't really practical. If I do go through all of them
	// and if they mismatch, what can I do? That should be another verification function when this is created
	// it will look visually wrong if something mismatches, but nothing really is better solution than that. 
	// in editor, when this is created, the test has to be done to verify all are matches. 

	if( AnimSegments.Num() > 0 )
	{
		const struct FAnimSegment & Segment = AnimSegments[0];
		UAnimSequence * Sequence = Cast<UAnimSequence>(Segment.AnimReference);
		if ( Sequence )
		{
			return Sequence->AdditiveAnimType;
		}
	}
	return -1;
}

#if WITH_EDITOR
bool FAnimTrack::GetAllAnimationSequencesReferred(TArray<UAnimSequence*>& AnimationSequences) const
{
	for ( int32 I=0; I<AnimSegments.Num(); ++I )
	{
		const struct FAnimSegment & Segment = AnimSegments[I];
		if ( Segment.AnimReference  )
		{
			UAnimSequence * Sequence = Cast<UAnimSequence>(Segment.AnimReference);
			if ( Sequence )
			{
				AnimationSequences.Add(Sequence);
			}
		}
	}

	return ( AnimationSequences.Num() > 0 );
}

void FAnimTrack::ReplaceReferredAnimations(const TMap<UAnimSequence*, UAnimSequence*>& ReplacementMap)
{
	TArray<FAnimSegment> NewAnimSegments;
	for ( int32 I=0; I<AnimSegments.Num(); ++I )
	{
		struct FAnimSegment & Segment = AnimSegments[I];
		UAnimSequence * Sequence = Cast<UAnimSequence>(Segment.AnimReference);

		if ( Sequence  )
		{
			UAnimSequence* const* ReplacementAsset = (UAnimSequence*const*)ReplacementMap.Find(Sequence);
			if(ReplacementAsset)
			{
				Segment.AnimReference = *ReplacementAsset;
				NewAnimSegments.Add(Segment);
			}
		}
	}
	AnimSegments = NewAnimSegments;
}

void FAnimTrack::CollapseAnimSegments()
{
	if(AnimSegments.Num() > 0)
	{
		// Sort function
		struct FSortFloatInt
		{
			bool operator()( const TKeyValuePair<float, int32> &A, const TKeyValuePair<float, int32>&B ) const
			{
				return A.Key < B.Key;
			}
		};

		// Create sorted map of start time to segment
		TArray<TKeyValuePair<float, int32>> m;
		for( int32 SegmentInd=0; SegmentInd < AnimSegments.Num(); ++SegmentInd )
		{
			m.Add(TKeyValuePair<float, int32>(AnimSegments[SegmentInd].StartPos, SegmentInd));
		}
		m.Sort(FSortFloatInt());

		//collapse all start times based on sorted map
		FAnimSegment* PrevSegment = &AnimSegments[m[0].Value];
		PrevSegment->StartPos = 0.0f;

		for ( int32 SegmentInd=1; SegmentInd < m.Num(); ++SegmentInd )
		{
			FAnimSegment* CurrSegment = &AnimSegments[m[SegmentInd].Value];
			CurrSegment->StartPos = PrevSegment->StartPos + PrevSegment->GetLength();
			PrevSegment = CurrSegment;
		}
	}
}

void FAnimTrack::SortAnimSegments()
{
	if(AnimSegments.Num() > 0)
	{
		struct FCompareSegments
		{
			bool operator()( const FAnimSegment &A, const FAnimSegment&B ) const
			{
				return A.StartPos < B.StartPos;
			}
		};

		AnimSegments.Sort( FCompareSegments() );

		// rearrange, make sure there are no gaps between and all start times are correctly set
		AnimSegments[0].StartPos = 0.0f;

		for ( int32 J=1; J < AnimSegments.Num(); J++ )
		{
			AnimSegments[J].StartPos = AnimSegments[J-1].StartPos + AnimSegments[J-1].GetLength();
		}
	}
}
#endif

///////////////////////////////////////////////////////
// UAnimCompositeBase
///////////////////////////////////////////////////////

UAnimCompositeBase::UAnimCompositeBase(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

#if WITH_EDITOR
void UAnimCompositeBase::SetSequenceLength(float InSequenceLength)
{
	SequenceLength = InSequenceLength;
}
#endif

/*
void UAnimCompositeBase::AddAnimSegment( FAnimTrack & Track, FAnimSegment& NewSegment )
{

}

bool UAnimCompositeBase::DeleteAnimSegment( FAnimTrack & Track, FAnimSegment& SegmentToDelete )
{

}
*/

