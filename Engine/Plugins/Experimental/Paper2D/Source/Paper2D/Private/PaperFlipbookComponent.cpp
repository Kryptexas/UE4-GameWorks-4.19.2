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

void UPaperFlipbookComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Advance time
	const float TotalTime = (SourceFlipbook != nullptr) ? SourceFlipbook->GetTotalDuration() : 0.0f;
	if (TotalTime > 0.0f)
	{
		AccumulatedTime += DeltaTime * PlayRate;
		AccumulatedTime = FMath::Fmod(AccumulatedTime, TotalTime);
	}
	else
	{
		AccumulatedTime = 0.0f;
	}

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

void UPaperFlipbookComponent::SetCurrentTime(float NewTime)
{
	if (NewTime != AccumulatedTime)
	{
		AccumulatedTime = NewTime;
		CalculateCurrentFrame();
	}
}

const UObject* UPaperFlipbookComponent::AdditionalStatObject() const
{
	return SourceFlipbook;
}
