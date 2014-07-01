// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperFlipbookSceneProxy.h"
#include "PaperFlipbookComponent.h"

//////////////////////////////////////////////////////////////////////////
// UPaperFlipbookComponent

UPaperFlipbookComponent::UPaperFlipbookComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial"));
	Material = DefaultMaterial.Object;

	SpriteColor = FLinearColor::White;

	Mobility = EComponentMobility::Movable;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	bTickInEditor = true;
	
	CachedFrameIndex = INDEX_NONE;
	AccumulatedTime = 0.0f;
	PlayRate = 1.0f;

	bLooping = true;
	bReversePlayback = false;
	bPlaying = true;
}

UPaperSprite* UPaperFlipbookComponent::GetSpriteAtCachedIndex() const
{
	UPaperSprite* SpriteToSend = nullptr;
	if ((SourceFlipbook != nullptr) && SourceFlipbook->IsValidKeyFrameIndex(CachedFrameIndex))
	{
		SpriteToSend = SourceFlipbook->GetKeyFrameChecked(CachedFrameIndex).Sprite;
	}
	return SpriteToSend;
}

FPrimitiveSceneProxy* UPaperFlipbookComponent::CreateSceneProxy()
{
	FPaperFlipbookSceneProxy* NewProxy = new FPaperFlipbookSceneProxy(this);

	CalculateCurrentFrame();
	UPaperSprite* SpriteToSend = GetSpriteAtCachedIndex();

	FSpriteDrawCallRecord DrawCall;
	DrawCall.BuildFromSprite(SpriteToSend);
	DrawCall.Color = SpriteColor;
	NewProxy->SetDrawCall_RenderThread(DrawCall);
	return NewProxy;
}

FBoxSphereBounds UPaperFlipbookComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	if (SourceFlipbook != nullptr)
	{
		// Graphics bounds.
		FBoxSphereBounds NewBounds = SourceFlipbook->GetRenderBounds().TransformBy(LocalToWorld);

		//@TODO: PAPER2D: Add collision support to flipbooks
#if 0
		// Add bounds of collision geometry (if present).
		if (UBodySetup* BodySetup = SourceFlipbook->BodySetup)
		{
			const FBox AggGeomBox = BodySetup->AggGeom.CalcAABB(LocalToWorld);
			if (AggGeomBox.IsValid)
			{
				NewBounds = Union(NewBounds, FBoxSphereBounds(AggGeomBox));
			}
		}
#endif

		// Apply bounds scale
		NewBounds.BoxExtent *= BoundsScale;
		NewBounds.SphereRadius *= BoundsScale;

		return NewBounds;
	}
	else
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
	}
}

void UPaperFlipbookComponent::CalculateCurrentFrame()
{
	const int32 LastCachedFrame = CachedFrameIndex;
	CachedFrameIndex = (SourceFlipbook != nullptr) ? SourceFlipbook->GetKeyFrameIndexAtTime(AccumulatedTime) : INDEX_NONE;

	if (CachedFrameIndex != LastCachedFrame)
	{
		// Indicate we need to send new dynamic data.
		MarkRenderDynamicDataDirty();
	}
}

void UPaperFlipbookComponent::TickFlipbook(float DeltaTime)
{
	bool bIsFinished = false;

	if (bPlaying)
	{
		float NewPosition = AccumulatedTime;
		const float TimelineLength = GetFlipbookLength();

		if (!bReversePlayback)
		{
			NewPosition = AccumulatedTime + (DeltaTime * PlayRate);

			if (NewPosition > TimelineLength)
			{
				if (bLooping)
				{
					// If looping, play to end, jump to start, and set target to somewhere near the beginning.
					SetPlaybackPosition(TimelineLength, true);
					SetPlaybackPosition(0.0f, false);

					if (TimelineLength > 0.0f)
					{
						while (NewPosition > TimelineLength)
						{
							NewPosition -= TimelineLength;
						}
					}
					else
					{
						NewPosition = 0.0f;
					}
				}
				else
				{
					// If not looping, snap to end and stop playing.
					NewPosition = TimelineLength;
					Stop();
					bIsFinished = true;
				}
			}
		}
		else
		{
			NewPosition = AccumulatedTime - (DeltaTime * PlayRate);

			if (NewPosition < 0.0f)
			{
				if (bLooping)
				{
					// If looping, play to start, jump to end, and set target to somewhere near the end.
					SetPlaybackPosition(0.0f, true);
					SetPlaybackPosition(TimelineLength, false);

					if (TimelineLength > 0.0f)
					{
						while (NewPosition < 0.0f)
						{
							NewPosition += TimelineLength;
						}
					}
					else
					{
						NewPosition = 0.0f;
					}
				}
				else
				{
					// If not looping, snap to start and stop playing.
					NewPosition = 0.0f;
					Stop();
					bIsFinished = true;
				}
			}
		}

		SetPlaybackPosition(NewPosition, true);
	}

#if 0
	// Notify user that timeline finished
	if (bIsFinished)
	{
		TimelineFinishedFunc.ExecuteIfBound();
		TimelineFinishFuncStatic.ExecuteIfBound();
	}
#endif
}

void UPaperFlipbookComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Advance time
	TickFlipbook(DeltaTime);

	// Update the frame and push it to the renderer if necessary
	CalculateCurrentFrame();
}

void UPaperFlipbookComponent::SendRenderDynamicData_Concurrent()
{
	if (SceneProxy != NULL)
	{
		UPaperSprite* SpriteToSend = GetSpriteAtCachedIndex();

		FSpriteDrawCallRecord DrawCall;
		DrawCall.BuildFromSprite(SpriteToSend);
		DrawCall.Color = SpriteColor;

		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FSendPaperRenderComponentDynamicData,
				FPaperRenderSceneProxy*,InSceneProxy,(FPaperRenderSceneProxy*)SceneProxy,
				FSpriteDrawCallRecord,InSpriteToSend,DrawCall,
			{
				InSceneProxy->SetDrawCall_RenderThread(InSpriteToSend);
			});
	}
}

