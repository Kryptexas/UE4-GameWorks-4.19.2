// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"

//////////////////////////////////////////////////////////////////////////
// UCameraAnimInst

UCameraAnimInst::UCameraAnimInst(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	bFinished = true;
	bAutoReleaseWhenFinished = true;
	PlayRate = 1.0f;
	TransientScaleModifier = 1.0f;
	PlaySpace = CAPS_CameraLocal;

	InterpGroupInst = PCIP.CreateDefaultSubobject<UInterpGroupInst>(this, TEXT("InterpGroupInst0"));
}

void UCameraAnimInst::AdvanceAnim(float DeltaTime, bool bJump)
{
	// check to see if our animnodeseq has been deleted.  not a fan of 
	// polling for this, but we want to stop this immediately, not when
	// GC gets around to cleaning up.
	if ( (CamAnim != NULL) && !bFinished )
	{
		// will set to true if anim finishes this frame
		bool bAnimJustFinished = false;

		float const ScaledDeltaTime = DeltaTime * PlayRate;

		// find new times
		CurTime += ScaledDeltaTime;
		if (bBlendingIn)
		{
			CurBlendInTime += DeltaTime;
		}
		if (bBlendingOut)
		{
			CurBlendOutTime += DeltaTime;
		}

		// see if we've crossed any important time thresholds and deal appropriately
		if (bLooping)
		{
			if (CurTime > CamAnim->AnimLength)
			{
				// loop back to the beginning
				CurTime -= CamAnim->AnimLength;
			}
		}
		else
		{
			if (CurTime > CamAnim->AnimLength)
			{
				// done!!
				bAnimJustFinished = true;
			}
			else if (CurTime > (CamAnim->AnimLength - BlendOutTime))
			{
				// start blending out
				bBlendingOut = true;
				CurBlendOutTime = CurTime - (CamAnim->AnimLength - BlendOutTime);
			}
		}

		if (bBlendingIn)
		{
			if (CurBlendInTime > BlendInTime)
			{
				// done blending in!
				bBlendingIn = false;
			}
		}
		if (bBlendingOut)
		{
			if (CurBlendOutTime > BlendOutTime)
			{
				// done!!
				CurBlendOutTime = BlendOutTime;
				bAnimJustFinished = true;
			}
		}
		
		// calculate blend weight. calculating separately and taking the minimum handles overlapping blends nicely.
		{
			float BlendInWeight = (bBlendingIn) ? (CurBlendInTime / BlendInTime) : 1.f;
			float BlendOutWeight = (bBlendingOut) ? (1.f - CurBlendOutTime / BlendOutTime) : 1.f;
			CurrentBlendWeight = FMath::Min(BlendInWeight, BlendOutWeight) * BasePlayScale * TransientScaleModifier;
		}

		// this will update tracks and apply the effects to the group actor (except move tracks)
		InterpGroupInst->Group->UpdateGroup(CurTime, InterpGroupInst, false, bJump);

		if (bAnimJustFinished)
		{
			// completely finished
			Stop(true);
		}
		else if (RemainingTime > 0.f)
		{
			// handle any specified duration
			RemainingTime -= DeltaTime;
			if (RemainingTime <= 0.f)
			{
				// stop with blend out
				Stop();
			}
		}
	}
}

void UCameraAnimInst::Update(float NewRate, float NewScale, float NewBlendInTime, float NewBlendOutTime, float NewDuration)
{
	if (bBlendingOut)
	{
		bBlendingOut = false;
		CurBlendOutTime = 0.f;

		// stop any blendout and reverse it to a blendin
		bBlendingIn = true;
		CurBlendInTime = NewBlendInTime * (1.f - CurBlendOutTime / BlendOutTime);
	}

	PlayRate = NewRate;
	BasePlayScale = NewScale;
	BlendInTime = NewBlendInTime;
	BlendOutTime = NewBlendOutTime;
	RemainingTime = (NewDuration > 0.f) ? (NewDuration - BlendOutTime) : 0.f;
	bFinished = false;
}


void UCameraAnimInst::Play(class UCameraAnim* Anim, class AActor* CamActor, float InRate, float InScale, float InBlendInTime, float InBlendOutTime, bool bInLooping, bool bRandomStartTime, float Duration)
{
	if (Anim && Anim->CameraInterpGroup)
	{
		// make sure any previous anim has been terminated correctly
		Stop(true);

		CurTime = bRandomStartTime ? (FMath::FRand() * Anim->AnimLength) : 0.f;
		CurBlendInTime = 0.f;
		CurBlendOutTime = 0.f;
		bBlendingIn = true;
		bBlendingOut = false;
		bFinished = false;

		// copy properties
		CamAnim = Anim;
		PlayRate = InRate;
		BasePlayScale = InScale;
		BlendInTime = InBlendInTime;
		BlendOutTime = InBlendOutTime;
		bLooping = bInLooping;
		RemainingTime = (Duration > 0.f) ? (Duration - BlendOutTime) : 0.f;

		// init the interpgroup

		if ( CamActor->IsA(ACameraActor::StaticClass()) )
		{
			// ensure CameraActor is zeroed, so RelativeToInitial anims get proper InitialTM
			CamActor->SetActorLocation(FVector::ZeroVector, false);
			CamActor->SetActorRotation(FRotator::ZeroRotator);
		}
		InterpGroupInst->InitGroupInst(CamAnim->CameraInterpGroup, CamActor);

		// cache move track refs
		for (int32 Idx = 0; Idx < InterpGroupInst->TrackInst.Num(); ++Idx)
		{
			MoveTrack = Cast<UInterpTrackMove>(CamAnim->CameraInterpGroup->InterpTracks[Idx]);
			if (MoveTrack != NULL)
			{
				MoveInst = CastChecked<UInterpTrackInstMove>(InterpGroupInst->TrackInst[Idx]);
				// only 1 move track per group, so we can bail here
				break;					
			}
		}	
	}
}

void UCameraAnimInst::Stop(bool bImmediate)
{
	if ( bImmediate || (BlendOutTime <= 0.f) )
	{
		if (InterpGroupInst->Group != NULL)
		{
			InterpGroupInst->TermGroupInst(true);
		}
		MoveTrack = NULL;
		MoveInst = NULL;
		bFinished = true;
 	}
	else
	{
		// start blend out
		bBlendingOut = true;
		CurBlendOutTime = 0.f;
	}
}

void UCameraAnimInst::ApplyTransientScaling(float Scalar)
{
	TransientScaleModifier *= Scalar;
}

void UCameraAnimInst::SetPlaySpace(ECameraAnimPlaySpace NewSpace, FRotator UserPlaySpace)
{
	PlaySpace = NewSpace;
	UserPlaySpaceMatrix = (PlaySpace == CAPS_UserDefined) ? FRotationMatrix(UserPlaySpace) : FMatrix::Identity;
}

