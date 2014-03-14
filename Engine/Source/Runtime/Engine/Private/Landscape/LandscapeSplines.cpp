// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	LandscapeSpline.cpp
=============================================================================*/

#include "EnginePrivate.h"
#include "EngineTerrainClasses.h"
#include "Landscape/LandscapeSplineProxies.h"

IMPLEMENT_HIT_PROXY(HLandscapeSplineProxy, HHitProxy);
IMPLEMENT_HIT_PROXY(HLandscapeSplineProxy_Segment, HLandscapeSplineProxy);
IMPLEMENT_HIT_PROXY(HLandscapeSplineProxy_ControlPoint, HLandscapeSplineProxy);
IMPLEMENT_HIT_PROXY(HLandscapeSplineProxy_Tangent, HLandscapeSplineProxy);

//////////////////////////////////////////////////////////////////////////
// LANDSCAPE SPLINES SCENE PROXY

/** Represents a ULandscapeSplinesComponent to the scene manager. */
#if WITH_EDITOR
class FLandscapeSplinesSceneProxy : public FPrimitiveSceneProxy
{
private:
	const FLinearColor	SplineColor;

	const UTexture2D*	ControlPointSprite;
	const bool			bDrawFalloff;

	struct FSegmentProxy
	{
		ULandscapeSplineSegment* Owner;
		TRefCountPtr<HHitProxy> HitProxy;
		TArray<FLandscapeSplineInterpPoint> Points;
		uint32 bSelected : 1;
	};
	TArray<FSegmentProxy> Segments;

	struct FControlPointProxy
	{
		ULandscapeSplineControlPoint* Owner;
		TRefCountPtr<HHitProxy> HitProxy;
		FVector Location;
		TArray<FLandscapeSplineInterpPoint> Points;
		uint32 bSelected : 1;
	};
	TArray<FControlPointProxy> ControlPoints;

public:

	~FLandscapeSplinesSceneProxy()
	{
	}

	FLandscapeSplinesSceneProxy(ULandscapeSplinesComponent* Component):
		FPrimitiveSceneProxy(Component),
		SplineColor(Component->SplineColor),
		ControlPointSprite(Component->ControlPointSprite),
		bDrawFalloff(Component->bShowSplineEditorMesh)
	{
		Segments.Reserve(Component->Segments.Num());
		for (int32 i = 0; i < Component->Segments.Num(); i++)
		{
			ULandscapeSplineSegment* Segment = Component->Segments[i];

			FSegmentProxy SegmentProxy;
			SegmentProxy.Owner = Segment;
			SegmentProxy.HitProxy = NULL;
			SegmentProxy.Points = Segment->GetPoints();
			SegmentProxy.bSelected = Segment->IsSplineSelected();
			Segments.Add(SegmentProxy);
		}

		ControlPoints.Reserve(Component->ControlPoints.Num());
		for (int32 i = 0; i < Component->ControlPoints.Num(); i++)
		{
			ULandscapeSplineControlPoint* ControlPoint = Component->ControlPoints[i];

			FControlPointProxy ControlPointProxy;
			ControlPointProxy.Owner = ControlPoint;
			ControlPointProxy.HitProxy = NULL;
			ControlPointProxy.Location = ControlPoint->Location;
			ControlPointProxy.Points = ControlPoint->GetPoints();
			ControlPointProxy.bSelected = ControlPoint->IsSplineSelected();
			ControlPoints.Add(ControlPointProxy);
		}
	}

	virtual HHitProxy* CreateHitProxies(UPrimitiveComponent* Component, TArray<TRefCountPtr<HHitProxy> >& OutHitProxies)
	{
		OutHitProxies.Reserve(OutHitProxies.Num() + Segments.Num() + ControlPoints.Num());
		for (int32 iSegment = 0; iSegment < Segments.Num(); iSegment++)
		{
			FSegmentProxy& Segment = Segments[iSegment];
			Segment.HitProxy = new HLandscapeSplineProxy_Segment(Segment.Owner);
			OutHitProxies.Add(Segment.HitProxy);
		}
		for (int32 iControlPoint = 0; iControlPoint < ControlPoints.Num(); iControlPoint++)
		{
			FControlPointProxy& ControlPoint = ControlPoints[iControlPoint];
			ControlPoint.HitProxy = new HLandscapeSplineProxy_ControlPoint(ControlPoint.Owner);
			OutHitProxies.Add(ControlPoint.HitProxy);
		}
		return NULL;
	}

	virtual void DrawDynamicElements(FPrimitiveDrawInterface* PDI,const FSceneView* View)
	{
		// Slight Depth Bias so that the splines show up when they exactly match the target surface
		// e.g. someone playing with splines on a newly-created perfectly-flat landscape
		static const float DepthBias = -0.0001;

		const FMatrix& LocalToWorld = GetLocalToWorld();

		const FLinearColor SelectedSplineColor = GEngine->GetSelectedMaterialColor();
		const FLinearColor SelectedControlPointSpriteColor = FLinearColor::White + (GEngine->GetSelectedMaterialColor() * GEngine->SelectionHighlightIntensity * 10); // copied from FSpriteSceneProxy::DrawDynamicElements()

		for (int32 iSegment = 0; iSegment < Segments.Num(); iSegment++)
		{
			const FSegmentProxy& Segment = Segments[iSegment];

			const FLinearColor SegmentColor = Segment.bSelected ? SelectedSplineColor : SplineColor;

			FLandscapeSplineInterpPoint OldPoint = Segment.Points[0];
			OldPoint.Center       = LocalToWorld.TransformPosition(OldPoint.Center);
			OldPoint.Left         = LocalToWorld.TransformPosition(OldPoint.Left);
			OldPoint.Right        = LocalToWorld.TransformPosition(OldPoint.Right);
			OldPoint.FalloffLeft  = LocalToWorld.TransformPosition(OldPoint.FalloffLeft);
			OldPoint.FalloffRight = LocalToWorld.TransformPosition(OldPoint.FalloffRight);
			for(int32 i = 1; i < Segment.Points.Num(); i++)
			{
				FLandscapeSplineInterpPoint NewPoint = Segment.Points[i];
				NewPoint.Center       = LocalToWorld.TransformPosition(NewPoint.Center);
				NewPoint.Left         = LocalToWorld.TransformPosition(NewPoint.Left);
				NewPoint.Right        = LocalToWorld.TransformPosition(NewPoint.Right);
				NewPoint.FalloffLeft  = LocalToWorld.TransformPosition(NewPoint.FalloffLeft);
				NewPoint.FalloffRight = LocalToWorld.TransformPosition(NewPoint.FalloffRight);

				// Draw lines from the last keypoint.
				if (PDI->IsHitTesting()) PDI->SetHitProxy(Segment.HitProxy);

				// center line
				PDI->DrawLine(OldPoint.Center, NewPoint.Center, SegmentColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);

				// draw sides
				PDI->DrawLine(OldPoint.Left, NewPoint.Left, SegmentColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);
				PDI->DrawLine(OldPoint.Right, NewPoint.Right, SegmentColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);

				if (PDI->IsHitTesting()) PDI->SetHitProxy(NULL);

				// draw falloff sides
				if (bDrawFalloff)
				{
					DrawDashedLine(PDI, OldPoint.FalloffLeft, NewPoint.FalloffLeft, SegmentColor, 100, GetDepthPriorityGroup(View), DepthBias);
					DrawDashedLine(PDI, OldPoint.FalloffRight, NewPoint.FalloffRight, SegmentColor, 100, GetDepthPriorityGroup(View), DepthBias);
				}

				OldPoint = NewPoint;
			}
		}

		for (int32 iControlPoint = 0; iControlPoint < ControlPoints.Num(); iControlPoint++)
		{
			const FControlPointProxy& ControlPoint = ControlPoints[iControlPoint];

			const FVector ControlPointLocation = LocalToWorld.TransformPosition(ControlPoint.Location);

			// Draw Sprite

			const FLinearColor ControlPointSpriteColor = ControlPoint.bSelected ? SelectedControlPointSpriteColor : FLinearColor::White;

			if (PDI->IsHitTesting()) PDI->SetHitProxy(ControlPoint.HitProxy);

			PDI->DrawSprite(
				ControlPointLocation,
				ControlPointSprite->Resource->GetSizeX() * 2,
				ControlPointSprite->Resource->GetSizeY() * 2,
				ControlPointSprite->Resource,
				ControlPointSpriteColor,
				GetDepthPriorityGroup(View),
				0, ControlPointSprite->Resource->GetSizeX(),
				0, ControlPointSprite->Resource->GetSizeY(),
				SE_BLEND_Masked);


			// Draw Lines
			const FLinearColor ControlPointColor = ControlPoint.bSelected ? SelectedSplineColor : SplineColor;

			if (ControlPoint.Points.Num() == 1)
			{
				FLandscapeSplineInterpPoint NewPoint = ControlPoint.Points[0];
				NewPoint.Center = LocalToWorld.TransformPosition(NewPoint.Center);
				NewPoint.Left   = LocalToWorld.TransformPosition(NewPoint.Left);
				NewPoint.Right  = LocalToWorld.TransformPosition(NewPoint.Right);
				NewPoint.FalloffLeft  = LocalToWorld.TransformPosition(NewPoint.FalloffLeft);
				NewPoint.FalloffRight = LocalToWorld.TransformPosition(NewPoint.FalloffRight);

				// draw end for spline connection
				PDI->DrawPoint(NewPoint.Center, ControlPointColor, 6.0f, GetDepthPriorityGroup(View));
				PDI->DrawLine(NewPoint.Left, NewPoint.Center, ControlPointColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);
				PDI->DrawLine(NewPoint.Right, NewPoint.Center, ControlPointColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);
				if (bDrawFalloff)
				{
					DrawDashedLine(PDI, NewPoint.FalloffLeft, NewPoint.Left, ControlPointColor, 100, GetDepthPriorityGroup(View), DepthBias);
					DrawDashedLine(PDI, NewPoint.FalloffRight, NewPoint.Right, ControlPointColor, 100, GetDepthPriorityGroup(View), DepthBias);
				}
			}
			else if (ControlPoint.Points.Num() >= 2)
			{
				FLandscapeSplineInterpPoint OldPoint = ControlPoint.Points.Last();
				//OldPoint.Left   = LocalToWorld.TransformPosition(OldPoint.Left);
				OldPoint.Right  = LocalToWorld.TransformPosition(OldPoint.Right);
				//OldPoint.FalloffLeft  = LocalToWorld.TransformPosition(OldPoint.FalloffLeft);
				OldPoint.FalloffRight = LocalToWorld.TransformPosition(OldPoint.FalloffRight);

				for(int32 i = 0; i < ControlPoint.Points.Num(); i++)
				{
					FLandscapeSplineInterpPoint NewPoint = ControlPoint.Points[i];
					NewPoint.Center = LocalToWorld.TransformPosition(NewPoint.Center);
					NewPoint.Left   = LocalToWorld.TransformPosition(NewPoint.Left);
					NewPoint.Right  = LocalToWorld.TransformPosition(NewPoint.Right);
					NewPoint.FalloffLeft  = LocalToWorld.TransformPosition(NewPoint.FalloffLeft);
					NewPoint.FalloffRight = LocalToWorld.TransformPosition(NewPoint.FalloffRight);

					if (PDI->IsHitTesting()) PDI->SetHitProxy(ControlPoint.HitProxy);

					// center line
					PDI->DrawLine(ControlPointLocation, NewPoint.Center, ControlPointColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);

					// draw sides
					PDI->DrawLine(OldPoint.Right, NewPoint.Left, ControlPointColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);

					if (PDI->IsHitTesting()) PDI->SetHitProxy(NULL);

					// draw falloff sides
					if (bDrawFalloff)
					{
						DrawDashedLine(PDI, OldPoint.FalloffRight, NewPoint.FalloffLeft, ControlPointColor, 100, GetDepthPriorityGroup(View), DepthBias);
					}

					// draw end for spline connection
					PDI->DrawPoint(NewPoint.Center, ControlPointColor, 6.0f, GetDepthPriorityGroup(View));
					PDI->DrawLine(NewPoint.Left, NewPoint.Center, ControlPointColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);
					PDI->DrawLine(NewPoint.Right, NewPoint.Center, ControlPointColor, GetDepthPriorityGroup(View), 0.0f, DepthBias);
					if (bDrawFalloff)
					{
						DrawDashedLine(PDI, NewPoint.FalloffLeft, NewPoint.Left, ControlPointColor, 100, GetDepthPriorityGroup(View), DepthBias);
						DrawDashedLine(PDI, NewPoint.FalloffRight, NewPoint.Right, ControlPointColor, 100, GetDepthPriorityGroup(View), DepthBias);
					}

					//OldPoint = NewPoint;
					OldPoint.Right = NewPoint.Right;
					OldPoint.FalloffRight = NewPoint.FalloffRight;
				}
			}
		}

		if (PDI->IsHitTesting()) PDI->SetHitProxy(NULL);
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View)
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View) && View->Family->EngineShowFlags.Splines;
		Result.bDynamicRelevance = true;
		return Result;
	}

	virtual uint32 GetMemoryFootprint( void ) const
	{
		return sizeof( *this ) + GetAllocatedSize();
	}
	uint32 GetAllocatedSize( void ) const
	{
		uint32 AllocatedSize = FPrimitiveSceneProxy::GetAllocatedSize() + Segments.GetAllocatedSize();
		for (int32 iSegment = 0; iSegment < Segments.Num(); iSegment++)
		{
			const FSegmentProxy& Segment = Segments[iSegment];
			AllocatedSize += Segment.Points.GetAllocatedSize();
		}
		return AllocatedSize;
	}
};
#endif