bool UPaperFlipbookComponent::SetFlipbook(class UPaperFlipbook* NewFlipbook)
{
	if (NewFlipbook != SourceFlipbook)
	{
		// Don't allow changing the sprite if we are "static".
		AActor* Owner = GetOwner();
		if (!IsRegistered() || (Owner == NULL) || (Mobility != EComponentMobility::Static))
		{
			SourceFlipbook = NewFlipbook;

			// We need to also reset the frame and time also
			AccumulatedTime = 0.0f;
			CalculateCurrentFrame();

			// Need to send this to render thread at some point
			MarkRenderStateDirty();

			// Update physics representation right away
			RecreatePhysicsState();

			// Since we have new mesh, we need to update bounds
			UpdateBounds();

			return true;
		}
	}

	return false;
}

UPaperFlipbook* UPaperFlipbookComponent::GetFlipbook()
{
	return SourceFlipbook;
}

UMaterialInterface* UPaperFlipbookComponent::GetSpriteMaterial() const
{
	return Material;
}

void UPaperFlipbookComponent::SetSpriteColor(FLinearColor NewColor)
{
	// Can't set color on a static component
	if (!(IsRegistered() && (Mobility == EComponentMobility::Static)) && (SpriteColor != NewColor))
	{
		SpriteColor = NewColor;

		//@TODO: Should we send immediately?
		MarkRenderDynamicDataDirty();
	}
}

const UObject* UPaperFlipbookComponent::AdditionalStatObject() const
{
	return SourceFlipbook;
}


void UPaperFlipbookComponent::Play()
{
	Activate();
	bReversePlayback = false;
	bPlaying = true;
}

void UPaperFlipbookComponent::PlayFromStart()
{
	SetPlaybackPosition(0.0f, /*bFireEvents=*/ false);
	Play();
}

void UPaperFlipbookComponent::Reverse()
{
	Activate();
	bReversePlayback = true;
	bPlaying = true;
}

void UPaperFlipbookComponent::ReverseFromEnd()
{
	SetPlaybackPosition(GetFlipbookLength(), /*bFireEvents=*/ false);
	Reverse();
}

void UPaperFlipbookComponent::Stop()
{
	bPlaying = false;
}

bool UPaperFlipbookComponent::IsPlaying() const
{
	return bPlaying;
}

bool UPaperFlipbookComponent::IsReversing() const
{
	return bPlaying && bReversePlayback;
}

void UPaperFlipbookComponent::SetPlaybackPosition(float NewPosition, bool bFireEvents)
{
	float OldPosition = AccumulatedTime;
	AccumulatedTime = NewPosition;

	// If we should be firing events for this track...
	if (bFireEvents)
	{
		float MinTime;
		float MaxTime;
		if (!bReversePlayback)
		{
			// If playing sequence forwards.
			MinTime = OldPosition;
			MaxTime = AccumulatedTime;

			// Slight hack here.. if playing forwards and reaching the end of the sequence, force it over a little to ensure we fire events actually on the end of the sequence.
			if (MaxTime == GetFlipbookLength())
			{
				MaxTime += (float)KINDA_SMALL_NUMBER;
			}
		}
		else
		{
			// If playing sequence backwards.
			MinTime = AccumulatedTime;
			MaxTime = OldPosition;

			// Same small hack as above for backwards case.
			if (MinTime == 0.0f)
			{
				MinTime -= (float)KINDA_SMALL_NUMBER;
			}
		}

#if 0
		// See which events fall into traversed region.
		for (int32 i = 0; i < Events.Num(); i++)
		{
			float EventTime = Events[i].Time;

			// Need to be slightly careful here and make behavior for firing events symmetric when playing forwards of backwards.
			bool bFireThisEvent = false;
			if (!bReversePlayback)
			{
				if (EventTime >= MinTime && EventTime < MaxTime)
				{
					bFireThisEvent = true;
				}
			}
			else
			{
				if (EventTime > MinTime && EventTime <= MaxTime)
				{
					bFireThisEvent = true;
				}
			}

			if (bFireThisEvent)
			{
				Events[i].EventFunc.ExecuteIfBound();
			}
		}
#endif
	}

#if 0
	// Execute the delegate to say that all properties are updated
	TimelinePostUpdateFunc.ExecuteIfBound();
#endif

	if (OldPosition != AccumulatedTime)
	{
		CalculateCurrentFrame();
	}
}

float UPaperFlipbookComponent::GetPlaybackPosition() const
{
	return AccumulatedTime;
}

void UPaperFlipbookComponent::SetLooping(bool bNewLooping)
{
	bLooping = bNewLooping;
}

bool UPaperFlipbookComponent::IsLooping() const
{
	return bLooping;
}

void UPaperFlipbookComponent::SetPlayRate(float NewRate)
{
	PlayRate = NewRate;
}

float UPaperFlipbookComponent::GetPlayRate() const
{
	return PlayRate;
}

void UPaperFlipbookComponent::SetNewTime(float NewTime)
{
	// Ensure value is sensible
	//@TODO: PAPER2D: NewTime = FMath::Clamp<float>(NewTime, 0.0f, Length);

	SetPlaybackPosition(NewTime, /*bFireEvents=*/ false);
}

float UPaperFlipbookComponent::GetFlipbookLength() const
{
	return (SourceFlipbook != nullptr) ? SourceFlipbook->GetTotalDuration() : 0.0f;
}
