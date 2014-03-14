// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "AnimationUtils.h"
#include "AnimationRuntime.h"

#define NOTIFY_TRIGGER_OFFSET KINDA_SMALL_NUMBER;

float GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::Type OffsetType)
{
	switch (OffsetType)
	{
	case EAnimEventTriggerOffsets::OffsetBefore:
		{
			return -NOTIFY_TRIGGER_OFFSET;
			break;
		}
	case EAnimEventTriggerOffsets::OffsetAfter:
		{
			return NOTIFY_TRIGGER_OFFSET;
			break;
		}
	case EAnimEventTriggerOffsets::NoOffset:
		{
			return 0.f;
			break;
		}
	default:
		{
			check(false); // Unknown value supplied for OffsetType
			break;
		}
	}
	return 0.f;
}

/////////////////////////////////////////////////////
// FAnimNotifyEvent

void FAnimNotifyEvent::RefreshTriggerOffset(EAnimEventTriggerOffsets::Type PredictedOffsetType)
{
	if(PredictedOffsetType == EAnimEventTriggerOffsets::NoOffset || TriggerTimeOffset == 0.f)
	{
		TriggerTimeOffset = GetTriggerTimeOffsetForType(PredictedOffsetType);
	}
}

void FAnimNotifyEvent::RefreshEndTriggerOffset( EAnimEventTriggerOffsets::Type PredictedOffsetType )
{
	if(PredictedOffsetType == EAnimEventTriggerOffsets::NoOffset || EndTriggerTimeOffset == 0.f)
	{
		EndTriggerTimeOffset = GetTriggerTimeOffsetForType(PredictedOffsetType);
	}
}

float FAnimNotifyEvent::GetTriggerTime() const
{
	return DisplayTime + TriggerTimeOffset;
}

float FAnimNotifyEvent::GetEndTriggerTime() const
{
	return GetTriggerTime() + Duration + EndTriggerTimeOffset;
}

/////////////////////////////////////////////////////
// FFloatCurve

void FFloatCurve::SetCurveTypeFlag(EAnimCurveFlags InFlag, bool bValue)
{
	if (bValue)
	{
		CurveTypeFlags |= InFlag;
	}
	else
	{
		CurveTypeFlags &= ~InFlag;
	}
}

bool FFloatCurve::GetCurveTypeFlag(EAnimCurveFlags InFlag) const
{
	return (CurveTypeFlags & InFlag) != 0;
}


void FFloatCurve::SetCurveTypeFlags(int32 NewCurveTypeFlags)
{
	CurveTypeFlags = NewCurveTypeFlags;
}

int32 FFloatCurve::GetCurveTypeFlags() const
{
	return CurveTypeFlags;
}

/////////////////////////////////////////////////////
// FRawCurveTracks

void FRawCurveTracks::EvaluateCurveData(class UAnimInstance* Instance, float CurrentTime, float BlendWeight ) const
{
	// evaluate the curve data at the CurrentTime and add to Instance
	for (auto CurveIter = FloatCurves.CreateConstIterator(); CurveIter; ++CurveIter)
	{
		const FFloatCurve & Curve = *CurveIter;
		Instance->AddCurveValue( Curve.CurveName, Curve.FloatCurve.Eval(CurrentTime)*BlendWeight, Curve.GetCurveTypeFlags() );
	}
}

FFloatCurve * FRawCurveTracks::GetCurveData(FName InCurveName) 
{
	for (auto CurveIter = FloatCurves.CreateIterator(); CurveIter; ++CurveIter)
	{
		FFloatCurve & Curve = *CurveIter;
		if (Curve.CurveName == InCurveName)
		{
			return &Curve;
		}
	}

	return NULL;
}

bool FRawCurveTracks::DeleteCurveData(FName InCurveName)
{
	for ( int32 Id=0; Id<FloatCurves.Num(); ++Id )
	{
		if (FloatCurves[Id].CurveName == InCurveName)
		{
			FloatCurves.RemoveAt(Id);
			return true;
		}
	}

	return false;
}

bool FRawCurveTracks::AddCurveData(FName InCurveName, int32 CurveFlags /*= ACF_DefaultCurve*/)
{
	// if we don't have same name used
	if ( GetCurveData(InCurveName) == NULL )
	{
		FloatCurves.Add( FFloatCurve(InCurveName, CurveFlags) );
		return true;
	}

	return false;
}

/////////////////////////////////////////////////////

