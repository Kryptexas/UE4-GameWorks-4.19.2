// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "EnginePrivate.h"


UCapsuleComponent::UCapsuleComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	ShapeColor = FColor(223, 149, 157, 255);

	CapsuleRadius = 22.0f;
	CapsuleHalfHeight = 44.0f;
	bUseEditorCompositing = true;
}



FPrimitiveSceneProxy* UCapsuleComponent::CreateSceneProxy()
{
	/** Represents a UCapsuleComponent to the scene manager. */
	class FDrawCylinderSceneProxy : public FPrimitiveSceneProxy
	{
	public:
		FDrawCylinderSceneProxy(const UCapsuleComponent* InComponent)
			:	FPrimitiveSceneProxy(InComponent)
			,	bDrawOnlyIfSelected( InComponent->bDrawOnlyIfSelected )
			,	CapsuleRadius( InComponent->CapsuleRadius )
			,	CapsuleHalfHeight( InComponent->CapsuleHalfHeight )
			,	ShapeColor( InComponent->ShapeColor )
		{
			bWillEverBeLit = false;
		}

		/** 
		* Draw the scene proxy as a dynamic element
		*
		* @param	PDI - draw interface to render to
		* @param	View - current view
		*/
		virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View) OVERRIDE
		{
			QUICK_SCOPE_CYCLE_COUNTER( STAT_DrawCylinderSceneProxy_DrawDynamicElements );

			const FLinearColor DrawCapsuleColor = GetSelectionColor(ShapeColor, IsSelected(), IsHovered(), /*bUseOverlayIntensity=*/false);
			const FMatrix& LocalToWorld = GetLocalToWorld();
			const int32 CapsuleSides =  FMath::Clamp<int32>(CapsuleRadius/4.f, 16, 64);
			DrawWireCapsule( PDI, LocalToWorld.GetOrigin(), LocalToWorld.GetScaledAxis( EAxis::X ), LocalToWorld.GetScaledAxis( EAxis::Y ), LocalToWorld.GetScaledAxis( EAxis::Z ), DrawCapsuleColor, CapsuleRadius, CapsuleHalfHeight, CapsuleSides, SDPG_World );
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
		const float		CapsuleRadius;
		const float		CapsuleHalfHeight;
		const FColor	ShapeColor;
	};

	return new FDrawCylinderSceneProxy( this );
}


FBoxSphereBounds UCapsuleComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FVector BoxPoint = FVector(CapsuleRadius,CapsuleRadius,CapsuleHalfHeight);
	return FBoxSphereBounds(FVector::ZeroVector, BoxPoint, BoxPoint.Size()).TransformBy(LocalToWorld);
}

void UCapsuleComponent::CalcBoundingCylinder(float& CylinderRadius, float& CylinderHalfHeight) const 
{
	const float Scale = ComponentToWorld.GetMaximumAxisScale();
	const float CapsuleEndCapCenter = FMath::Max(CapsuleHalfHeight - CapsuleRadius, 0.f);
	const FVector ZAxis = ComponentToWorld.TransformVectorNoScale(FVector(0.f, 0.f, CapsuleEndCapCenter * Scale));
	
	const float ScaledRadius = CapsuleRadius * Scale;
	
	CylinderRadius		= ScaledRadius + FMath::Sqrt(FMath::Square(ZAxis.X) + FMath::Square(ZAxis.Y));
	CylinderHalfHeight	= ScaledRadius + ZAxis.Z;
}

void UCapsuleComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && (Ar.UE4Ver() < VER_UE4_AFTER_CAPSULE_HALF_HEIGHT_CHANGE))
	{
		if ((CapsuleHeight_DEPRECATED != 0.0f) || (Ar.UE4Ver() < VER_UE4_BLUEPRINT_VARS_NOT_READ_ONLY))
		{
			CapsuleHalfHeight = CapsuleHeight_DEPRECATED;
			CapsuleHeight_DEPRECATED = 0.0f;
		}
	}

	CapsuleHalfHeight = FMath::Max(CapsuleHalfHeight, CapsuleRadius);
}

#if WITH_EDITOR
void UCapsuleComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.Property ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == FName(TEXT("CapsuleHalfHeight")) || PropertyName == FName(TEXT("CapsuleRadius")))
	{
		CapsuleHalfHeight = FMath::Max(CapsuleHalfHeight, CapsuleRadius);
	}
}
#endif	// WITH_EDITOR

void UCapsuleComponent::SetCapsuleSize(float NewRadius, float NewHalfHeight, bool bUpdateOverlaps)
{
	CapsuleHalfHeight = FMath::Max(NewHalfHeight, NewRadius);
	CapsuleRadius = NewRadius;
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

void UCapsuleComponent::UpdateBodySetup()
{
	if(ShapeBodySetup == NULL)
	{
		check(!IsTemplate());
		ShapeBodySetup = ConstructObject<UBodySetup>(UBodySetup::StaticClass(), this);
		ShapeBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;

		ShapeBodySetup->AggGeom.SphylElems.AddZeroed();
	}

	check(ShapeBodySetup->AggGeom.SphylElems.Num() == 1);
	FKSphylElem* SE = ShapeBodySetup->AggGeom.SphylElems.GetTypedData();

	// apply non uniform scale factor
	// min scale is applied when body is created
	FVector Scale3D = ComponentToWorld.GetScale3D().GetAbs();
	float MinScale = Scale3D.GetMin();
	float ScaleRadius;
	float ScaleLength;
	// do not set scale radius/length to be 0.f
	// that will cause error on initializing physics
	// physics will handle with min threshold number	
	if (MinScale > 0.f)
	{
		// divided by min since that min scale will be applied in InitBody
		ScaleRadius = Scale3D.GetMax() / MinScale;
		ScaleLength = Scale3D.Z/MinScale;
	}
	else
	{
		ScaleRadius = 1.f;
		ScaleLength = 1.f;
	}

	SE->SetTransform( FTransform::Identity );

	// this is a bit confusing since radius and height is scaled
	// first apply the scale first 
	float Radius = FMath::Max(CapsuleRadius * ScaleRadius, 0.1f);
	float HalfHeight = CapsuleHalfHeight * ScaleLength;

	// now find half height without the caps
	HalfHeight = FMath::Max<float>(HalfHeight - Radius, 1.f);
	// set to final value
	SE->Radius = Radius;
	SE->Length = 2 * HalfHeight;
}

bool UCapsuleComponent::IsZeroExtent() const
{
	return (CapsuleRadius == 0.f) && (CapsuleHalfHeight == 0.f);
}


FCollisionShape UCapsuleComponent::GetCollisionShape(float Inflation) const
{
	const float Radius = FMath::Max(0.f, GetScaledCapsuleRadius() + Inflation);
	const float HalfHeight = FMath::Max(0.f, GetScaledCapsuleHalfHeight() + Inflation);
	return FCollisionShape::MakeCapsule(Radius, HalfHeight);
}

bool UCapsuleComponent::AreSymmetricRotations(const FQuat& A, const FQuat& B, const FVector& Scale3D) const
{
	if (Scale3D.X != Scale3D.Y)
	{
		return false;
	}

	const FVector AUp = A.GetAxisZ();
	const FVector BUp = B.GetAxisZ();
	return AUp.Equals(BUp);
}