//////////////////////////////////////////////////////////////////////////
// SPLINE COMPONENT

ULandscapeSplinesComponent::ULandscapeSplinesComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
	SplineResolution = 512;
	SplineColor = FColor(0, 192, 48);

	if (!IsRunningCommandlet())
	{
		// Structure to hold one-time initialization
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinder<UTexture2D> SpriteTexture;
			ConstructorHelpers::FObjectFinder<UStaticMesh> SplineEditorMesh;
			FConstructorStatics()
				: SpriteTexture(TEXT("/Engine/EditorResources/S_Terrain.S_Terrain"))
				, SplineEditorMesh(TEXT("/Engine/EditorLandscapeResources/SplineEditorMesh"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		ControlPointSprite = ConstructorStatics.SpriteTexture.Object;
		SplineEditorMesh = ConstructorStatics.SplineEditorMesh.Object;
	}
#endif
	//RelativeScale3D = FVector(1/100.0f, 1/100.0f, 1/100.0f); // cancel out landscape scale. The scale is set up when component is created, but for a default landscape it's this
}

void ULandscapeSplinesComponent::OnRegister()
{
	Super::OnRegister();

	for (int32 iControlPoint = 0; iControlPoint < ControlPoints.Num(); iControlPoint++)
	{
		ULandscapeSplineControlPoint* ControlPoint = ControlPoints[iControlPoint];
		ControlPoint->RegisterComponents();
	}
	for (int32 iSegment = 0; iSegment < Segments.Num(); iSegment++)
	{
		ULandscapeSplineSegment* Segment = Segments[iSegment];
		Segment->RegisterComponents();
	}
}

void ULandscapeSplinesComponent::OnUnregister()
{
	for (int32 iControlPoint = 0; iControlPoint < ControlPoints.Num(); iControlPoint++)
	{
		ULandscapeSplineControlPoint* ControlPoint = ControlPoints[iControlPoint];
		ControlPoint->UnregisterComponents();
	}
	for (int32 iSegment = 0; iSegment < Segments.Num(); iSegment++)
	{
		ULandscapeSplineSegment* Segment = Segments[iSegment];
		Segment->UnregisterComponents();
	}

	Super::OnUnregister();
}

#if WITH_EDITOR
FPrimitiveSceneProxy* ULandscapeSplinesComponent::CreateSceneProxy()
{
	return new FLandscapeSplinesSceneProxy(this);
}
#endif

FBoxSphereBounds ULandscapeSplinesComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	FBox NewBoundsCalc(0);

	for (int32 iControlPoint = 0; iControlPoint < ControlPoints.Num(); iControlPoint++)
	{
		ULandscapeSplineControlPoint* ControlPoint = ControlPoints[iControlPoint];
		NewBoundsCalc += ControlPoint->GetBounds();
	}

	for (int32 iSegment = 0; iSegment < Segments.Num(); iSegment++)
	{
		ULandscapeSplineSegment* Segment = Segments[iSegment];
		NewBoundsCalc += Segment->GetBounds();
	}

	NewBoundsCalc = NewBoundsCalc.TransformBy(LocalToWorld);
	return FBoxSphereBounds(NewBoundsCalc);
}

bool ULandscapeSplinesComponent::ModifySplines(bool bAlwaysMarkDirty /*= true*/)
{
	bool bSavedToTransactionBuffer = Modify(bAlwaysMarkDirty);

	for (int32 ControlPointIndex = 0; ControlPointIndex < ControlPoints.Num(); ControlPointIndex++)
	{
		ULandscapeSplineControlPoint* ControlPoint = ControlPoints[ControlPointIndex];
		if (ControlPoint)
		{
			bSavedToTransactionBuffer = ControlPoint->Modify(bAlwaysMarkDirty) || bSavedToTransactionBuffer;
		}
	}
	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); SegmentIndex++)
	{
		ULandscapeSplineSegment* Segment = Segments[SegmentIndex];
		if (Segment)
		{
			bSavedToTransactionBuffer = Segment->Modify(bAlwaysMarkDirty) || bSavedToTransactionBuffer;
		}
	}

	return bSavedToTransactionBuffer;
}

#if WITH_EDITOR
void ULandscapeSplinesComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const bool bUpdateCollision = PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive;

	for (int32 iControlPoint = 0; iControlPoint < ControlPoints.Num(); iControlPoint++)
	{
		ULandscapeSplineControlPoint* ControlPoint = ControlPoints[iControlPoint];
		ControlPoint->UpdateSplinePoints(bUpdateCollision);
	}
	for (int32 iSegment = 0; iSegment < Segments.Num(); iSegment++)
	{
		ULandscapeSplineSegment* Segment = Segments[iSegment];
		Segment->UpdateSplinePoints(bUpdateCollision);
	}
}

void ULandscapeSplinesComponent::ShowSplineEditorMesh(bool bShow)
{
	bShowSplineEditorMesh = bShow;

	for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); SegmentIndex++)
	{
		ULandscapeSplineSegment* Segment = Segments[SegmentIndex];
		if (Segment)
		{
			Segment->UpdateSplineEditorMesh();
		}
	}

	MarkRenderStateDirty();
}
#endif // WITH_EDITOR

//////////////////////////////////////////////////////////////////////////
// CONTROL POINT MESH COMPONENT

UControlPointMeshComponent::UControlPointMeshComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
#if WITH_EDITORONLY_DATA
	bSelected = false;
#endif
}

//////////////////////////////////////////////////////////////////////////
// SPLINE CONTROL POINT

ULandscapeSplineControlPoint::ULandscapeSplineControlPoint(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Width = 1000;
	SideFalloff = 1000;
	EndFalloff = 2000;

#if WITH_EDITORONLY_DATA
	Mesh = NULL;
	MeshScale = FVector(1);

	LayerName = NAME_None;
	bRaiseTerrain = true;
	bLowerTerrain = true;

	MeshComponent = NULL;
	bEnableCollision = true;
	bCastShadow = true;

	// transients
	bSelected = false;
#endif
}

void ULandscapeSplineControlPoint::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (MeshComponent != NULL)
	{
		MeshComponent->SetFlags(RF_TextExportTransient);
	}
#endif
}

FLandscapeSplineSegmentConnection& FLandscapeSplineConnection::GetNearConnection() const
{
	return Segment->Connections[End];
}

FLandscapeSplineSegmentConnection& FLandscapeSplineConnection::GetFarConnection() const
{
	return Segment->Connections[1 - End];
}

