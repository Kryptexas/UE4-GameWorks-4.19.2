// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperAnimatedRenderSceneProxy.h"
#include "PaperAnimatedRenderComponent.h"

//////////////////////////////////////////////////////////////////////////
// UPaperAnimatedRenderComponent

UPaperAnimatedRenderComponent::UPaperAnimatedRenderComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial"));
	Material = DefaultMaterial.Object;

	SpriteColor = FLinearColor::White;

	Mobility = EComponentMobility::Movable; //@TODO: Change the default!
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	bTickInEditor = true;
	
	CachedFrameIndex = INDEX_NONE;
	AccumulatedTime = 0.0f;
	PlayRate = 1.0f;
}

FPrimitiveSceneProxy* UPaperAnimatedRenderComponent::CreateSceneProxy()
{
	return new FPaperAnimatedRenderSceneProxy(this);
}

FBoxSphereBounds UPaperAnimatedRenderComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	const float NewScale = LocalToWorld.GetScale3D().GetMax() * 300.0f;//(Sprite ? (float)FMath::Max(Sprite->GetSizeX(),Sprite->GetSizeY()) : 1.0f);  //@TODO: Compute a realistic bound
	return FBoxSphereBounds(LocalToWorld.GetTranslation(), FVector(NewScale,NewScale,NewScale), FMath::Sqrt(3.0f * FMath::Square(NewScale)));
}

void UPaperAnimatedRenderComponent::CalculateCurrentFrame()
{
	if (SourceFlipbook != NULL && SourceFlipbook->FramesPerSecond > 0)
	{
		int32 LastCachedFrame = CachedFrameIndex;

		float SumTime = 0.0f;
		int32 FrameIndex;
		for (FrameIndex = 0; FrameIndex < SourceFlipbook->KeyFrames.Num(); ++FrameIndex)
		{
			SumTime += SourceFlipbook->KeyFrames[FrameIndex].FrameRun / SourceFlipbook->FramesPerSecond;

			if (AccumulatedTime < SumTime)
			{
				break;
			}
		}

		if (AccumulatedTime >= SumTime)
		{
			AccumulatedTime = FMath::Fmod(AccumulatedTime, SumTime);
			//@TODO: Could have gone farther than this!!!
			CachedFrameIndex = 0;
		}
		else
		{
			CachedFrameIndex = FrameIndex;
		}

		if (CachedFrameIndex != LastCachedFrame)
		{
			// Indicate we need to send new dynamic data.
			MarkRenderDynamicDataDirty();
		}
	}
}

void UPaperAnimatedRenderComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Indicate we need to send new dynamic data.
	MarkRenderDynamicDataDirty();
	
	AccumulatedTime += DeltaTime * PlayRate;
	CalculateCurrentFrame();

	//Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UPaperAnimatedRenderComponent::SendRenderDynamicData_Concurrent()
{
	if (SceneProxy != NULL)
	{
		UPaperSprite* SpriteToSend = NULL;
		if ((SourceFlipbook != NULL) && (CachedFrameIndex >= 0) && (CachedFrameIndex < SourceFlipbook->KeyFrames.Num()))
		{
			SpriteToSend = SourceFlipbook->KeyFrames[CachedFrameIndex].Sprite;
		}

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

bool UPaperAnimatedRenderComponent::SetFlipbook(class UPaperFlipbook* NewFlipbook)
{
	if (NewFlipbook != SourceFlipbook)
	{
		// Don't allow changing the sprite if we are "static".
		AActor* Owner = GetOwner();
		if (!IsRegistered() || (Owner == NULL) || (Mobility != EComponentMobility::Static))
		{
			SourceFlipbook = NewFlipbook;

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

UPaperFlipbook* UPaperAnimatedRenderComponent::GetFlipbook()
{
	return SourceFlipbook;
}

UMaterialInterface* UPaperAnimatedRenderComponent::GetSpriteMaterial() const
{
	return Material;
}

void UPaperAnimatedRenderComponent::SetSpriteColor(FLinearColor NewColor)
{
	// Can't set color on a static component
	if (!(IsRegistered() && (Mobility == EComponentMobility::Static)) && (SpriteColor != NewColor))
	{
		SpriteColor = NewColor;

		//@TODO: Should we send immediately?
		MarkRenderDynamicDataDirty();
	}
}

void UPaperAnimatedRenderComponent::SetCurrentTime(float NewTime)
{
	// Can't set color on a static component
	if (!(IsRegistered() && (Mobility == EComponentMobility::Static)) && (NewTime != AccumulatedTime))
	{
		AccumulatedTime = NewTime;
		CalculateCurrentFrame();
	}
}