UAnimSequenceBase::UAnimSequenceBase(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
	, RateScale(1.0f)
{
}

void UAnimSequenceBase::PostLoad()
{
	Super::PostLoad();

	// Convert Notifies to new data
	if( GIsEditor && GetLinkerUE4Version() < VER_UE4_ANIMNOTIFY_NAMECHANGE && Notifies.Num() > 0 )
	{
		LOG_SCOPE_VERBOSITY_OVERRIDE(LogAnimation, ELogVerbosity::Warning);
 		// convert animnotifies
 		for (int32 I=0; I<Notifies.Num(); ++I)
 		{
 			if (Notifies[I].Notify!=NULL)
 			{
				FString Label = Notifies[I].Notify->GetClass()->GetName();
				Label = Label.Replace(TEXT("AnimNotify_"), TEXT(""), ESearchCase::CaseSensitive);
				Notifies[I].NotifyName = FName(*Label);
 			}
 		}
	}

	if ( GetLinkerUE4Version() < VER_UE4_MORPHTARGET_CURVE_INTEGRATION )
	{
		UpgradeMorphTargetCurves();
	}
	// Ensure notifies are sorted.
	SortNotifies();

#if WITH_EDITORONLY_DATA
	InitializeNotifyTrack();
	UpdateAnimNotifyTrackCache();
#endif
}

void UAnimSequenceBase::UpgradeMorphTargetCurves()
{
	// make sure this doesn't get called by anywhere else or you'll have to check this before calling 
	if ( GetLinkerUE4Version() < VER_UE4_MORPHTARGET_CURVE_INTEGRATION )
	{
		for ( auto CurveIter = RawCurveData.FloatCurves.CreateIterator(); CurveIter; ++CurveIter )
		{
			FFloatCurve & Curve = (*CurveIter);
			// make sure previous curves has editable flag
			Curve.SetCurveTypeFlag(ACF_DefaultCurve, true);
		}	
	}
}

void UAnimSequenceBase::SortNotifies()
{
	struct FCompareFAnimNotifyEvent
	{
		FORCEINLINE bool operator()( const FAnimNotifyEvent& A, const FAnimNotifyEvent& B ) const
		{
			return A.GetTriggerTime() < B.GetTriggerTime();
		}
	};
	Notifies.Sort( FCompareFAnimNotifyEvent() );
}

/** 
 * Retrieves AnimNotifies given a StartTime and a DeltaTime.
 * Time will be advanced and support looping if bAllowLooping is true.
 * Supports playing backwards (DeltaTime<0).
 * Returns notifies between StartTime (exclusive) and StartTime+DeltaTime (inclusive)
 */
void UAnimSequenceBase::GetAnimNotifies(const float & StartTime, const float & DeltaTime, const float bAllowLooping, TArray<const FAnimNotifyEvent *> & OutActiveNotifies) const
{
	// Early out if we have no notifies
	if( (Notifies.Num() == 0) || (DeltaTime == 0.f) )
	{
		return;
	}

	bool const bPlayingBackwards = (DeltaTime < 0.f);
	float PreviousPosition = StartTime;
	float CurrentPosition = StartTime;
	float DesiredDeltaMove = DeltaTime;

	do 
	{
		// Disable looping here. Advance to desired position, or beginning / end of animation 
		const ETypeAdvanceAnim AdvanceType = FAnimationRuntime::AdvanceTime(false, DesiredDeltaMove, CurrentPosition, SequenceLength);

		// Verify position assumptions
		check( bPlayingBackwards ? (CurrentPosition <= PreviousPosition) : (CurrentPosition >= PreviousPosition));
		
		GetAnimNotifiesFromDeltaPositions(PreviousPosition, CurrentPosition, OutActiveNotifies);
	
		// If we've hit the end of the animation, and we're allowed to loop, keep going.
		if( (AdvanceType == ETAA_Finished) &&  bAllowLooping )
		{
			const float ActualDeltaMove = (CurrentPosition - PreviousPosition);
			DesiredDeltaMove -= ActualDeltaMove; 

			PreviousPosition = bPlayingBackwards ? SequenceLength : 0.f;
			CurrentPosition = PreviousPosition;
		}
		else
		{
			break;
		}
	} 
	while( true );
}

/** 
 * Retrieves AnimNotifies between two time positions. ]PreviousPosition, CurrentPosition]
 * Between PreviousPosition (exclusive) and CurrentPosition (inclusive).
 * Supports playing backwards (CurrentPosition<PreviousPosition).
 * Only supports contiguous range, does NOT support looping and wrapping over.
 */
void UAnimSequenceBase::GetAnimNotifiesFromDeltaPositions(const float & PreviousPosition, const float & CurrentPosition, TArray<const FAnimNotifyEvent *> & OutActiveNotifies) const
{
	// Early out if we have no notifies
	if( (Notifies.Num() == 0) || (PreviousPosition == CurrentPosition) )
	{
		return;
	}

	bool const bPlayingBackwards = (CurrentPosition < PreviousPosition);

	// If playing backwards, flip Min and Max.
	if( bPlayingBackwards )
	{
		for (int32 NotifyIndex=0; NotifyIndex<Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent & AnimNotifyEvent = Notifies[NotifyIndex];
			const float NotifyStartTime = AnimNotifyEvent.GetTriggerTime();
			const float NotifyEndTime = AnimNotifyEvent.GetEndTriggerTime();

			if( (NotifyStartTime < PreviousPosition) && (NotifyEndTime >= CurrentPosition) )
			{
				OutActiveNotifies.Add(&AnimNotifyEvent);
			}
		}
	}
	else
	{
		for (int32 NotifyIndex=0; NotifyIndex<Notifies.Num(); NotifyIndex++)
		{
			const FAnimNotifyEvent & AnimNotifyEvent = Notifies[NotifyIndex];
			const float NotifyStartTime = AnimNotifyEvent.GetTriggerTime();
			const float NotifyEndTime = AnimNotifyEvent.GetEndTriggerTime();

			if( (NotifyStartTime <= CurrentPosition) && (NotifyEndTime > PreviousPosition) )
			{
				OutActiveNotifies.Add(&AnimNotifyEvent);
			}
		}
	}
}

void UAnimSequenceBase::TickAssetPlayerInstance(const FAnimTickRecord& Instance, class UAnimInstance* InstanceOwner, FAnimAssetTickContext& Context) const
{
	float& CurrentTime = *(Instance.TimeAccumulator);
	const float PreviousTime = CurrentTime;

	const float PlayRate = Instance.PlayRateMultiplier * this->RateScale;
	if( Context.IsLeader() )
	{
		const float DeltaTime = Context.GetDeltaTime();
		const float MoveDelta = PlayRate * DeltaTime;
		if( MoveDelta != 0.f )
		{
			// Advance time
			FAnimationRuntime::AdvanceTime(Instance.bLooping, MoveDelta, CurrentTime, SequenceLength);

			if( Context.ShouldGenerateNotifies() )
			{
				// Harvest and record notifies
				TArray<const FAnimNotifyEvent*> AnimNotifies;
				GetAnimNotifies(PreviousTime, MoveDelta, Instance.bLooping, AnimNotifies);
				InstanceOwner->AddAnimNotifies(AnimNotifies, Instance.EffectiveBlendWeight);
			}
		}

		Context.SetSyncPoint(CurrentTime / SequenceLength);
	}
	else
	{
		// Follow the leader
		CurrentTime = Context.GetSyncPoint() * SequenceLength;
		//@TODO: NOTIFIES: Calculate AdvanceType based on what the new delta time is

		if( Context.ShouldGenerateNotifies() && (CurrentTime != PreviousTime) )
		{
			// Figure out delta time 
			float MoveDelta = CurrentTime - PreviousTime;
			// if we went against play rate, then loop around.
			if( (MoveDelta * PlayRate) < 0.f )
			{
				MoveDelta += FMath::Sign<float>(PlayRate) * SequenceLength;
			}

			// Harvest and record notifies
			TArray<const FAnimNotifyEvent*> AnimNotifies;
			GetAnimNotifies(PreviousTime, MoveDelta, Instance.bLooping, AnimNotifies);
			InstanceOwner->AddAnimNotifies(AnimNotifies, Instance.EffectiveBlendWeight);
		}
	}

	// Evaluate Curve data now - even if time did not move, we still need to return curve if it exists
	EvaluateCurveData(InstanceOwner, CurrentTime, Instance.EffectiveBlendWeight);
}

#if WITH_EDITOR

void UAnimSequenceBase::UpdateAnimNotifyTrackCache()
{
	for (int32 TrackIndex=0; TrackIndex<AnimNotifyTracks.Num(); ++TrackIndex)
	{
		AnimNotifyTracks[TrackIndex].Notifies.Empty();
	}

	for (int32 NotifyIndex = 0; NotifyIndex<Notifies.Num(); ++NotifyIndex)
	{
		int32 TrackIndex = Notifies[NotifyIndex].TrackIndex;
		if (AnimNotifyTracks.IsValidIndex(TrackIndex))
		{
			AnimNotifyTracks[TrackIndex].Notifies.Add(&Notifies[NotifyIndex]);
		}
		else
		{
			// this notifyindex isn't valid, delete
			// this should not happen, but if it doesn, find best place to add
			ensureMsg(0, TEXT("AnimNotifyTrack: Wrong indices found"));
			AnimNotifyTracks[0].Notifies.Add(&Notifies[NotifyIndex]);
		}
	}

	// notification broadcast
	OnNotifyChanged.Broadcast();
}

void UAnimSequenceBase::InitializeNotifyTrack()
{
	if ( AnimNotifyTracks.Num() == 0 ) 
	{
		AnimNotifyTracks.Add(FAnimNotifyTrack(TEXT("1"), FLinearColor::White ));
	}
}

int32 UAnimSequenceBase::GetNumberOfFrames()
{
	return (SequenceLength/0.033f);
}

void UAnimSequenceBase::RegisterOnNotifyChanged(const FOnNotifyChanged& Delegate)
{
	OnNotifyChanged.Add(Delegate);
}
void UAnimSequenceBase::UnregisterOnNotifyChanged(void * Unregister)
{
	OnNotifyChanged.RemoveAll(Unregister);
}

void UAnimSequenceBase::ClampNotifiesAtEndOfSequence()
{
	const float NotifyClampTime = SequenceLength - 0.01f; //Slight offset so that notify is still draggable
	for(int i = 0; i < Notifies.Num(); ++ i)
	{
		if(Notifies[i].DisplayTime >= SequenceLength)
		{
			Notifies[i].DisplayTime = NotifyClampTime;
			Notifies[i].TriggerTimeOffset = GetTriggerTimeOffsetForType(EAnimEventTriggerOffsets::OffsetBefore);
		}
	}
}

EAnimEventTriggerOffsets::Type UAnimSequenceBase::CalculateOffsetForNotify(float NotifyDisplayTime) const
{
	if(NotifyDisplayTime == 0.f)
	{
		return EAnimEventTriggerOffsets::OffsetAfter;
	}
	else if(NotifyDisplayTime == SequenceLength)
	{
		return EAnimEventTriggerOffsets::OffsetBefore;
	}
	return EAnimEventTriggerOffsets::NoOffset;
}

void UAnimSequenceBase::GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const
{
	Super::GetAssetRegistryTags(OutTags);
	if(Notifies.Num() > 0)
	{
		FString NotifyList;

		// add notifies to 
		for(auto Iter=Notifies.CreateConstIterator(); Iter; ++Iter)
		{
			// only add if not BP anim notify since they're handled separate
			if(Iter->IsBlueprintNotify() == false)
			{
				NotifyList += FString::Printf(TEXT("%s%c"), *Iter->NotifyName.ToString(), USkeleton::AnimNotifyTagDeliminator);
			}
		}
		
		if(NotifyList.Len() > 0)
		{
			OutTags.Add(FAssetRegistryTag(USkeleton::AnimNotifyTag, NotifyList, FAssetRegistryTag::TT_Hidden));
		}
	}
}

#endif	//WITH_EDITOR


/** Add curve data to Instance at the time of CurrentTime **/
void UAnimSequenceBase::EvaluateCurveData(class UAnimInstance* Instance, float CurrentTime, float BlendWeight ) const
{
	// if we have compression, this should change
	// this is raw data evaluation
	RawCurveData.EvaluateCurveData(Instance, CurrentTime, BlendWeight);
}

void UAnimSequenceBase::ExtractRootTrack(float Pos, FTransform & RootTransform, const FBoneContainer * RequiredBones) const
{
	// Fallback to root bone from reference skeleton.
	if (RequiredBones)
	{
		const FReferenceSkeleton & RefSkeleton = RequiredBones->GetReferenceSkeleton();
		if (RefSkeleton.GetNum() > 0)
		{
			RootTransform = RefSkeleton.GetRefBonePose()[0];
			return;
		}
	}

	USkeleton * MySkeleton = GetSkeleton();
	// If we don't have a RequiredBones array, get root bone from default skeleton.
	if (!RequiredBones && MySkeleton)
	{
		const FReferenceSkeleton RefSkeleton = MySkeleton->GetReferenceSkeleton();
		if (RefSkeleton.GetNum() > 0)
		{
			RootTransform = RefSkeleton.GetRefBonePose()[0];
			return;
		}
	}

	// Otherwise, use identity.
	RootTransform = FTransform::Identity;
};