FName ULandscapeSplineControlPoint::GetBestConnectionTo(FVector Destination) const
{
	FName BestSocket = NAME_None;
	float BestScore = -FLT_MAX;

#if WITH_EDITORONLY_DATA
	if (Mesh != NULL)
	{
#else
	if (MeshComponent != NULL)
	{
		const UStaticMesh* Mesh = MeshComponent->StaticMesh;
		const FVector MeshScale = MeshComponent->RelativeScale3D;
#endif
		for (auto It = Mesh->Sockets.CreateConstIterator(); It; ++It)
		{
			const UStaticMeshSocket* Socket = *It;

			FTransform SocketTransform = FTransform(Socket->RelativeRotation, Socket->RelativeLocation) * FTransform(Rotation, Location, MeshScale);
			FVector SocketLocation = SocketTransform.GetTranslation();
			FRotator SocketRotation = SocketTransform.GetRotation().Rotator();

			float Score = (Destination - Location).Size() - (Destination - SocketLocation).Size(); // Score closer sockets higher
			Score *= FMath::Abs(FVector::DotProduct((Destination - SocketLocation), SocketRotation.Vector())); // score closer rotation higher

			if (Score > BestScore)
			{
				BestSocket = Socket->SocketName;
				BestScore = Score;
			}
		}
	}

	return BestSocket;
}

void ULandscapeSplineControlPoint::GetConnectionLocalLocationAndRotation(FName SocketName, OUT FVector& OutLocation, OUT FRotator& OutRotation) const
{
	OutLocation = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;

#if WITH_EDITORONLY_DATA
	if (Mesh != NULL)
	{
#else
	if (MeshComponent != NULL)
	{
		UStaticMesh* Mesh = MeshComponent->StaticMesh;
#endif
		const UStaticMeshSocket* Socket = Mesh->FindSocket(SocketName);
		if (Socket != NULL)
		{
			OutLocation = Socket->RelativeLocation;
			OutRotation = Socket->RelativeRotation;
		}
	}
}

void ULandscapeSplineControlPoint::GetConnectionLocationAndRotation(FName SocketName, OUT FVector& OutLocation, OUT FRotator& OutRotation) const
{
	OutLocation = Location;
	OutRotation = Rotation;

#if WITH_EDITORONLY_DATA
	if (Mesh != NULL)
	{
#else
	if (MeshComponent != NULL)
	{
		UStaticMesh* Mesh = MeshComponent->StaticMesh;
		const FVector MeshScale = MeshComponent->RelativeScale3D;
#endif
		const UStaticMeshSocket* Socket = Mesh->FindSocket(SocketName);
		if (Socket != NULL)
		{
			FTransform SocketTransform = FTransform(Socket->RelativeRotation, Socket->RelativeLocation) * FTransform(Rotation, Location, MeshScale);
			OutLocation = SocketTransform.GetTranslation();
			OutRotation = SocketTransform.GetRotation().Rotator().GetNormalized();
		}
	}
}

#if WITH_EDITOR
void ULandscapeSplineControlPoint::SetSplineSelected(bool bInSelected)
{
	bSelected = bInSelected;
	GetOuterULandscapeSplinesComponent()->MarkRenderStateDirty();

	if (MeshComponent != NULL)
	{
		MeshComponent->bSelected = bInSelected;
		MeshComponent->PushSelectionToProxy();
	}
}

void ULandscapeSplineControlPoint::AutoCalcRotation()
{
	Modify();

	FRotator Delta = FRotator::ZeroRotator;

	for (auto It = ConnectedSegments.CreateConstIterator(); It; It++)
	{
		// Get the start and end location/rotation of this connection
		FVector StartLocation; FRotator StartRotation;
		this->GetConnectionLocationAndRotation((*It).GetNearConnection().SocketName, StartLocation, StartRotation);
		FVector StartLocalLocation; FRotator StartLocalRotation;
		this->GetConnectionLocalLocationAndRotation((*It).GetNearConnection().SocketName, StartLocalLocation, StartLocalRotation);
		FVector EndLocation; FRotator EndRotation;
		(*It).GetFarConnection().ControlPoint->GetConnectionLocationAndRotation((*It).GetFarConnection().SocketName, EndLocation, EndRotation);

		// Find the delta between the direction of the tangent at the connection point and
		// the direction to the other end's control point
		FQuat SocketLocalRotation = StartLocalRotation.Quaternion();
		if (FMath::Sign((*It).GetNearConnection().TangentLen) < 0)
		{
			SocketLocalRotation = SocketLocalRotation * FRotator(0, 180, 0).Quaternion();
		}
		const FVector  DesiredDirection = (EndLocation - StartLocation);
		const FQuat    DesiredSocketRotation = DesiredDirection.Rotation().Quaternion();
		const FRotator DesiredRotation = (DesiredSocketRotation * SocketLocalRotation.Inverse()).Rotator().GetNormalized();
		const FRotator DesiredRotationDelta = (DesiredRotation - Rotation).GetNormalized();

		Delta += DesiredRotationDelta;
	}

	// Average delta of all connections
	Delta *= 1.0f / ConnectedSegments.Num();

	// Apply Delta and normalize
	Rotation = (Rotation + Delta).GetNormalized();
}

void ULandscapeSplineControlPoint::AutoFlipTangents()
{
	for (auto It = ConnectedSegments.CreateConstIterator(); It; It++)
	{
		(*It).Segment->AutoFlipTangents();
	}
}

void ULandscapeSplineControlPoint::AutoSetConnections(bool bIncludingValid)
{
	for (auto It = ConnectedSegments.CreateConstIterator(); It; It++)
	{
		FLandscapeSplineSegmentConnection& NearConnection = (*It).GetNearConnection();
		if (bIncludingValid ||
			(Mesh != NULL && Mesh->FindSocket(NearConnection.SocketName) == NULL) ||
			(Mesh == NULL && NearConnection.SocketName != NAME_None))
		{
			FLandscapeSplineSegmentConnection& FarConnection = (*It).GetFarConnection();
			FVector EndLocation; FRotator EndRotation;
			FarConnection.ControlPoint->GetConnectionLocationAndRotation(FarConnection.SocketName, EndLocation, EndRotation);

			NearConnection.SocketName = GetBestConnectionTo(EndLocation);
			NearConnection.TangentLen = FMath::Abs(NearConnection.TangentLen);

			// Allow flipping tangent on the null connection
			if (NearConnection.SocketName == NAME_None)
			{
				FVector StartLocation; FRotator StartRotation;
				NearConnection.ControlPoint->GetConnectionLocationAndRotation(NearConnection.SocketName, StartLocation, StartRotation);

				if (FVector::DotProduct((EndLocation - StartLocation).SafeNormal(), StartRotation.Vector()) < 0)
				{
					NearConnection.TangentLen = -NearConnection.TangentLen;
				}
			}
		}
	}
}
#endif

void ULandscapeSplineControlPoint::RegisterComponents()
{
	if (MeshComponent != NULL)
	{
		MeshComponent->RegisterComponent();
	}
}

void ULandscapeSplineControlPoint::UnregisterComponents()
{
	if (MeshComponent != NULL)
	{
		MeshComponent->UnregisterComponent();
	}
}

bool ULandscapeSplineControlPoint::OwnsComponent(const class UControlPointMeshComponent* StaticMeshComponent) const
{
	return MeshComponent == StaticMeshComponent;
}

#if WITH_EDITOR
void ULandscapeSplineControlPoint::UpdateSplinePoints(bool bUpdateCollision)
{
	ULandscapeSplinesComponent* OuterSplines = GetOuterULandscapeSplinesComponent();

	if (MeshComponent == NULL && Mesh != NULL ||
		MeshComponent != NULL &&
			(MeshComponent->StaticMesh != Mesh ||
			MeshComponent->RelativeLocation != Location ||
			MeshComponent->RelativeRotation != Rotation ||
			MeshComponent->RelativeScale3D  != MeshScale)
		)
	{
		OuterSplines->Modify();
		bNavDirty = true;

		if (MeshComponent)
		{
			MeshComponent->Modify();
			MeshComponent->UnregisterComponent();
		}

		if (Mesh != NULL)
		{
			if (MeshComponent == NULL)
			{
				Modify();
				MeshComponent = ConstructObject<UControlPointMeshComponent>(UControlPointMeshComponent::StaticClass(), OuterSplines->GetOuter(), NAME_None, RF_Transactional|RF_TextExportTransient);
				MeshComponent->bSelected = bSelected;
				MeshComponent->AttachTo(OuterSplines);
			}

			if (MeshComponent->StaticMesh != Mesh)
			{
				MeshComponent->SetStaticMesh(Mesh);

				AutoSetConnections(false);
			}

			MeshComponent->RelativeLocation = Location;
			MeshComponent->RelativeRotation = Rotation;
			MeshComponent->RelativeScale3D  = MeshScale;
			MeshComponent->InvalidateLightingCache();
			MeshComponent->RegisterComponent();
		}
		else
		{
			// UpdateNavOctree won't work if we destroy any components, ghosts get left in the navmesh, so we have to unregister
			OuterSplines->GetWorld()->GetNavigationSystem()->UnregisterNavigationRelevantActor(OuterSplines->GetOwner());

			Modify();
			MeshComponent->DestroyComponent();
			MeshComponent = NULL;

			AutoSetConnections(false);
		}
	}

	if (MeshComponent != NULL)
	{
		if (MeshComponent->Materials != MaterialOverrides)
		{
			MeshComponent->Modify();
			MeshComponent->Materials = MaterialOverrides;
			MeshComponent->MarkRenderStateDirty();
			if (MeshComponent->BodyInstance.IsValidBodyInstance())
			{
				MeshComponent->BodyInstance.UpdatePhysicalMaterials();
			}
		}

		if (MeshComponent->CastShadow != bCastShadow)
		{
			MeshComponent->Modify();
			MeshComponent->SetCastShadow(bCastShadow);
		}

		ECollisionEnabled::Type CollisionType = bEnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision;
		if (MeshComponent->BodyInstance.GetCollisionEnabled() != CollisionType)
		{
			MeshComponent->Modify();
			MeshComponent->BodyInstance.SetCollisionEnabled(CollisionType);
			bNavDirty = true;
		}
	}

	if (Mesh != NULL)
	{
		Points.Reset(ConnectedSegments.Num());

		for (auto It = ConnectedSegments.CreateConstIterator(); It; It++)
		{
			const FLandscapeSplineConnection& Connection = (*It);

			FVector StartLocation; FRotator StartRotation;
			GetConnectionLocationAndRotation(Connection.GetNearConnection().SocketName, StartLocation, StartRotation);

			const float Roll = FMath::DegreesToRadians(StartRotation.Roll);
			const FVector Tangent = StartRotation.Vector();
			const FVector BiNormal = FQuat(Tangent, -Roll).RotateVector((Tangent ^ FVector(0, 0, -1)).SafeNormal());
			const FVector LeftPos = StartLocation - BiNormal * Width;
			const FVector RightPos = StartLocation + BiNormal * Width;
			const FVector FalloffLeftPos = StartLocation - BiNormal * (Width + SideFalloff);
			const FVector FalloffRightPos = StartLocation + BiNormal * (Width + SideFalloff);

			new(Points) FLandscapeSplineInterpPoint(StartLocation, LeftPos, RightPos, FalloffLeftPos, FalloffRightPos, 1.0f);
		}

		const FVector CPLocation = Location;
		Points.Sort([&CPLocation](const FLandscapeSplineInterpPoint& x, const FLandscapeSplineInterpPoint& y){return (x.Center - CPLocation).Rotation().Yaw < (y.Center - CPLocation).Rotation().Yaw;});
	}
	else
	{
		Points.Reset(1);

		FVector StartLocation; FRotator StartRotation;
		GetConnectionLocationAndRotation(NAME_None, StartLocation, StartRotation);

		const float Roll = FMath::DegreesToRadians(StartRotation.Roll);
		const FVector Tangent = StartRotation.Vector();
		const FVector BiNormal = FQuat(Tangent, -Roll).RotateVector((Tangent ^ FVector(0, 0, -1)).SafeNormal());
		const FVector LeftPos = StartLocation - BiNormal * Width;
		const FVector RightPos = StartLocation + BiNormal * Width;
		const FVector FalloffLeftPos = StartLocation - BiNormal * (Width + SideFalloff);
		const FVector FalloffRightPos = StartLocation + BiNormal * (Width + SideFalloff);

		new(Points) FLandscapeSplineInterpPoint(StartLocation, LeftPos, RightPos, FalloffLeftPos, FalloffRightPos, 1.0f);
	}

	// Update Bounds
	Bounds = FBox(0);
	for (int32 i = 0; i < Points.Num(); i++)
	{
		Bounds += Points[i].FalloffLeft;
		Bounds += Points[i].FalloffRight;
	}

	OuterSplines->MarkRenderStateDirty();

	if (bNavDirty && bUpdateCollision)
	{
		OuterSplines->GetWorld()->GetNavigationSystem()->UpdateNavOctree(OuterSplines->GetOwner());
		bNavDirty = false;
	}

	for (auto It = ConnectedSegments.CreateConstIterator(); It; It++)
	{
		(*It).Segment->UpdateSplinePoints(bUpdateCollision);
	}
}

void ULandscapeSplineControlPoint::DeleteSplinePoints()
{
	Modify();

	ULandscapeSplinesComponent* OuterSplines = CastChecked<ULandscapeSplinesComponent>(GetOuter());

	Points.Reset();
	Bounds = FBox(0);

	OuterSplines->MarkRenderStateDirty();

	// UpdateNavOctree won't work if we destroy any components, ghosts get left in the navmesh, so we have to unregister
	OuterSplines->GetWorld()->GetNavigationSystem()->UnregisterNavigationRelevantActor(OuterSplines->GetOwner());

	if (MeshComponent != NULL)
	{
		MeshComponent->Modify();
		MeshComponent->UnregisterComponent();
		MeshComponent->DestroyComponent();
		MeshComponent = NULL;
	}

	OuterSplines->GetWorld()->GetNavigationSystem()->UpdateNavOctree(OuterSplines->GetOwner());
}

void ULandscapeSplineControlPoint::PostEditUndo()
{
	// Shouldn't call base, that results in calling UpdateSplinePoints() which will have already been reloaded from the transaction
	//Super::PostEditUndo();

	GetOuterULandscapeSplinesComponent()->MarkRenderStateDirty();
}

void ULandscapeSplineControlPoint::PostDuplicate(bool bDuplicateForPIE)
{
	if (!bDuplicateForPIE)
	{
		ULandscapeSplinesComponent* OuterSplines = CastChecked<ULandscapeSplinesComponent>(GetOuter());

		if (MeshComponent != NULL)
		{
			if (MeshComponent->GetOuter() != OuterSplines->GetOuter())
			{
				MeshComponent = DuplicateObject<UControlPointMeshComponent>(MeshComponent, OuterSplines->GetOuter(), *MeshComponent->GetName());
				MeshComponent->AttachTo(OuterSplines);
			}
		}
	}

	Super::PostDuplicate(bDuplicateForPIE);
}

void ULandscapeSplineControlPoint::PostEditImport()
{
	Super::PostEditImport();

	GetOuterULandscapeSplinesComponent()->ControlPoints.AddUnique(this);
}

void ULandscapeSplineControlPoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	Width = FMath::Max(Width, 0.001f);
	SideFalloff = FMath::Max(SideFalloff, 0.0f);
	EndFalloff = FMath::Max(EndFalloff, 0.0f);

	const bool bUpdateCollision = PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive;
	UpdateSplinePoints(bUpdateCollision);
}
#endif // WITH_EDITOR

//////////////////////////////////////////////////////////////////////////
// SPLINE SEGMENT

ULandscapeSplineSegment::ULandscapeSplineSegment(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{

	Connections[0].ControlPoint = NULL;
	Connections[0].TangentLen = 0;
	Connections[1].ControlPoint = NULL;
	Connections[1].TangentLen = 0;

#if WITH_EDITORONLY_DATA
	LayerName = NAME_None;
	bRaiseTerrain = true;
	bLowerTerrain = true;

	// SplineMesh properties
	SplineMeshes.Empty();
	bEnableCollision = true;
	bCastShadow = true;

	// transients
	bSelected = false;
#endif
}

void ULandscapeSplineSegment::PostInitProperties()
{
	Super::PostInitProperties();

#if WITH_EDITORONLY_DATA
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_NeedLoad | RF_AsyncLoading))
	{
		// create a new random seed for all new objects
		RandomSeed = FMath::Rand();
	}
#endif
}

void ULandscapeSplineSegment::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

#if WITH_EDITOR
	if (Ar.UE4Ver() < VER_UE4_SPLINE_MESH_ORIENTATION)
	{
		for (auto It = SplineMeshes.CreateIterator(); It; It++)
		{
			FLandscapeSplineMeshEntry& MeshEntry = *It;
			switch (MeshEntry.Orientation_DEPRECATED)
			{
			case LSMO_XUp:
				MeshEntry.ForwardAxis = ESplineMeshAxis::Z;
				MeshEntry.UpAxis = ESplineMeshAxis::X;
				break;
			case LSMO_YUp:
				MeshEntry.ForwardAxis = ESplineMeshAxis::Z;
				MeshEntry.UpAxis = ESplineMeshAxis::Y;
				break;
			}
		}
	}
#endif
}

void ULandscapeSplineSegment::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	if (GIsEditor &&
		GetLinkerUE4Version() < VER_UE4_ADDED_LANDSCAPE_SPLINE_EDITOR_MESH &&
		MeshComponents.Num() == 0)
	{
		UpdateSplinePoints();
	}

	if (GIsEditor)
	{
		// Regenerate spline mesh components if any of the meshes used have gone missing
		// Otherwise the spline will have no editor mesh and won't be selectable
		ULandscapeSplinesComponent* OuterSplines = GetOuterULandscapeSplinesComponent();
		for (auto* MeshComponent : MeshComponents)
		{
			MeshComponent->ConditionalPostLoad();
			if (MeshComponent->StaticMesh == NULL)
			{
				UpdateSplinePoints();
				break;
			}
		}
	}

	for (auto It = MeshComponents.CreateConstIterator(); It; It++)
	{
		(*It)->SetFlags(RF_TextExportTransient);
	}
#endif
}

/**  */
#if WITH_EDITOR
void ULandscapeSplineSegment::SetSplineSelected(bool bInSelected)
{
	bSelected = bInSelected;
	GetOuterULandscapeSplinesComponent()->MarkRenderStateDirty();

	for (auto It = MeshComponents.CreateConstIterator(); It; It++)
	{
		(*It)->bSelected = bInSelected;
		(*It)->PushSelectionToProxy();
	}
}

void ULandscapeSplineSegment::AutoFlipTangents()
{
	FVector StartLocation; FRotator StartRotation;
	Connections[0].ControlPoint->GetConnectionLocationAndRotation(Connections[0].SocketName, StartLocation, StartRotation);
	FVector EndLocation; FRotator EndRotation;
	Connections[1].ControlPoint->GetConnectionLocationAndRotation(Connections[1].SocketName, EndLocation, EndRotation);

	// Flipping the tangent is only allowed if not using a socket
	if (Connections[0].SocketName == NAME_None && FVector::DotProduct((EndLocation - StartLocation).SafeNormal() * Connections[0].TangentLen, StartRotation.Vector()) < 0)
	{
		Connections[0].TangentLen = -Connections[0].TangentLen;
	}
	if (Connections[1].SocketName == NAME_None && FVector::DotProduct((StartLocation - EndLocation).SafeNormal() * Connections[1].TangentLen, EndRotation.Vector()) < 0)
	{
		Connections[1].TangentLen = -Connections[1].TangentLen;
	}
}
#endif

void ULandscapeSplineSegment::RegisterComponents()
{
	for (auto It = MeshComponents.CreateConstIterator(); It; It++)
	{
		(*It)->RegisterComponent();
	}
}

void ULandscapeSplineSegment::UnregisterComponents()
{
	for (auto It = MeshComponents.CreateConstIterator(); It; It++)
	{
		(*It)->UnregisterComponent();
	}
}

bool ULandscapeSplineSegment::OwnsComponent(const class USplineMeshComponent* SplineMeshComponent) const
{
	return MeshComponents.FindByKey(SplineMeshComponent) != NULL;
}

static float ApproxLength(const FInterpCurveVector& SplineInfo, const float Start = 0.0f, const float End = 1.0f, const int32 ApproxSections = 4)
{
	float SplineLength = 0;
	FVector OldPos = SplineInfo.Eval(Start, FVector::ZeroVector);
	for (int32 i = 1; i <= ApproxSections; i++)
	{
		FVector NewPos = SplineInfo.Eval(FMath::Lerp(Start, End, (float)i / (float)ApproxSections), FVector::ZeroVector);
		SplineLength += (NewPos - OldPos).Size();
		OldPos = NewPos;
	}

	return SplineLength;
}

