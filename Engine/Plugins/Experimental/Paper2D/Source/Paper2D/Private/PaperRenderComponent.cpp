// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRenderSceneProxy.h"
#include "PaperRenderComponent.h"
#include "PhysicsEngine/BodySetup2D.h"

//////////////////////////////////////////////////////////////////////////
// UPaperRenderComponent

UPaperRenderComponent::UPaperRenderComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	MaterialOverride = nullptr;

	SpriteColor = FLinearColor::White;

	Mobility = EComponentMobility::Static;
}

FPrimitiveSceneProxy* UPaperRenderComponent::CreateSceneProxy()
{
	FPaperRenderSceneProxy* NewProxy = new FPaperRenderSceneProxy(this);
	FSpriteDrawCallRecord DrawCall;
	DrawCall.BuildFromSprite(SourceSprite);
	NewProxy->SetDrawCall_RenderThread(DrawCall);
	return NewProxy;
}

FBoxSphereBounds UPaperRenderComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	if (SourceSprite != NULL)
	{
		// Graphics bounds.
		FBoxSphereBounds NewBounds = SourceSprite->GetRenderBounds().TransformBy(LocalToWorld);

		// Add bounds of collision geometry (if present).
		if (UBodySetup* BodySetup = SourceSprite->BodySetup)
		{
			const FBox AggGeomBox = BodySetup->AggGeom.CalcAABB(LocalToWorld);
			if (AggGeomBox.IsValid)
			{
				NewBounds = Union(NewBounds,FBoxSphereBounds(AggGeomBox));
			}
		}

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

void UPaperRenderComponent::SendRenderDynamicData_Concurrent()
{
	if (SceneProxy != NULL)
	{
		FSpriteDrawCallRecord DrawCall;
		DrawCall.BuildFromSprite(SourceSprite);
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

bool UPaperRenderComponent::HasAnySockets() const
{
	if (SourceSprite != NULL)
	{
		return SourceSprite->HasAnySockets();
	}

	return false;
}

FTransform UPaperRenderComponent::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
	if (SourceSprite != NULL)
	{
		if (FPaperSpriteSocket* Socket = SourceSprite->FindSocket(InSocketName))
		{
			switch (TransformSpace)
			{
				case RTS_World:
					return Socket->LocalTransform * ComponentToWorld;

				case RTS_Actor:
					if (const AActor* Actor = GetOwner())
					{
						const FTransform SocketTransform = Socket->LocalTransform * ComponentToWorld;
						return SocketTransform.GetRelativeTransform(Actor->GetTransform());
					}
					break;

				case RTS_Component:
					return Socket->LocalTransform;

				default:
					check(false);
			}
		}
	}

	return FTransform::Identity;
}

void UPaperRenderComponent::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	if (SourceSprite != NULL)
	{
		return SourceSprite->QuerySupportedSockets(OutSockets);
	}
}

UBodySetup* UPaperRenderComponent::GetBodySetup()
{
	return (SourceSprite != nullptr) ? SourceSprite->BodySetup : nullptr;
}

bool UPaperRenderComponent::SetSprite(class UPaperSprite* NewSprite)
{
	if (NewSprite != SourceSprite)
	{
		// Don't allow changing the sprite if we are "static".
		AActor* Owner = GetOwner();
		if (!IsRegistered() || (Owner == NULL) || (Mobility != EComponentMobility::Static))
		{
			SourceSprite = NewSprite;

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

UPaperSprite* UPaperRenderComponent::GetSprite()
{
	return SourceSprite;
}

void UPaperRenderComponent::SetSpriteColor(FLinearColor NewColor)
{
	// Can't set color on a static component
	if (!(IsRegistered() && (Mobility == EComponentMobility::Static)) && (SpriteColor != NewColor))
	{
		SpriteColor = NewColor;

		//@TODO: Should we send immediately?
		MarkRenderDynamicDataDirty();
	}
}

FLinearColor UPaperRenderComponent::GetWireframeColor() const
{
	return FLinearColor::Yellow;
}

const UObject* UPaperRenderComponent::AdditionalStatObject() const
{
	return SourceSprite;
}
