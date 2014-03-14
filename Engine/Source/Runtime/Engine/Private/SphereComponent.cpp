// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"

USphereComponent::USphereComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	SphereRadius = 32.0f;
	ShapeColor = FColor(255, 0, 0, 255);

	bUseEditorCompositing = true;
}

FBoxSphereBounds USphereComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	return FBoxSphereBounds( FVector::ZeroVector, FVector(SphereRadius), SphereRadius ).TransformBy(LocalToWorld);
}

void USphereComponent::CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const
{
	CylinderRadius = SphereRadius * ComponentToWorld.GetMaximumAxisScale();
	CylinderHalfHeight = CylinderRadius;
}

void USphereComponent::UpdateBodySetup()
{
	if( ShapeBodySetup == NULL )
	{
		ShapeBodySetup = ConstructObject<UBodySetup>(UBodySetup::StaticClass(), this);
		ShapeBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
		ShapeBodySetup->AggGeom.SphereElems.AddZeroed();
	}

	check (ShapeBodySetup->AggGeom.SphereElems.Num() == 1);
	FKSphereElem* se = ShapeBodySetup->AggGeom.SphereElems.GetTypedData();

	// check for mal formed values
	float Radius = SphereRadius;
	if( Radius < KINDA_SMALL_NUMBER )
	{
		Radius = 0.1f;
	}

	// now set the PhysX data values
	se->Center = FVector::ZeroVector;
	se->Radius = Radius;
}

void USphereComponent::SetSphereRadius( float InSphereRadius, bool bUpdateOverlaps )
{
	SphereRadius = InSphereRadius;
	MarkRenderStateDirty();

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

bool USphereComponent::IsZeroExtent() const
{
	return SphereRadius == 0.f;
}


FPrimitiveSceneProxy* USphereComponent::CreateSceneProxy()
{
	/** Represents a DrawLightRadiusComponent to the scene manager. */
	class FSphereSceneProxy : public FPrimitiveSceneProxy
	{
	public:

		/** Initialization constructor. */
		FSphereSceneProxy(const USphereComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	bDrawOnlyIfSelected( InComponent->bDrawOnlyIfSelected )
			,	SphereColor(InComponent->ShapeColor)
			,	ShapeMaterial(InComponent->ShapeMaterial)
			,	SphereRadius(InComponent->SphereRadius)
		{
			bWillEverBeLit = false;
		}

		  // FPrimitiveSceneProxy interface.
		  
		/** 
		* Draw the scene proxy as a dynamic element
		*
		* @param	PDI - draw interface to render to
		* @param	View - current view
		*/
		virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) OVERRIDE
		{
			QUICK_SCOPE_CYCLE_COUNTER( STAT_SphereSceneProxy_DrawDynamicElements );

			const FMatrix& LocalToWorld = GetLocalToWorld();
			int32 SphereSides =  FMath::Clamp<int32>(SphereRadius/4.f, 16, 64);
			if(ShapeMaterial && !View->Family->EngineShowFlags.Wireframe)
			{
				DrawSphere(PDI,LocalToWorld.GetOrigin(), FVector(SphereRadius), SphereSides, SphereSides/2, ShapeMaterial->GetRenderProxy(false), SDPG_World);
			}
			else
			{
				const FLinearColor DrawSphereColor = GetSelectionColor(SphereColor, IsSelected(), IsHovered(), /*bUseOverlayIntensity=*/false);
				DrawCircle( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), DrawSphereColor, SphereRadius, SphereSides, SDPG_World );
				DrawCircle( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Z ), DrawSphereColor, SphereRadius, SphereSides, SDPG_World );
				DrawCircle( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), DrawSphereColor, SphereRadius, SphereSides, SDPG_World );
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)  OVERRIDE
		{
			const bool bVisibleForSelection = !bDrawOnlyIfSelected || IsSelected();
			const bool bVisibleForShowFlags = true; // @TODO

			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View) && bVisibleForSelection && bVisibleForShowFlags;
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}

		virtual uint32 GetMemoryFootprint( void ) const OVERRIDE { return( sizeof( *this ) + GetAllocatedSize() ); }
		uint32 GetAllocatedSize( void ) const { return( FPrimitiveSceneProxy::GetAllocatedSize() ); }

	private:
		const uint32				bDrawOnlyIfSelected:1;
		const FColor				SphereColor;
		const UMaterialInterface*	ShapeMaterial;
		const float					SphereRadius;
	};

	return new FSphereSceneProxy( this );
}


FCollisionShape USphereComponent::GetCollisionShape(float Inflation) const
{
	const float Radius = FMath::Max(0.f, GetScaledSphereRadius() + Inflation);
	return FCollisionShape::MakeSphere(Radius);
}

bool USphereComponent::AreSymmetricRotations(const FQuat& A, const FQuat& B, const FVector& Scale3D) const
{
	// All rotations are equal when scale is uniform.
	// Not detecting rotations around non-uniform scale.
	return Scale3D.GetAbs().AllComponentsEqual() || A.Equals(B);
}