/** Util that takes a 2D vector and rotates it by RotAngle (given in radians) */
static FVector2D RotateVec2D(const FVector2D InVec, float RotAngle)
{
	FVector2D OutVec;
	OutVec.X = (InVec.X * FMath::Cos(RotAngle)) - (InVec.Y * FMath::Sin(RotAngle));
	OutVec.Y = (InVec.X * FMath::Sin(RotAngle)) + (InVec.Y * FMath::Cos(RotAngle));
	return OutVec;
}

static const float& GetAxisValue(const FVector& InVector, ESplineMeshAxis::Type InAxis)
{
	switch (InAxis)
	{
	case ESplineMeshAxis::X:
		return InVector.X;
	case ESplineMeshAxis::Y:
		return InVector.Y;
	case ESplineMeshAxis::Z:
		return InVector.Z;
	default:
		check(0);
		return InVector.Z;
	}
}

static float& GetAxisValue(FVector& InVector, ESplineMeshAxis::Type InAxis)
{
	switch (InAxis)
	{
	case ESplineMeshAxis::X:
		return InVector.X;
	case ESplineMeshAxis::Y:
		return InVector.Y;
	case ESplineMeshAxis::Z:
		return InVector.Z;
	default:
		check(0);
		return InVector.Z;
	}
}

static ESplineMeshAxis::Type CrossAxis(ESplineMeshAxis::Type InForwardAxis, ESplineMeshAxis::Type InUpAxis)
{
	check(InForwardAxis != InUpAxis);
	return (ESplineMeshAxis::Type)(3 ^ InForwardAxis ^ InUpAxis);
}

bool FLandscapeSplineMeshEntry::IsValid() const
{
	return Mesh != NULL && ForwardAxis != UpAxis && Scale.GetAbsMin() > KINDA_SMALL_NUMBER;
}

