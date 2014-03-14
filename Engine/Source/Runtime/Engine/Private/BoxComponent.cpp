// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"

UBoxComponent::UBoxComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BoxExtent = FVector(32.0f, 32.0f, 32.0f);

	bUseEditorCompositing = true;
}


void UBoxComponent::SetBoxExtent(FVector NewBoxExtent, bool bUpdateOverlaps)
{
	BoxExtent = NewBoxExtent;
	MarkRenderStateDirty();

	// do this if already created
	// otherwise, it hasn't been really created yet
	if (bPhysicsStateCreated)
	{
		DestroyPhysicsState();
		UpdateBodySetup();
		CreatePhysicsState();

		if ( bUpdateOverlaps && IsCollisionEnabled() && GetOwner() )
		{
			UpdateOverlaps();
		}
	}
}

void UBoxComponent::UpdateBodySetup()
{
	if( ShapeBodySetup == NULL )
	{
		ShapeBodySetup = ConstructObject<UBodySetup>(UBodySetup::StaticClass(), this);
		ShapeBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		ShapeBodySetup->AggGeom.BoxElems.AddZeroed();
	}

	check(ShapeBodySetup->AggGeom.BoxElems.Num() == 1);
	FKBoxElem* se = ShapeBodySetup->AggGeom.BoxElems.GetTypedData();

	// @todo UE4 do we allow this now?
	// check for malformed values
	if( BoxExtent.X < KINDA_SMALL_NUMBER )
	{
		BoxExtent.X = 1.0f;
	}

	if( BoxExtent.Y < KINDA_SMALL_NUMBER )
	{
		BoxExtent.Y = 1.0f;
	}

	if( BoxExtent.Z < KINDA_SMALL_NUMBER )
	{
		BoxExtent.Z = 1.0f;
	}

	// apply non uniform scale factor
	// min scale is applied when body is created
	FVector Scale3D = ComponentToWorld.GetScale3D().GetAbs();
	// divided by min since that will be applied to physX
	Scale3D /= Scale3D.GetMin();

	// now set the PhysX data values
	se->SetTransform( FTransform::Identity );
	se->X = BoxExtent.X*2*Scale3D.X;
	se->Y = BoxExtent.Y*2*Scale3D.Y;
	se->Z = BoxExtent.Z*2*Scale3D.Z;
}

bool UBoxComponent::IsZeroExtent() const
{
	return BoxExtent.IsZero();
}

FBoxSphereBounds UBoxComponent::CalcBounds(const FTransform & LocalToWorld) const 
{
	return FBoxSphereBounds( FBox( -BoxExtent, BoxExtent ) ).TransformBy(LocalToWorld);
}



FPrimitiveSceneProxy* UBoxComponent::CreateSceneProxy()
{
	/** Represents a UCapsuleComponent to the scene manager. */
	class FBoxSceneProxy : public FPrimitiveSceneProxy
	{
	public:
		FBoxSceneProxy(const UBoxComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	bDrawOnlyIfSelected( InComponent->bDrawOnlyIfSelected )
			,   BoxExtents( InComponent->BoxExtent )
			,	BoxColor( InComponent->ShapeColor )
		{
			bWillEverBeLit = false;
		}


		virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) OVERRIDE
		{
			QUICK_SCOPE_CYCLE_COUNTER( STAT_BoxSceneProxy_DrawDynamicElements );

			const FMatrix& LocalToWorld = GetLocalToWorld();
			const FColor DrawColor = GetSelectionColor(BoxColor, IsSelected(), IsHovered(), /*bUseOverlayIntensity=*/false);
			DrawOrientedWireBox(PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), BoxExtents, DrawColor, SDPG_World);
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE
		{
			const bool bVisible = !bDrawOnlyIfSelected || IsSelected();

			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View) && bVisible;
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}
		virtual uint32 GetMemoryFootprint( void ) const OVERRIDE { return( sizeof( *this ) + GetAllocatedSize() ); }
		uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

	private:
		const uint32	bDrawOnlyIfSelected:1;
		const FVector	BoxExtents;
		const FColor	BoxColor;
	};

	return new FBoxSceneProxy( this );
}


FCollisionShape UBoxComponent::GetCollisionShape(float Inflation) const
{
	FVector Extent = GetScaledBoxExtent() + Inflation;
	if (Inflation < 0.f)
	{
		// Don't shrink below zero size.
		Extent = Extent.ComponentMax(FVector::ZeroVector);
	}

	return FCollisionShape::MakeBox(Extent);
}
