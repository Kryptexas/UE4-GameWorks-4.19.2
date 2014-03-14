// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperRenderSceneProxy.h"
#include "PaperRenderComponent.h"

//////////////////////////////////////////////////////////////////////////
// UPaperRenderComponent

UPaperRenderComponent::UPaperRenderComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetCollisionProfileName(UCollisionProfile::BlockAllDynamic_ProfileName);

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial.DefaultSpriteMaterial"));
	TestMaterial = DefaultMaterial.Object;

	SpriteColor = FLinearColor::White;

	Mobility = EComponentMobility::Movable; //@TODO: Change the default!
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	bTickInEditor = true;
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
		if (UBodySetup* BodySetup = SourceSprite->BodySetup3D)
		{
			const FBox AggGeomBox = BodySetup->AggGeom.CalcAABB(LocalToWorld);
			if (AggGeomBox.IsValid)
			{
				NewBounds = Union(NewBounds,FBoxSphereBounds(AggGeomBox));
			}
		}

		// Takes into account that the static mesh collision code nudges collisions out by up to 1 unit.
		//@TODO: Needed?  (copied from StaticMeshComponent)
		NewBounds.BoxExtent += FVector(1,1,1);
		NewBounds.SphereRadius += 1.0f;
		NewBounds.BoxExtent *= BoundsScale;
		NewBounds.SphereRadius *= BoundsScale;

		return NewBounds;
	}
	else
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
	}
}

void UPaperRenderComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Indicate we need to send new dynamic data.
	MarkRenderDynamicDataDirty();
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

void UPaperRenderComponent::CreatePhysicsState()
{
	Super::CreatePhysicsState();
	CreatePhysicsState2D();
}

void UPaperRenderComponent::DestroyPhysicsState()
{
	Super::DestroyPhysicsState();
	DestroyPhysicsState2D();
}

void UPaperRenderComponent::DestroyPhysicsState2D()
{
	// clean up physics engine representation
	if (BodyInstance2D.IsValidBodyInstance())
	{
		BodyInstance2D.TermBody();
	}
}

void UPaperRenderComponent::CreatePhysicsState2D()
{
	if (!BodyInstance2D.IsValidBodyInstance() && (SourceSprite != NULL) && (SourceSprite->SpriteCollisionDomain == ESpriteCollisionMode::Use2DPhysics))
	{
		UBodySetup2D* BodySetup = GetRBBodySetup2D();
		//@TODO: if (UBodySetup2D* BodySetup = GetRBBodySetup2D())
		{
			BodyInstance2D.InitBody(BodySetup, SourceSprite, ComponentToWorld, this);
		}
	}
}

UBodySetup2D* UPaperRenderComponent::GetRBBodySetup2D()
{
	return NULL;
}

void UPaperRenderComponent::OnUpdateTransform(bool bSkipPhysicsMove)
{
	Super::OnUpdateTransform(bSkipPhysicsMove);

	// Always send new transform to physics
	if (bPhysicsStateCreated && !bSkipPhysicsMove)
	{
		// @todo UE4 rather than always pass false, this function should know if it is being teleported or not!
		SendPhysicsTransform(false);

		BodyInstance2D.SetBodyTransform(ComponentToWorld);
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
			return Socket->LocalTransform * ComponentToWorld;
			switch(TransformSpace)
			{
				case RTS_World:
				{
					return Socket->LocalTransform * ComponentToWorld;
				}
				case RTS_Actor:
				{
					if( const AActor* Actor = GetOwner() )
					{
						FTransform SocketTransform = Socket->LocalTransform * ComponentToWorld;
						return SocketTransform.GetRelativeTransform(Actor->GetTransform());
					}
					break;
				}
				case RTS_Component:
				{
					return Socket->LocalTransform;
				}
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

void UPaperRenderComponent::SetSimulatePhysics(bool bSimulate)
{
	Super::SetSimulatePhysics(bSimulate);
	BodyInstance2D.SetInstanceSimulatePhysics(bSimulate);
}

UBodySetup* UPaperRenderComponent::GetBodySetup()
{
	return (SourceSprite != NULL) ? SourceSprite->BodySetup3D : NULL;
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