#if WITH_EDITOR
void ULandscapeSplineSegment::UpdateSplinePoints(bool bUpdateCollision)
{
	Modify();

	ULandscapeSplinesComponent* OuterSplines = CastChecked<ULandscapeSplinesComponent>(GetOuter());

	SplineInfo.Points.Empty(2);
	Points.Reset();

	if (Connections[0].ControlPoint == NULL
		|| Connections[1].ControlPoint == NULL)
	{
		return;
	}

	// Set up BSpline
	FVector StartLocation; FRotator StartRotation;
	Connections[0].ControlPoint->GetConnectionLocationAndRotation(Connections[0].SocketName, StartLocation, StartRotation);
	new(SplineInfo.Points) FInterpCurvePoint<FVector>(0.0f, StartLocation, StartRotation.Vector() * Connections[0].TangentLen, StartRotation.Vector() * Connections[0].TangentLen, CIM_CurveUser);
	FVector EndLocation; FRotator EndRotation;
	Connections[1].ControlPoint->GetConnectionLocationAndRotation(Connections[1].SocketName, EndLocation, EndRotation);
	new(SplineInfo.Points) FInterpCurvePoint<FVector>(1.0f, EndLocation, EndRotation.Vector() * -Connections[1].TangentLen, EndRotation.Vector() * -Connections[1].TangentLen, CIM_CurveUser);

	// Pointify

	// Calculate spline length
	const float SplineLength = ApproxLength(SplineInfo, 0.0f, 1.0f, 4);

	float StartFalloffFraction = ((Connections[0].ControlPoint->ConnectedSegments.Num() > 1) ? 0 : (Connections[0].ControlPoint->EndFalloff / SplineLength));
	float EndFalloffFraction = ((Connections[1].ControlPoint->ConnectedSegments.Num() > 1) ? 0 : (Connections[1].ControlPoint->EndFalloff / SplineLength));

	// Stop the start and end fall-off overlapping
	const float TotalFalloff = StartFalloffFraction + EndFalloffFraction;
	if (TotalFalloff > 1.0f)
	{
		StartFalloffFraction /= TotalFalloff;
		EndFalloffFraction /= TotalFalloff;
	}

	const float StartWidth = Connections[0].ControlPoint->Width;
	const float EndWidth = Connections[1].ControlPoint->Width;
	const float StartSideFalloff = Connections[0].ControlPoint->SideFalloff;
	const float EndSideFalloff = Connections[1].ControlPoint->SideFalloff;
	const float StartRollDegrees = StartRotation.Roll * (Connections[0].TangentLen > 0 ? 1 : -1);
	const float EndRollDegrees = EndRotation.Roll * (Connections[1].TangentLen > 0 ? -1 : 1);
	const float StartRoll = FMath::DegreesToRadians(StartRollDegrees);
	const float EndRoll = FMath::DegreesToRadians(EndRollDegrees);

	int32 NumPoints = FMath::Ceil(SplineLength / OuterSplines->SplineResolution);
	NumPoints = FMath::Clamp(NumPoints, 1, 1000);

	float OldKeyTime = 0;
	for(int32 i = 0; i < SplineInfo.Points.Num(); i++)
	{
		const float NewKeyTime = SplineInfo.Points[i].InVal;
		const float NewKeyCosInterp = 0.5f - 0.5f * FMath::Cos(NewKeyTime * PI);
		const float NewKeyWidth = FMath::Lerp(StartWidth, EndWidth, NewKeyCosInterp);
		const float NewKeyFalloff = FMath::Lerp(StartSideFalloff, EndSideFalloff, NewKeyCosInterp);
		const float NewKeyRoll = FMath::Lerp(StartRoll, EndRoll, NewKeyCosInterp);
		const FVector NewKeyPos = SplineInfo.Eval(NewKeyTime, FVector::ZeroVector);
		const FVector NewKeyTangent = SplineInfo.EvalDerivative(NewKeyTime, FVector::ZeroVector).SafeNormal();
		const FVector NewKeyBiNormal = FQuat(NewKeyTangent, -NewKeyRoll).RotateVector((NewKeyTangent ^ FVector(0, 0, -1)).SafeNormal());
		const FVector NewKeyLeftPos = NewKeyPos - NewKeyBiNormal * NewKeyWidth;
		const FVector NewKeyRightPos = NewKeyPos + NewKeyBiNormal * NewKeyWidth;
		const FVector NewKeyFalloffLeftPos = NewKeyPos - NewKeyBiNormal * (NewKeyWidth + NewKeyFalloff);
		const FVector NewKeyFalloffRightPos = NewKeyPos + NewKeyBiNormal * (NewKeyWidth + NewKeyFalloff);
		const float NewKeyStartEndFalloff = FMath::Min((StartFalloffFraction > 0 ? NewKeyTime / StartFalloffFraction : 1.0f), (EndFalloffFraction > 0 ? (1 - NewKeyTime) / EndFalloffFraction : 1.0f));

		// If not the first keypoint, interp from the last keypoint.
		if(i > 0)
		{
			int32 NumSteps = FMath::Ceil( (NewKeyTime - OldKeyTime) * NumPoints );
			float DrawSubstep = (NewKeyTime - OldKeyTime) / NumSteps;

			// Add a point for each substep, except the ends because that's the point added outside the interp'ing.
			for(int32 j = 1; j < NumSteps; j++)
			{
				const float NewTime = OldKeyTime + j*DrawSubstep;
				const float NewCosInterp = 0.5f - 0.5f * FMath::Cos(NewTime * PI);
				const float NewWidth = FMath::Lerp(StartWidth, EndWidth, NewCosInterp);
				const float NewFalloff = FMath::Lerp(StartSideFalloff, EndSideFalloff, NewCosInterp);
				const float NewRoll = FMath::Lerp(StartRoll, EndRoll, NewCosInterp);
				const FVector NewPos = SplineInfo.Eval(NewTime, FVector::ZeroVector);
				const FVector NewTangent = SplineInfo.EvalDerivative(NewTime, FVector::ZeroVector).SafeNormal();
				const FVector NewBiNormal = FQuat(NewTangent, -NewRoll).RotateVector((NewTangent ^ FVector(0, 0, -1)).SafeNormal());
				const FVector NewLeftPos = NewPos - NewBiNormal * NewWidth;
				const FVector NewRightPos = NewPos + NewBiNormal * NewWidth;
				const FVector NewFalloffLeftPos = NewPos - NewBiNormal * (NewWidth + NewFalloff);
				const FVector NewFalloffRightPos = NewPos + NewBiNormal * (NewWidth + NewFalloff);
				const float NewStartEndFalloff = FMath::Min((StartFalloffFraction > 0 ? NewTime / StartFalloffFraction : 1.0f), (EndFalloffFraction > 0 ? (1 - NewTime) / EndFalloffFraction : 1.0f));

				new(Points) FLandscapeSplineInterpPoint(NewPos, NewLeftPos, NewRightPos, NewFalloffLeftPos, NewFalloffRightPos, NewStartEndFalloff);
			}
		}

		new(Points) FLandscapeSplineInterpPoint(NewKeyPos, NewKeyLeftPos, NewKeyRightPos, NewKeyFalloffLeftPos, NewKeyFalloffRightPos, NewKeyStartEndFalloff);

		OldKeyTime = NewKeyTime;
	}

	// Handle self-intersection errors due to tight turns
	FixSelfIntersection(&FLandscapeSplineInterpPoint::Left);
	FixSelfIntersection(&FLandscapeSplineInterpPoint::Right);
	FixSelfIntersection(&FLandscapeSplineInterpPoint::FalloffLeft);
	FixSelfIntersection(&FLandscapeSplineInterpPoint::FalloffRight);

	// Update Bounds
	Bounds = FBox(0);
	for (int32 i = 0; i < Points.Num(); i++)
	{
		Bounds += Points[i].FalloffLeft;
		Bounds += Points[i].FalloffRight;
	}

	OuterSplines->MarkRenderStateDirty();

	// Spline mesh components
	TArray<const FLandscapeSplineMeshEntry*> UsableMeshes;
	UsableMeshes.Reserve(SplineMeshes.Num());
	for (int32 i = 0; i < SplineMeshes.Num(); i++)
	{
		const FLandscapeSplineMeshEntry* MeshEntry = &SplineMeshes[i];
		if (MeshEntry->IsValid())
		{
			UsableMeshes.Add(MeshEntry);
		}
	}

	// Editor mesh
	FLandscapeSplineMeshEntry SplineEditorMeshEntry;
	SplineEditorMeshEntry.Mesh = OuterSplines->SplineEditorMesh;
	SplineEditorMeshEntry.Scale.X = 3;
	SplineEditorMeshEntry.Offset.Y = 0.5f;
	bool bUsingEditorMesh = false;
	if (UsableMeshes.Num() == 0 && OuterSplines->SplineEditorMesh != NULL)
	{
		bUsingEditorMesh = true;
		UsableMeshes.Add(&SplineEditorMeshEntry);
	}

	OuterSplines->Modify();
	for (auto It = MeshComponents.CreateConstIterator(); It; It++)
	{
		(*It)->Modify();
	}
	UnregisterComponents();

	if (SplineLength > 0 && (StartWidth > 0 || EndWidth > 0) && UsableMeshes.Num() > 0)
	{
		float T = 0;
		int32 iMesh = 0;

		struct FMeshSettings
		{
			const float T;
			const FLandscapeSplineMeshEntry* MeshEntry;

			FMeshSettings(float InT, const FLandscapeSplineMeshEntry* InMeshEntry) :
				T(InT), MeshEntry(InMeshEntry) { }
		};
		TArray<FMeshSettings> MeshSettings;
		MeshSettings.Reserve(21);

		FRandomStream Random(RandomSeed);

		// First pass:
		// Choose meshes, create components, calculate lengths
		while (T < 1.0f && iMesh < 20) // Max 20 meshes per spline segment
		{
			const float CosInterp = 0.5f - 0.5f * FMath::Cos(T * PI);
			const float Width = FMath::Lerp(StartWidth, EndWidth, CosInterp);

			const FLandscapeSplineMeshEntry* MeshEntry = UsableMeshes[Random.RandHelper(UsableMeshes.Num())];
			UStaticMesh* Mesh = MeshEntry->Mesh;
			const FBoxSphereBounds MeshBounds = Mesh->GetBounds();

			FVector Scale = MeshEntry->Scale;
			if (MeshEntry->bScaleToWidth)
			{
				Scale *= Width / GetAxisValue(MeshBounds.BoxExtent, CrossAxis(MeshEntry->ForwardAxis, MeshEntry->UpAxis));
			}

			const float MeshLength = FMath::Abs(GetAxisValue(MeshBounds.BoxExtent, MeshEntry->ForwardAxis) * 2 * GetAxisValue(Scale, MeshEntry->ForwardAxis));
			float MeshT = (MeshLength / SplineLength);

			// Improve our approximation if we're not going off the end of the spline
			if (T + MeshT <= 1.0f)
			{
				MeshT *= (MeshLength / ApproxLength(SplineInfo, T, T + MeshT, 4));
				MeshT *= (MeshLength / ApproxLength(SplineInfo, T, T + MeshT, 4));
			}

			// If it's smaller to round up than down, don't add another component
			if (iMesh != 0 && (1.0f - T) < (T + MeshT - 1.0f))
			{
				break;
			}

			USplineMeshComponent* MeshComponent = NULL;
			if (iMesh < MeshComponents.Num())
			{
				MeshComponent = MeshComponents[iMesh];
			}
			else
			{
				MeshComponent = ConstructObject<USplineMeshComponent>(USplineMeshComponent::StaticClass(), OuterSplines->GetOuter(), NAME_None, RF_Transactional|RF_TextExportTransient);
				MeshComponent->bSelected = bSelected;
				MeshComponent->AttachTo(OuterSplines);
				MeshComponents.Add(MeshComponent);
			}

			// if the spline mesh isn't the editor mesh or wasn't the editor mesh
			if (!bUsingEditorMesh || (MeshComponent->StaticMesh != NULL && !MeshComponent->bHiddenInGame))
			{
				// either collision is enabled or the mesh component had collision enabled before
				if (bEnableCollision || MeshComponent->BodyInstance.GetCollisionEnabled() != ECollisionEnabled::NoCollision)
				{
					bNavDirty = true;
				}
			}

			MeshComponent->SetStaticMesh(Mesh);

			MeshComponent->Materials = MeshEntry->MaterialOverrides;
			MeshComponent->MarkRenderStateDirty();
			if (MeshComponent->BodyInstance.IsValidBodyInstance())
			{
				MeshComponent->BodyInstance.UpdatePhysicalMaterials();
			}

			MeshComponent->SetHiddenInGame(bUsingEditorMesh);
			MeshComponent->SetVisibility(!bUsingEditorMesh || OuterSplines->bShowSplineEditorMesh);

			MeshSettings.Add(FMeshSettings(T, MeshEntry));
			iMesh++;
			T += MeshT;
		}
		MeshSettings.Add(FMeshSettings(T, NULL));

		if (iMesh < MeshComponents.Num())
		{
			// UpdateNavOctree won't work if we destroy any components, ghosts get left in the navmesh, so we have to unregister
			OuterSplines->GetWorld()->GetNavigationSystem()->UnregisterNavigationRelevantActor(OuterSplines->GetOwner());

			for (int32 i = iMesh; i < MeshComponents.Num(); i++)
			{
				//MeshComponents[i]->Modify();
				MeshComponents[i]->DestroyComponent();
			}

			MeshComponents.RemoveAt(iMesh, MeshComponents.Num() - iMesh);

			bNavDirty = true;
		}

		// Second pass:
		// Rescale components to fit a whole number to the spline, set up final parameters
		const float Rescale = 1.0f / T;
		for (int32 i = 0; i < MeshComponents.Num(); i++)
		{
			USplineMeshComponent* const MeshComponent = MeshComponents[i];
			const UStaticMesh* const Mesh = MeshComponent->StaticMesh;
			const FBoxSphereBounds MeshBounds = Mesh->GetBounds();

			const float T = MeshSettings[i].T * Rescale;
			const FLandscapeSplineMeshEntry* MeshEntry = MeshSettings[i].MeshEntry;
			const ESplineMeshAxis::Type SideAxis = CrossAxis(MeshEntry->ForwardAxis, MeshEntry->UpAxis);

			const float TEnd = MeshSettings[i + 1].T * Rescale;

			const float CosInterp = 0.5f - 0.5f * FMath::Cos(T * PI);
			const float Width = FMath::Lerp(StartWidth, EndWidth, CosInterp);
			const bool bDoOrientationRoll = (MeshEntry->ForwardAxis == ESplineMeshAxis::X && MeshEntry->UpAxis == ESplineMeshAxis::Y) ||
			                                (MeshEntry->ForwardAxis == ESplineMeshAxis::Y && MeshEntry->UpAxis == ESplineMeshAxis::Z) ||
			                                (MeshEntry->ForwardAxis == ESplineMeshAxis::Z && MeshEntry->UpAxis == ESplineMeshAxis::X);
			const float Roll = FMath::Lerp(StartRoll, EndRoll, CosInterp) + (bDoOrientationRoll ? -HALF_PI : 0);

			FVector Scale = MeshEntry->Scale;
			if (MeshEntry->bScaleToWidth)
			{
				Scale *= Width / GetAxisValue(MeshBounds.BoxExtent, SideAxis);
			}

			FVector2D Offset = MeshEntry->Offset;
			if (MeshEntry->bCenterH)
			{
				if (bDoOrientationRoll)
				{
					Offset.Y -= GetAxisValue(MeshBounds.Origin, SideAxis);
				}
				else
				{
					Offset.X -= GetAxisValue(MeshBounds.Origin, SideAxis);
				}
			}

			FVector2D Scale2D;
			switch (MeshEntry->ForwardAxis)
			{
			case ESplineMeshAxis::X:
				Scale2D = FVector2D(Scale.Y, Scale.Z);
				break;
			case ESplineMeshAxis::Y:
				Scale2D = FVector2D(Scale.Z, Scale.X);
				break;
			case ESplineMeshAxis::Z:
				Scale2D = FVector2D(Scale.X, Scale.Y);
				break;
			default:
				check(0);
				break;
			}
			Offset *= Scale2D;
			Offset = RotateVec2D(Offset, -Roll);

			MeshComponent->SplineParams.StartPos = SplineInfo.Eval(T, FVector::ZeroVector);
			MeshComponent->SplineParams.StartTangent = SplineInfo.EvalDerivative(T, FVector::ZeroVector) * (TEnd - T);
			MeshComponent->SplineParams.StartScale = Scale2D;
			MeshComponent->SplineParams.StartRoll = Roll;
			MeshComponent->SplineParams.StartOffset = Offset;

			const float CosInterpEnd = 0.5f - 0.5f * FMath::Cos(TEnd * PI);
			const float WidthEnd = FMath::Lerp(StartWidth, EndWidth, CosInterpEnd);
			const float RollEnd = FMath::Lerp(StartRoll, EndRoll, CosInterpEnd) + (bDoOrientationRoll ? -HALF_PI : 0);

			FVector ScaleEnd = MeshEntry->Scale;
			if (MeshEntry->bScaleToWidth)
			{
				ScaleEnd *= WidthEnd / GetAxisValue(MeshBounds.BoxExtent, SideAxis);
			}

			FVector2D OffsetEnd = MeshEntry->Offset;
			if (MeshEntry->bCenterH)
			{
				if (bDoOrientationRoll)
				{
					OffsetEnd.Y -= GetAxisValue(MeshBounds.Origin, SideAxis);
				}
				else
				{
					OffsetEnd.X -= GetAxisValue(MeshBounds.Origin, SideAxis);
				}
			}

			FVector2D Scale2DEnd;
			switch (MeshEntry->ForwardAxis)
			{
			case ESplineMeshAxis::X:
				Scale2DEnd = FVector2D(ScaleEnd.Y, ScaleEnd.Z);
				break;
			case ESplineMeshAxis::Y:
				Scale2DEnd = FVector2D(ScaleEnd.Z, ScaleEnd.X);
				break;
			case ESplineMeshAxis::Z:
				Scale2DEnd = FVector2D(ScaleEnd.X, ScaleEnd.Y);
				break;
			default:
				check(0);
				break;
			}
			OffsetEnd *= Scale2DEnd;
			OffsetEnd = RotateVec2D(OffsetEnd, -RollEnd);

			MeshComponent->SplineParams.EndPos = SplineInfo.Eval(TEnd, FVector::ZeroVector);
			MeshComponent->SplineParams.EndTangent = SplineInfo.EvalDerivative(TEnd, FVector::ZeroVector) * (TEnd - T);
			MeshComponent->SplineParams.EndScale = Scale2DEnd;
			MeshComponent->SplineParams.EndRoll = RollEnd;
			MeshComponent->SplineParams.EndOffset = OffsetEnd;

			MeshComponent->SplineXDir = FVector(0,0,1); // Up, to be consistent between joined meshes. We rotate it to horizontal using roll if using Z Forward X Up or X Forward Y Up
			MeshComponent->ForwardAxis = MeshEntry->ForwardAxis;

			if (GetAxisValue(MeshEntry->Scale, MeshEntry->ForwardAxis) < 0)
			{
				Swap(MeshComponent->SplineParams.StartPos, MeshComponent->SplineParams.EndPos);
				Swap(MeshComponent->SplineParams.StartTangent, MeshComponent->SplineParams.EndTangent);
				Swap(MeshComponent->SplineParams.StartScale, MeshComponent->SplineParams.EndScale);
				Swap(MeshComponent->SplineParams.StartRoll, MeshComponent->SplineParams.EndRoll);
				Swap(MeshComponent->SplineParams.StartOffset, MeshComponent->SplineParams.EndOffset);

				MeshComponent->SplineParams.StartTangent = -MeshComponent->SplineParams.StartTangent;
				MeshComponent->SplineParams.EndTangent = -MeshComponent->SplineParams.EndTangent;
				MeshComponent->SplineParams.StartScale.X = -MeshComponent->SplineParams.StartScale.X;
				MeshComponent->SplineParams.EndScale.X = -MeshComponent->SplineParams.EndScale.X;
				MeshComponent->SplineParams.StartRoll = -MeshComponent->SplineParams.StartRoll;
				MeshComponent->SplineParams.EndRoll = -MeshComponent->SplineParams.EndRoll;
				MeshComponent->SplineParams.StartOffset.X = -MeshComponent->SplineParams.StartOffset.X;
				MeshComponent->SplineParams.EndOffset.X = -MeshComponent->SplineParams.EndOffset.X;
			}

			MeshComponent->SetCastShadow(bCastShadow);
			MeshComponent->InvalidateLightingCache();

			MeshComponent->BodyInstance.bEnableCollision_DEPRECATED = bEnableCollision && !bUsingEditorMesh;
			MeshComponent->BodyInstance.SetCollisionEnabled((bEnableCollision && !bUsingEditorMesh) ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

#if WITH_EDITOR
			if (bUpdateCollision)
			{
				MeshComponent->RecreateCollision();
			}
			else
			{
				if (MeshComponent->BodySetup)
				{
					MeshComponent->BodySetup->InvalidatePhysicsData();
					MeshComponent->BodySetup->AggGeom.EmptyElements();
				}
			}
#endif
		}

		// Finally, register components
		RegisterComponents();
	}
	else
	{
		if (MeshComponents.Num() > 0)
		{
			// UpdateNavOctree won't work if we destroy any components, ghosts get left in the navmesh, so we have to unregister
			OuterSplines->GetWorld()->GetNavigationSystem()->UnregisterNavigationRelevantActor(OuterSplines->GetOwner());

			for (auto It = MeshComponents.CreateConstIterator(); It; It++)
			{
				(*It)->DestroyComponent();
			}
			MeshComponents.Empty();
			bNavDirty = true;
		}
	}

	if (bNavDirty && bUpdateCollision)
	{
		OuterSplines->GetWorld()->GetNavigationSystem()->UpdateNavOctree(OuterSplines->GetOwner());
		bNavDirty = false;
	}
}

void ULandscapeSplineSegment::UpdateSplineEditorMesh()
{
	ULandscapeSplinesComponent* OuterSplines = CastChecked<ULandscapeSplinesComponent>(GetOuter());

	for (auto It = MeshComponents.CreateConstIterator(); It; ++It)
	{
		USplineMeshComponent* MeshComponent = (*It);
		if (MeshComponent->bHiddenInGame)
		{
			MeshComponent->SetVisibility(OuterSplines->bShowSplineEditorMesh);
		}
	}
}

void ULandscapeSplineSegment::DeleteSplinePoints()
{
	Modify();

	ULandscapeSplinesComponent* OuterSplines = CastChecked<ULandscapeSplinesComponent>(GetOuter());

	SplineInfo.Reset();
	Points.Reset();
	Bounds = FBox(0);

	OuterSplines->MarkRenderStateDirty();

	// UpdateNavOctree won't work if we destroy any components, ghosts get left in the navmesh, so we have to unregister
	OuterSplines->GetWorld()->GetNavigationSystem()->UnregisterNavigationRelevantActor(OuterSplines->GetOwner());

	for (auto It = MeshComponents.CreateConstIterator(); It; It++)
	{
		USplineMeshComponent* MeshComponent = (*It);

		MeshComponent->Modify();
		MeshComponent->UnregisterComponent();
		MeshComponent->DestroyComponent();
	}
	MeshComponents.Empty();

	OuterSplines->GetWorld()->GetNavigationSystem()->UpdateNavOctree(OuterSplines->GetOwner());
}

static bool LineIntersect( const FVector2D& L1Start, const FVector2D& L1End, const FVector2D& L2Start, const FVector2D& L2End, FVector2D& Intersect, float Tolerance = KINDA_SMALL_NUMBER)
{
	float tA = (L2End - L2Start) ^ (L2Start - L1Start);
	float tB = (L1End - L1Start) ^ (L2Start - L1Start);
	float Denom = (L2End - L2Start) ^ (L1End - L1Start);

	if( FMath::IsNearlyZero( tA ) && FMath::IsNearlyZero( tB )  )
	{
		// Lines are the same
		Intersect = (L2Start + L2End) / 2;
		return true;
	}

	if( FMath::IsNearlyZero(Denom) )
	{
		// Lines are parallel
		Intersect = (L2Start + L2End) / 2;
		return false;
	}

	tA /= Denom;
	tB /= Denom;

	Intersect = L1Start + tA * (L1End - L1Start);

	if( tA >= -Tolerance && tA <= (1.0f + Tolerance ) && tB >= -Tolerance && tB <= (1.0f + Tolerance) )
	{
		return true;
	}

	return false;
}

bool ULandscapeSplineSegment::FixSelfIntersection(FVector FLandscapeSplineInterpPoint::* Side)
{
	int32 StartSide = INDEX_NONE;
	for(int32 i = 0; i < Points.Num(); i++)
	{
		bool bReversed = false;

		if (i < Points.Num() - 1)
		{
			const FLandscapeSplineInterpPoint& CurrentPoint = Points[i];
			const FLandscapeSplineInterpPoint& NextPoint = Points[i+1];
			const FVector Direction = (NextPoint.Center - CurrentPoint.Center).SafeNormal();
			const FVector SideDirection = (NextPoint.*Side - CurrentPoint.*Side).SafeNormal();
			bReversed = (SideDirection | Direction) < 0;
		}

		if (bReversed)
		{
			if (StartSide == INDEX_NONE)
			{
				StartSide = i;
			}
		}
		else
		{
			if (StartSide != INDEX_NONE)
			{
				int32 EndSide = i;

				// step startSide back until before the endSide point
				while (StartSide > 0)
				{
					const float Projection = (Points[StartSide].*Side - Points[StartSide-1].*Side) | (Points[EndSide].*Side - Points[StartSide-1].*Side);
					if (Projection >= 0)
					{
						break;
					}
					StartSide--;
				}
				// step endSide forwards until after the startSide point
				while (EndSide < Points.Num() - 1)
				{
					const float Projection = (Points[EndSide].*Side - Points[EndSide+1].*Side) | (Points[StartSide].*Side - Points[EndSide+1].*Side);
					if (Projection >= 0)
					{
						break;
					}
					EndSide++;
				}

				// Can't do anything if the start and end intersect, as they're both unalterable
				if (StartSide == 0 && EndSide == Points.Num() - 1)
				{
					return false;
				}

				FVector2D Collapse;
				if (StartSide == 0)
				{
					Collapse = FVector2D(Points[StartSide].*Side);
					StartSide++;
				}
				else if (EndSide == Points.Num() - 1)
				{
					Collapse = FVector2D(Points[EndSide].*Side);
					EndSide--;
				}
				else
				{
					LineIntersect(FVector2D(Points[StartSide-1].*Side), FVector2D(Points[StartSide].*Side),
						FVector2D(Points[EndSide+1].*Side), FVector2D(Points[EndSide].*Side), Collapse);
				}

				for (int32 j = StartSide; j <= EndSide; j++)
				{
					(Points[j].*Side).X = Collapse.X;
					(Points[j].*Side).Y = Collapse.Y;
				}

				StartSide = INDEX_NONE;
				i = EndSide;
			}
		}
	}

	return true;
}
#endif

void ULandscapeSplineSegment::FindNearest( const FVector& InLocation, float& t, FVector& OutLocation, FVector& OutTangent )
{
	float TempOutDistanceSq;
	t = SplineInfo.InaccurateFindNearest(InLocation, TempOutDistanceSq);
	OutLocation = SplineInfo.Eval(t, FVector::ZeroVector);
	OutTangent = SplineInfo.EvalDerivative(t, FVector::ZeroVector);
}

bool ULandscapeSplineSegment::Modify(bool bAlwaysMarkDirty /*= true*/)
{
	bool bSavedToTransactionBuffer = Super::Modify(bAlwaysMarkDirty);

	for (auto MeshComponentIt = MeshComponents.CreateConstIterator(); MeshComponentIt; MeshComponentIt++)
	{
		if (*MeshComponentIt)
		{
			bSavedToTransactionBuffer = (*MeshComponentIt)->Modify(bAlwaysMarkDirty) || bSavedToTransactionBuffer;
		}
	}

	return bSavedToTransactionBuffer;
}

#if WITH_EDITOR
void ULandscapeSplineSegment::PostEditUndo()
{
	// Shouldn't call base, that results in calling UpdateSplinePoints() which will have already been reloaded from the transaction
	//Super::PostEditUndo();

	RegisterComponents();

	GetOuterULandscapeSplinesComponent()->MarkRenderStateDirty();
}

void ULandscapeSplineSegment::PostDuplicate(bool bDuplicateForPIE)
{
	if (!bDuplicateForPIE)
	{
		ULandscapeSplinesComponent* OuterSplines = CastChecked<ULandscapeSplinesComponent>(GetOuter());

		for (auto It = MeshComponents.CreateIterator(); It; ++It)
		{
			USplineMeshComponent*& MeshComponent = (*It);
			if (MeshComponent->GetOuter() != OuterSplines->GetOuter())
			{
				MeshComponent = DuplicateObject<USplineMeshComponent>(MeshComponent, OuterSplines->GetOuter(), *MeshComponent->GetName());
				MeshComponent->AttachTo(OuterSplines);
			}
		}
	}

	Super::PostDuplicate(bDuplicateForPIE);
}

void ULandscapeSplineSegment::PostEditImport()
{
	Super::PostEditImport();

	GetOuterULandscapeSplinesComponent()->Segments.AddUnique(this);

	if (Connections[0].ControlPoint != NULL)
	{
		Connections[0].ControlPoint->ConnectedSegments.AddUnique(FLandscapeSplineConnection(this, 0));
		Connections[1].ControlPoint->ConnectedSegments.AddUnique(FLandscapeSplineConnection(this, 1));
	}
}

void ULandscapeSplineSegment::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Flipping the tangent is only allowed if not using a socket
	if (Connections[0].SocketName != NAME_None)
	{
		Connections[0].TangentLen = FMath::Abs(Connections[0].TangentLen);
	}
	if (Connections[1].SocketName != NAME_None)
	{
		Connections[1].TangentLen = FMath::Abs(Connections[1].TangentLen);
	}

	const bool bUpdateCollision = PropertyChangedEvent.ChangeType != EPropertyChangeType::Interactive;
	UpdateSplinePoints(bUpdateCollision);
}
#endif // WITH_EDITOR


/** A vertex factory for spline-deformed static meshes */
struct FSplineMeshVertexFactory : public FLocalVertexFactory
{
	DECLARE_VERTEX_FACTORY_TYPE(FSplineMeshVertexFactory);
public:

	FSplineMeshVertexFactory(class FSplineMeshSceneProxy* InSplineProxy) :
		SplineSceneProxy(InSplineProxy)
	{
	}

	/** Should we cache the material's shadertype on this platform with this vertex factory? */
	static bool ShouldCache(EShaderPlatform Platform, const class FMaterial* Material, const class FShaderType* ShaderType)
	{
		return (Material->IsUsedWithSplineMeshes() || Material->IsSpecialEngineMaterial())
			&& FLocalVertexFactory::ShouldCache(Platform, Material, ShaderType)
			&& IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM3);
	}

	/** Modify compile environment to enable spline deformation */
	static void ModifyCompilationEnvironment(EShaderPlatform Platform, const FMaterial* Material, FShaderCompilerEnvironment& OutEnvironment)
	{
		FLocalVertexFactory::ModifyCompilationEnvironment(Platform, Material, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("USE_SPLINEDEFORM"),TEXT("1"));
	}

	/** Copy the data from another vertex factory */
	void Copy(const FSplineMeshVertexFactory& Other)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FSplineMeshVertexFactoryCopyData,
			FSplineMeshVertexFactory*,VertexFactory,this,
			const DataType*,DataCopy,&Other.Data,
		{
			VertexFactory->Data = *DataCopy;
		});
		BeginUpdateResourceRHI(this);
	}

	// FRenderResource interface.
	virtual void InitRHI()
	{
		FLocalVertexFactory::InitRHI();
	}

	static FVertexFactoryShaderParameters* ConstructShaderParameters(EShaderFrequency ShaderFrequency);

	class FSplineMeshSceneProxy* SplineSceneProxy;

private:
};



/** Factory specific params */
class FSplineMeshVertexFactoryShaderParameters : public FVertexFactoryShaderParameters
{
	void Bind(const FShaderParameterMap& ParameterMap) OVERRIDE;

	void SetMesh(FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const OVERRIDE;

	void Serialize(FArchive& Ar) OVERRIDE
	{
		Ar << SplineStartPosParam;
		Ar << SplineStartTangentParam;
		Ar << SplineStartRollParam;
		Ar << SplineStartScaleParam;
		Ar << SplineStartOffsetParam;
		
		Ar << SplineEndPosParam;
		Ar << SplineEndTangentParam;
		Ar << SplineEndRollParam;
		Ar << SplineEndScaleParam;
		Ar << SplineEndOffsetParam;

		Ar << SplineUpDirParam;
		Ar << SmoothInterpRollScaleParam;

		Ar << SplineMeshMinZParam;
		Ar << SplineMeshScaleZParam;

		Ar << SplineMeshDirParam;
		Ar << SplineMeshXParam;
		Ar << SplineMeshYParam;
	}

	virtual uint32 GetSize() const OVERRIDE
	{
		return sizeof(*this);
	}

private:
	FShaderParameter SplineStartPosParam;
	FShaderParameter SplineStartTangentParam;
	FShaderParameter SplineStartRollParam;
	FShaderParameter SplineStartScaleParam;
	FShaderParameter SplineStartOffsetParam;
	
	FShaderParameter SplineEndPosParam;
	FShaderParameter SplineEndTangentParam;
	FShaderParameter SplineEndRollParam;
	FShaderParameter SplineEndScaleParam;
	FShaderParameter SplineEndOffsetParam;
	
	FShaderParameter SplineUpDirParam;
	FShaderParameter SmoothInterpRollScaleParam;
	
	FShaderParameter SplineMeshMinZParam;
	FShaderParameter SplineMeshScaleZParam;

	FShaderParameter SplineMeshDirParam;
	FShaderParameter SplineMeshXParam;
	FShaderParameter SplineMeshYParam;
};

/** Scene proxy for SplineMesh instance */
class FSplineMeshSceneProxy : public FStaticMeshSceneProxy
{
protected:
	struct FLODResources
	{
		/** Pointer to vertex factory object */
		FSplineMeshVertexFactory* VertexFactory;

		FLODResources(FSplineMeshVertexFactory* InVertexFactory) :
			VertexFactory(InVertexFactory)
		{
		}
	};
public:

	FSplineMeshSceneProxy(USplineMeshComponent* InComponent):
		FStaticMeshSceneProxy(InComponent)
	{
		// make sure all the materials are okay to be rendered as a spline mesh
		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			FStaticMeshSceneProxy::FLODInfo& LODInfo = LODs[LODIndex];
			for (int32 SectionIndex = 0; SectionIndex < LODInfo.Sections.Num(); SectionIndex++)
			{
				FStaticMeshSceneProxy::FLODInfo::FSectionInfo& Section = LODInfo.Sections[SectionIndex];
				if (!Section.Material->CheckMaterialUsage(MATUSAGE_SplineMesh))
				{
					Section.Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}
			}
		}

		// Copy spline params from component
		SplineParams = InComponent->SplineParams;
		SplineXDir = InComponent->SplineXDir;
		bSmoothInterpRollScale = InComponent->bSmoothInterpRollScale;
		ForwardAxis = InComponent->ForwardAxis;

		// Fill in info about the mesh
		FBoxSphereBounds StaticMeshBounds = StaticMesh->GetBounds();
		SplineMeshScaleZ = 0.5f / GetAxisValue(StaticMeshBounds.BoxExtent, ForwardAxis); // 1/(2 * Extent)
		SplineMeshMinZ = GetAxisValue(StaticMeshBounds.Origin, ForwardAxis) * SplineMeshScaleZ - 0.5f;

		LODResources.Reset(InComponent->StaticMesh->RenderData->LODResources.Num());

		for (int32 LODIndex = 0; LODIndex < LODs.Num(); LODIndex++)
		{
			FSplineMeshVertexFactory* VertexFactory = new FSplineMeshVertexFactory(this);

			LODResources.Add(VertexFactory);

			InitResources(InComponent, LODIndex);
		}
	}

	virtual ~FSplineMeshSceneProxy() OVERRIDE
	{
		ReleaseResources();

		for (int32 LODIndex = 0; LODIndex < LODResources.Num(); LODIndex++)
		{
			delete LODResources[LODIndex].VertexFactory;
		}
		LODResources.Empty();
	}

	void InitResources(USplineMeshComponent* InComponent, int32 InLODIndex);

	void ReleaseResources();

	/** Sets up a shadow FMeshBatch for a specific LOD. */
	virtual bool GetShadowMeshElement(int32 LODIndex, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshElement) const OVERRIDE
	{
		//checkf(LODIndex == 0, TEXT("Getting spline static mesh element with invalid LOD [%d]"), LODIndex);

		if (FStaticMeshSceneProxy::GetShadowMeshElement(LODIndex, InDepthPriorityGroup, OutMeshElement))
		{
			OutMeshElement.VertexFactory = LODResources[LODIndex].VertexFactory;
			OutMeshElement.ReverseCulling ^= (SplineParams.StartScale.X < 0) ^ (SplineParams.StartScale.Y < 0);
			return true;
		}
		return false;
	}

	/** Sets up a FMeshBatch for a specific LOD and element. */
	virtual bool GetMeshElement(int32 LODIndex,int32 SectionIndex,uint8 InDepthPriorityGroup,FMeshBatch& OutMeshElement, const bool bUseSelectedMaterial, const bool bUseHoveredMaterial) const OVERRIDE
	{
		//checkf(LODIndex == 0 /*&& SectionIndex == 0*/, TEXT("Getting spline static mesh element with invalid params [%d, %d]"), LODIndex, SectionIndex);

		if (FStaticMeshSceneProxy::GetMeshElement(LODIndex, SectionIndex, InDepthPriorityGroup, OutMeshElement, bUseSelectedMaterial, bUseHoveredMaterial))
		{
			OutMeshElement.VertexFactory = LODResources[LODIndex].VertexFactory;
			OutMeshElement.ReverseCulling ^= (SplineParams.StartScale.X < 0) ^ (SplineParams.StartScale.Y < 0);
			return true;
		}
		return false;
	}

	/** Sets up a wireframe FMeshBatch for a specific LOD. */
	virtual bool GetWireframeMeshElement(int32 LODIndex, const FMaterialRenderProxy* WireframeRenderProxy, uint8 InDepthPriorityGroup, FMeshBatch& OutMeshElement) const OVERRIDE
	{
		//checkf(LODIndex == 0, TEXT("Getting spline static mesh element with invalid LOD [%d]"), LODIndex);

		if (FStaticMeshSceneProxy::GetWireframeMeshElement(LODIndex, WireframeRenderProxy, InDepthPriorityGroup, OutMeshElement))
		{
			OutMeshElement.VertexFactory = LODResources[LODIndex].VertexFactory;
			OutMeshElement.ReverseCulling ^= (SplineParams.StartScale.X < 0) ^ (SplineParams.StartScale.Y < 0);
			return true;
		}
		return false;
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) OVERRIDE
	{
		return FStaticMeshSceneProxy::GetViewRelevance(View);
	}

	// 	  virtual uint32 GetMemoryFootprint( void ) const { return 0; }

	/** Parameters that define the spline, used to deform mesh */
	FSplineMeshParams SplineParams;
	/** Axis (in component space) that is used to determine X axis for co-ordinates along spline */
	FVector SplineXDir;
	/** Smoothly (cubic) interpolate the Roll and Scale params over spline. */
	bool bSmoothInterpRollScale;
	/** Chooses the forward axis for the spline mesh orientation */
	ESplineMeshAxis::Type ForwardAxis;
	
	/** Minimum Z value of the entire mesh */
	float SplineMeshMinZ;
	/** Range of Z values over entire mesh */
	float SplineMeshScaleZ;

protected:
	TArray<FLODResources> LODResources;
};

FVertexFactoryShaderParameters* FSplineMeshVertexFactory::ConstructShaderParameters(EShaderFrequency ShaderFrequency)
{
	return ShaderFrequency == SF_Vertex ? new FSplineMeshVertexFactoryShaderParameters() : NULL;
}

IMPLEMENT_VERTEX_FACTORY_TYPE(FSplineMeshVertexFactory, "LocalVertexFactory", true, true, true, true, true);


void FSplineMeshVertexFactoryShaderParameters::Bind(const FShaderParameterMap& ParameterMap)
{
	SplineStartPosParam.Bind(ParameterMap,TEXT("SplineStartPos"), SPF_Mandatory);
	SplineStartTangentParam.Bind(ParameterMap,TEXT("SplineStartTangent"), SPF_Mandatory);
	SplineStartRollParam.Bind(ParameterMap,TEXT("SplineStartRoll"), SPF_Mandatory);
	SplineStartScaleParam.Bind(ParameterMap,TEXT("SplineStartScale"), SPF_Mandatory);
	SplineStartOffsetParam.Bind(ParameterMap,TEXT("SplineStartOffset"), SPF_Mandatory);

	SplineEndPosParam.Bind(ParameterMap,TEXT("SplineEndPos"), SPF_Mandatory);
	SplineEndTangentParam.Bind(ParameterMap,TEXT("SplineEndTangent"), SPF_Mandatory);
	SplineEndRollParam.Bind(ParameterMap,TEXT("SplineEndRoll"), SPF_Mandatory);
	SplineEndScaleParam.Bind(ParameterMap,TEXT("SplineEndScale"), SPF_Mandatory);
	SplineEndOffsetParam.Bind(ParameterMap,TEXT("SplineEndOffset"), SPF_Mandatory);
	
	SplineUpDirParam.Bind(ParameterMap,TEXT("SplineUpDir"), SPF_Mandatory);
	SmoothInterpRollScaleParam.Bind(ParameterMap,TEXT("SmoothInterpRollScale"), SPF_Mandatory);

	SplineMeshMinZParam.Bind(ParameterMap,TEXT("SplineMeshMinZ"), SPF_Mandatory);
	SplineMeshScaleZParam.Bind(ParameterMap,TEXT("SplineMeshScaleZ"), SPF_Mandatory);

	SplineMeshDirParam.Bind(ParameterMap,TEXT("SplineMeshDir"), SPF_Mandatory);
	SplineMeshXParam.Bind(ParameterMap,TEXT("SplineMeshX"), SPF_Mandatory);
	SplineMeshYParam.Bind(ParameterMap,TEXT("SplineMeshY"), SPF_Mandatory);
}

void FSplineMeshVertexFactoryShaderParameters::SetMesh(FShader* Shader,const FVertexFactory* VertexFactory,const FSceneView& View,const FMeshBatchElement& BatchElement,uint32 DataFlags) const
{	
	if(Shader->GetVertexShader())
	{
		FSplineMeshVertexFactory* SplineVertexFactory = (FSplineMeshVertexFactory*)VertexFactory;
		FSplineMeshSceneProxy* SplineProxy = SplineVertexFactory->SplineSceneProxy;
		FSplineMeshParams& SplineParams = SplineProxy->SplineParams;

		SetShaderValue(Shader->GetVertexShader(), SplineStartPosParam, SplineParams.StartPos);
		SetShaderValue(Shader->GetVertexShader(), SplineStartTangentParam, SplineParams.StartTangent);
		SetShaderValue(Shader->GetVertexShader(), SplineStartRollParam, SplineParams.StartRoll);
		SetShaderValue(Shader->GetVertexShader(), SplineStartScaleParam, SplineParams.StartScale);
		SetShaderValue(Shader->GetVertexShader(), SplineStartOffsetParam, SplineParams.StartOffset);

		SetShaderValue(Shader->GetVertexShader(), SplineEndPosParam, SplineParams.EndPos);
		SetShaderValue(Shader->GetVertexShader(), SplineEndTangentParam, SplineParams.EndTangent);
		SetShaderValue(Shader->GetVertexShader(), SplineEndRollParam, SplineParams.EndRoll);
		SetShaderValue(Shader->GetVertexShader(), SplineEndScaleParam, SplineParams.EndScale);
		SetShaderValue(Shader->GetVertexShader(), SplineEndOffsetParam, SplineParams.EndOffset);
	
		SetShaderValue(Shader->GetVertexShader(), SplineUpDirParam, SplineProxy->SplineXDir);
		SetShaderValue(Shader->GetVertexShader(), SmoothInterpRollScaleParam, SplineProxy->bSmoothInterpRollScale);

		SetShaderValue(Shader->GetVertexShader(), SplineMeshMinZParam, SplineProxy->SplineMeshMinZ);
		SetShaderValue(Shader->GetVertexShader(), SplineMeshScaleZParam, SplineProxy->SplineMeshScaleZ);

		FVector DirMask(0,0,0);
		DirMask[SplineProxy->ForwardAxis] = 1;
		SetShaderValue(Shader->GetVertexShader(), SplineMeshDirParam, DirMask);
		DirMask = FVector::ZeroVector;
		DirMask[(SplineProxy->ForwardAxis+1)%3] = 1;
		SetShaderValue(Shader->GetVertexShader(), SplineMeshXParam, DirMask);
		DirMask = FVector::ZeroVector;
		DirMask[(SplineProxy->ForwardAxis+2)%3] = 1;
		SetShaderValue(Shader->GetVertexShader(), SplineMeshYParam, DirMask);
	}
}

void FSplineMeshSceneProxy::InitResources(USplineMeshComponent* InComponent, int32 InLODIndex)
{
	// Initialize the static mesh's vertex factory.
	ENQUEUE_UNIQUE_RENDER_COMMAND_THREEPARAMETER(
		InitSplineMeshVertexFactory,
		FSplineMeshVertexFactory*,VertexFactory,LODResources[InLODIndex].VertexFactory,
		const FStaticMeshLODResources*,RenderData,&InComponent->StaticMesh->RenderData->LODResources[InLODIndex],
		UStaticMesh*,Parent,InComponent->StaticMesh,
	{
		FLocalVertexFactory::DataType Data;

		Data.PositionComponent = FVertexStreamComponent(
			&RenderData->PositionVertexBuffer,
			STRUCT_OFFSET(FPositionVertex,Position),
			RenderData->PositionVertexBuffer.GetStride(),
			VET_Float3
			);
		Data.TangentBasisComponents[0] = FVertexStreamComponent(
			&RenderData->VertexBuffer,
			STRUCT_OFFSET(FStaticMeshFullVertex,TangentX),
			RenderData->VertexBuffer.GetStride(),
			VET_PackedNormal
			);
		Data.TangentBasisComponents[1] = FVertexStreamComponent(
			&RenderData->VertexBuffer,
			STRUCT_OFFSET(FStaticMeshFullVertex,TangentZ),
			RenderData->VertexBuffer.GetStride(),
			VET_PackedNormal
			);

		if( RenderData->ColorVertexBuffer.GetNumVertices() > 0 )
		{
			Data.ColorComponent = FVertexStreamComponent(
				&RenderData->ColorVertexBuffer,
				0,	// Struct offset to color
				RenderData->ColorVertexBuffer.GetStride(),
				VET_Color
				);
		}

		Data.TextureCoordinates.Empty();

		if( !RenderData->VertexBuffer.GetUseFullPrecisionUVs() )
		{
			for(uint32 UVIndex = 0;UVIndex < RenderData->VertexBuffer.GetNumTexCoords();UVIndex++)
			{
				Data.TextureCoordinates.Add(FVertexStreamComponent(
					&RenderData->VertexBuffer,
					STRUCT_OFFSET(TStaticMeshFullVertexFloat16UVs<MAX_TEXCOORDS>,UVs) + sizeof(FVector2DHalf) * UVIndex,
					RenderData->VertexBuffer.GetStride(),
					VET_Half2
					));
			}
			if(	Parent->LightMapCoordinateIndex >= 0 && (uint32)Parent->LightMapCoordinateIndex < RenderData->VertexBuffer.GetNumTexCoords())
			{
				Data.LightMapCoordinateComponent = FVertexStreamComponent(
					&RenderData->VertexBuffer,
					STRUCT_OFFSET(TStaticMeshFullVertexFloat16UVs<MAX_TEXCOORDS>,UVs) + sizeof(FVector2DHalf) * Parent->LightMapCoordinateIndex,
					RenderData->VertexBuffer.GetStride(),
					VET_Half2
					);
			}
		}
		else
		{
			for(uint32 UVIndex = 0;UVIndex < RenderData->VertexBuffer.GetNumTexCoords();UVIndex++)
			{
				Data.TextureCoordinates.Add(FVertexStreamComponent(
					&RenderData->VertexBuffer,
					STRUCT_OFFSET(TStaticMeshFullVertexFloat32UVs<MAX_TEXCOORDS>,UVs) + sizeof(FVector2D) * UVIndex,
					RenderData->VertexBuffer.GetStride(),
					VET_Float2
					));
			}

			if (Parent->LightMapCoordinateIndex >= 0 && (uint32)Parent->LightMapCoordinateIndex < RenderData->VertexBuffer.GetNumTexCoords())
			{
				Data.LightMapCoordinateComponent = FVertexStreamComponent(
					&RenderData->VertexBuffer,
					STRUCT_OFFSET(TStaticMeshFullVertexFloat32UVs<MAX_TEXCOORDS>,UVs) + sizeof(FVector2D) * Parent->LightMapCoordinateIndex,
					RenderData->VertexBuffer.GetStride(),
					VET_Float2
					);
			}
		}	

		VertexFactory->SetData(Data);

		VertexFactory->InitResource();
	});
}


void FSplineMeshSceneProxy::ReleaseResources()
{
	for (int32 LODIndex = 0; LODIndex < LODResources.Num(); LODIndex++)
	{
		LODResources[LODIndex].VertexFactory->ReleaseResource();
	}
}

USplineMeshComponent::USplineMeshComponent(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	Mobility = EComponentMobility::Static;

	BodyInstance.bEnableCollision_DEPRECATED = false;
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	bHasCustomNavigableGeometry = EHasCustomNavigableGeometry::Yes;

	SplineXDir.X = 1.0f;
}

void USplineMeshComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.UE4Ver() < VER_UE4_SPLINE_MESH_ORIENTATION)
	{
		ForwardAxis = ESplineMeshAxis::Z;
		SplineParams.StartRoll -= HALF_PI;
		SplineParams.EndRoll -= HALF_PI;

		float Temp = SplineParams.StartOffset.X;
		SplineParams.StartOffset.X = -SplineParams.StartOffset.Y;
		SplineParams.StartOffset.Y = Temp;
		Temp = SplineParams.EndOffset.X;
		SplineParams.EndOffset.X = -SplineParams.EndOffset.Y;
		SplineParams.EndOffset.Y = Temp;
	}
}

FPrimitiveSceneProxy* USplineMeshComponent::CreateSceneProxy()
{
	// Verify that the mesh is valid before using it.
	const bool bMeshIsValid = 
		// make sure we have an actual staticmesh
		StaticMesh &&
		StaticMesh->HasValidRenderData();

	if(bMeshIsValid && (GRHIFeatureLevel >= ERHIFeatureLevel::SM3))
	{
		return ::new FSplineMeshSceneProxy(this);
	}
	else
	{
		return NULL;
	}
}

FBoxSphereBounds USplineMeshComponent::CalcBounds(const FTransform & LocalToWorld) const
{
	// Use util to generate bounds of spline
	FInterpCurvePoint<FVector> Start(0.f, SplineParams.StartPos, SplineParams.StartTangent, SplineParams.StartTangent, CIM_CurveUser);
	FInterpCurvePoint<FVector> End(1.f, SplineParams.EndPos, SplineParams.EndTangent, SplineParams.EndTangent, CIM_CurveUser);

	FVector CurveMax(-BIG_NUMBER, -BIG_NUMBER, -BIG_NUMBER);
	FVector CurveMin(BIG_NUMBER, BIG_NUMBER, BIG_NUMBER);
	CurveVectorFindIntervalBounds(Start, End, CurveMin, CurveMax);

	FBox LocalBox(CurveMin, CurveMax);

	// Find largest extent of mesh in XY, and add on all around
	if(StaticMesh)
	{
		FVector MinMeshExtent = StaticMesh->GetBounds().Origin - StaticMesh->GetBounds().BoxExtent;
		FVector MaxMeshExtent = StaticMesh->GetBounds().Origin + StaticMesh->GetBounds().BoxExtent;
		GetAxisValue(MinMeshExtent, ForwardAxis) = 0;
		GetAxisValue(MaxMeshExtent, ForwardAxis) = 0;
		float MaxDim = FMath::Max<float>(MinMeshExtent.GetAbsMax(), MaxMeshExtent.GetAbsMax());

		float MaxScale = FMath::Max(SplineParams.StartScale.GetAbsMax(), SplineParams.EndScale.GetAbsMax());

		LocalBox = LocalBox.ExpandBy(MaxScale * MaxDim);
	}

	return FBoxSphereBounds( LocalBox.TransformBy(LocalToWorld) );
}


/** 
 * Functions used for transforming a static mesh component based on a spline.  
 * This needs to be updated if the spline functionality changes!
 */
static float SmoothStep(float A, float B, float X)
{
	if (X < A)
	{
		return 0.0f;
	}
	else if (X >= B)
	{
		return 1.0f;
	}
	const float InterpFraction = (X - A) / (B - A);
	return InterpFraction * InterpFraction * (3.0f - 2.0f * InterpFraction);
}

static FVector SplineEvalPos(const FVector& StartPos, const FVector& StartTangent, const FVector& EndPos, const FVector& EndTangent, float A)
{
	const float A2 = A  * A;
	const float A3 = A2 * A;

	return (((2*A3)-(3*A2)+1) * StartPos) + ((A3-(2*A2)+A) * StartTangent) + ((A3-A2) * EndTangent) + (((-2*A3)+(3*A2)) * EndPos);
}

static FVector SplineEvalDir(const FVector& StartPos, const FVector& StartTangent, const FVector& EndPos, const FVector& EndTangent, float A)
{
	const FVector C = (6*StartPos) + (3*StartTangent) + (3*EndTangent) - (6*EndPos);
	const FVector D = (-6*StartPos) - (4*StartTangent) - (2*EndTangent) + (6*EndPos);
	const FVector E = StartTangent;

	const float A2 = A  * A;

	return ((C * A2) + (D * A) + E).SafeNormal();
}


FTransform USplineMeshComponent::CalcSliceTransform(const float DistanceAlong) const
{
	// Find how far 'along' mesh we are
	FBoxSphereBounds StaticMeshBounds = StaticMesh->GetBounds();
	const float MeshMinZ = GetAxisValue(StaticMeshBounds.Origin, ForwardAxis) - GetAxisValue(StaticMeshBounds.BoxExtent, ForwardAxis);
	const float MeshRangeZ = 2.0f * GetAxisValue(StaticMeshBounds.BoxExtent, ForwardAxis);
	const float Alpha = (DistanceAlong - MeshMinZ) / MeshRangeZ;

	// Apply hermite interp to Alpha if desired
	const float HermiteAlpha = bSmoothInterpRollScale ? SmoothStep(0.0, 1.0, Alpha) : Alpha;

	// Then find the point and direction of the spline at this point along
	FVector SplinePos = SplineEvalPos( SplineParams.StartPos, SplineParams.StartTangent, SplineParams.EndPos, SplineParams.EndTangent, Alpha );
	const FVector SplineDir = SplineEvalDir( SplineParams.StartPos, SplineParams.StartTangent, SplineParams.EndPos, SplineParams.EndTangent, Alpha );

	// Find base frenet frame
	const FVector BaseXVec = (SplineXDir ^ SplineDir).SafeNormal();
	const FVector BaseYVec = (SplineDir ^ BaseXVec).SafeNormal();

	// Offset the spline by the desired amount
	const FVector2D SliceOffset = FMath::Lerp<FVector2D>(SplineParams.StartOffset, SplineParams.EndOffset, HermiteAlpha);
	SplinePos += SliceOffset.X * BaseXVec;
	SplinePos += SliceOffset.Y * BaseYVec;

	// Apply roll to frame around spline
	const float UseRoll = FMath::Lerp(SplineParams.StartRoll, SplineParams.EndRoll, HermiteAlpha);
	const float CosAng = FMath::Cos(UseRoll);
	const float SinAng = FMath::Sin(UseRoll);
	const FVector XVec = (CosAng * BaseXVec) - (SinAng * BaseYVec);
	const FVector YVec = (CosAng * BaseYVec) + (SinAng * BaseXVec);

	// Find scale at this point along spline
	const FVector2D UseScale = FMath::Lerp(SplineParams.StartScale, SplineParams.EndScale, HermiteAlpha);

	// Build overall transform
	FTransform SliceTransform;
	switch (ForwardAxis)
	{
	case ESplineMeshAxis::X:
		SliceTransform = FTransform(SplineDir, XVec, YVec, SplinePos);
		SliceTransform.SetScale3D(FVector(1, UseScale.X, UseScale.Y));
		break;
	case ESplineMeshAxis::Y:
		SliceTransform = FTransform(YVec, SplineDir, XVec, SplinePos);
		SliceTransform.SetScale3D(FVector(UseScale.Y, 1, UseScale.X));
		break;
	case ESplineMeshAxis::Z:
		SliceTransform = FTransform(XVec, YVec, SplineDir, SplinePos);
		SliceTransform.SetScale3D(FVector(UseScale.X, UseScale.Y, 1));
		break;
	default:
		check(0);
		break;
	}

	return SliceTransform;
}


bool USplineMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	if (StaticMesh)
	{
		StaticMesh->GetPhysicsTriMeshData(CollisionData, InUseAllTriData);

		FVector Mask = FVector(1,1,1);
		GetAxisValue(Mask, ForwardAxis) = 0;

		for (int32 i = 0; i < CollisionData->Vertices.Num(); i++)
		{
			CollisionData->Vertices[i] = CalcSliceTransform(GetAxisValue(CollisionData->Vertices[i], ForwardAxis)).TransformPosition(CollisionData->Vertices[i] * Mask);
		}
		return true;
	}

	return false;
}

bool USplineMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	if (StaticMesh)
	{
		return StaticMesh->ContainsPhysicsTriMeshData(InUseAllTriData);
	}

	return false;
}

void USplineMeshComponent::CreatePhysicsState()
{
#if WITH_EDITOR
	if (StaticMesh != NULL && CachedMeshBodySetupGuid != StaticMesh->BodySetup->BodySetupGuid)
	{
		RecreateCollision();
	}
#endif

	return Super::CreatePhysicsState();
}

UBodySetup* USplineMeshComponent::GetBodySetup()
{
	// Don't return a body setup that has no collision, it means we are interactively moving the spline and don't want to build collision.
	// Instead we explicitly build collision with USplineMeshComponent::RecreateCollision()
	if (BodySetup != NULL && (BodySetup->TriMesh != NULL || BodySetup->AggGeom.GetElementCount() > 0))
	{
		return BodySetup;
	}
	return NULL;
}

#include "../AI/Navigation/RecastHelpers.h"
bool USplineMeshComponent::DoCustomNavigableGeometryExport(struct FNavigableGeometryExport* GeomExport) const
{
	// the NavCollision is supposed to be faster than exporting the regular collision,
	// but I'm not sure that's true here, as the regular collision is pre-distorted to the spline

	if (StaticMesh != NULL && StaticMesh->NavCollision != NULL)
	{
		UNavCollision* NavCollision = StaticMesh->NavCollision;
		
		if (ensure(!NavCollision->bIsDynamicObstacle))
		{
			if (NavCollision->bHasConvexGeometry)
			{
				TArray<FVector> VertexBuffer;
				VertexBuffer.Reserve(FMath::Max(NavCollision->ConvexCollision.VertexBuffer.Num(), NavCollision->TriMeshCollision.VertexBuffer.Num()));

				for (int32 i = 0; i < NavCollision->ConvexCollision.VertexBuffer.Num(); ++i)
				{
					FVector Vertex = NavCollision->ConvexCollision.VertexBuffer[i];
					Vertex = CalcSliceTransform(GetAxisValue(Vertex, ForwardAxis)).TransformPosition(Vertex);
					VertexBuffer.Add(Vertex);
				}
				GeomExport->ExportCustomMesh(VertexBuffer.GetData(), VertexBuffer.Num(),
					NavCollision->ConvexCollision.IndexBuffer.GetData(), NavCollision->ConvexCollision.IndexBuffer.Num(),
					ComponentToWorld);

				VertexBuffer.Reset();
				for (int32 i = 0; i < NavCollision->TriMeshCollision.VertexBuffer.Num(); ++i)
				{
					FVector Vertex = NavCollision->TriMeshCollision.VertexBuffer[i];
					Vertex = CalcSliceTransform(GetAxisValue(Vertex, ForwardAxis)).TransformPosition(Vertex);
					VertexBuffer.Add(Vertex);
				}
				GeomExport->ExportCustomMesh(VertexBuffer.GetData(), VertexBuffer.Num(),
					NavCollision->TriMeshCollision.IndexBuffer.GetData(), NavCollision->TriMeshCollision.IndexBuffer.Num(),
					ComponentToWorld);

				return false;
			}
		}
	}

	return true;
}

#if WITH_EDITOR
void USplineMeshComponent::RecreateCollision()
{
	if (StaticMesh && IsCollisionEnabled())
	{
		if (BodySetup == NULL)
		{
			BodySetup = DuplicateObject<UBodySetup>(StaticMesh->BodySetup, this);
			BodySetup->InvalidatePhysicsData();
		}
		else
		{
			BodySetup->Modify();
			BodySetup->InvalidatePhysicsData();
			BodySetup->CopyBodyPropertiesFrom(StaticMesh->BodySetup);
			BodySetup->CollisionTraceFlag = StaticMesh->BodySetup->CollisionTraceFlag;
		}

		if (BodySetup->CollisionTraceFlag == CTF_UseComplexAsSimple)
		{
			BodySetup->AggGeom.EmptyElements();
		}
		else
		{
			FVector Mask = FVector(1,1,1);
			GetAxisValue(Mask, ForwardAxis) = 0;

			// distortion of a sphere can't be done nicely, so we just transform the origin and size
			for (auto ItElement = BodySetup->AggGeom.SphereElems.CreateIterator(); ItElement; ++ItElement)
			{
				const float Z = GetAxisValue(ItElement->Center, ForwardAxis);
				FTransform SliceTransform = CalcSliceTransform(Z);
				ItElement->Center *= Mask;

				ItElement->Radius *= SliceTransform.GetMaximumAxisScale();

				SliceTransform.RemoveScaling();
				ItElement->Center = SliceTransform.TransformPosition( ItElement->Center );
			}

			// distortion of a sphyl can't be done nicely, so we just transform the origin and size
			for (auto ItElement = BodySetup->AggGeom.SphylElems.CreateIterator(); ItElement; ++ItElement)
			{
				const float Z = GetAxisValue(ItElement->Center, ForwardAxis);
				FTransform SliceTransform = CalcSliceTransform(Z);
				ItElement->Center *= Mask;

				FTransform TM = ItElement->GetTransform();
				ItElement->Length = (TM * SliceTransform).TransformVector(FVector(0, 0, ItElement->Length)).Size();
				ItElement->Radius *= SliceTransform.GetMaximumAxisScale();

				SliceTransform.RemoveScaling();
				ItElement->SetTransform( TM * SliceTransform );
			}

			// Convert boxes to convex hulls to better respect distortion
			for (auto ItElement = BodySetup->AggGeom.BoxElems.CreateIterator(); ItElement; ++ItElement)
			{
				FKConvexElem& ConvexElem = *new(BodySetup->AggGeom.ConvexElems) FKConvexElem();

				const FVector Radii = FVector(ItElement->X / 2, ItElement->Y / 2, ItElement->Z / 2);
				FTransform ElementTM = ItElement->GetTransform();
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector(-1,-1,-1)));
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector(-1,-1, 1)));
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector(-1, 1,-1)));
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector(-1, 1, 1)));
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector( 1,-1,-1)));
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector( 1,-1, 1)));
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector( 1, 1,-1)));
				ConvexElem.VertexData.Add(ElementTM.TransformPosition(Radii * FVector( 1, 1, 1)));

				ConvexElem.UpdateElemBox();
			}
			BodySetup->AggGeom.BoxElems.Empty();

			// transform the points of the convex hulls into spline space
			for (auto ItElement = BodySetup->AggGeom.ConvexElems.CreateIterator(); ItElement; ++ItElement)
			{
				for (auto It = (*ItElement).VertexData.CreateIterator(); It; ++It)
				{
					(*It) = CalcSliceTransform(GetAxisValue(*It, ForwardAxis)).TransformPosition((*It) * Mask);
				}
			}

			BodySetup->CreatePhysicsMeshes();
			CachedMeshBodySetupGuid = StaticMesh->BodySetup->BodySetupGuid;
		}
	}
	else
	{
		if (BodySetup != NULL)
		{
			BodySetup->MarkPendingKill();
			BodySetup = NULL;
			CachedMeshBodySetupGuid = FGuid();
		}
	}
}
#endif

#include "StaticMeshLight.h"
/** */
class FSplineStaticLightingMesh : public FStaticMeshStaticLightingMesh
{
public:

	FSplineStaticLightingMesh(const USplineMeshComponent* InPrimitive,int32 InLODIndex,const TArray<ULightComponent*>& InRelevantLights):
		FStaticMeshStaticLightingMesh(InPrimitive, InLODIndex, InRelevantLights),
		SplineComponent(InPrimitive)
	{
	}

#if WITH_EDITOR
	virtual const struct FSplineMeshParams* GetSplineParameters() const
	{
		return &SplineComponent->SplineParams;
	}
#endif	//WITH_EDITOR

private:
	const USplineMeshComponent* SplineComponent;
};

FStaticMeshStaticLightingMesh* USplineMeshComponent::AllocateStaticLightingMesh(int32 LODIndex, const TArray<ULightComponent*>& InRelevantLights)
{
	return new FSplineStaticLightingMesh(this, LODIndex, InRelevantLights);
}
