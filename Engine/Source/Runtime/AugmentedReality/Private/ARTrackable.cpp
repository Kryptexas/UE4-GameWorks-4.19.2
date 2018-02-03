// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ARTrackable.h"
#include "ARSystem.h"
#include "ARDebugDrawHelpers.h"
#include "DrawDebugHelpers.h"

//
//
//
UARTrackedGeometry::UARTrackedGeometry()
:TrackingState(EARTrackingState::Tracking)
,NativeResource(nullptr)
{
	
}

void UARTrackedGeometry::InitializeNativeResource(IARRef* InNativeResource)
{
	NativeResource.Reset(InNativeResource);
}

void UARTrackedGeometry::DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds ) const
{
	FTransform WorldTrans = GetLocalToWorldTransform();
	FVector Location = WorldTrans.GetLocation();
	FRotator Rotation = FRotator(WorldTrans.GetRotation());
	FVector Scale3D = WorldTrans.GetScale3D();
	DrawDebugCoordinateSystem(World, Location, Rotation, Scale3D.X, true, PersistForSeconds, 0, OutlineThickness);
}

TSharedPtr<FARSystemBase, ESPMode::ThreadSafe> UARTrackedGeometry::GetARSystem() const
{
	auto MyARSystem = ARSystem.Pin();
	return MyARSystem;
}

FTransform UARTrackedGeometry::GetLocalToTrackingTransform() const
{
	return LocalToAlignedTrackingTransform;
}

FTransform UARTrackedGeometry::GetLocalToTrackingTransform_NoAlignment() const
{
	return LocalToTrackingTransform;
}

EARTrackingState UARTrackedGeometry::GetTrackingState() const
{
	return TrackingState;
}

FTransform UARTrackedGeometry::GetLocalToWorldTransform() const
{
	return GetLocalToTrackingTransform() * GetARSystem()->GetTrackingToWorldTransform();
}

int32 UARTrackedGeometry::GetLastUpdateFrameNumber() const
{
	return LastUpdateFrameNumber;
}

FName UARTrackedGeometry::GetDebugName() const
{
	return DebugName;
}

float UARTrackedGeometry::GetLastUpdateTimestamp() const
{
	return LastUpdateTimestamp;
}

void UARTrackedGeometry::UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform )
{
	ARSystem = InTrackingSystem;
	LocalToTrackingTransform = InLocalToTrackingTransform;
	LastUpdateFrameNumber = FrameNumber;
	LastUpdateTimestamp = Timestamp;
	UpdateAlignmentTransform(InAlignmentTransform);
}

void UARTrackedGeometry::UpdateTrackingState( EARTrackingState NewTrackingState )
{
	TrackingState = NewTrackingState;

	if (TrackingState == EARTrackingState::StoppedTracking && NativeResource)
	{
		// Remove reference to the native resource since the tracked geometry is stopped tracking.
		NativeResource->RemoveRef();
	}
}

void UARTrackedGeometry::UpdateAlignmentTransform( const FTransform& NewAlignmentTransform )
{
	LocalToAlignedTrackingTransform = LocalToTrackingTransform * NewAlignmentTransform;
}

void UARTrackedGeometry::SetDebugName( FName InDebugName )
{
	DebugName = InDebugName;
}

IARRef* UARTrackedGeometry::GetNativeResource()
{
	return NativeResource.Get();
}

//
//
//
void UARPlaneGeometry::UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform, const FVector InCenter, const FVector InExtent )
{
	Super::UpdateTrackedGeometry(InTrackingSystem, FrameNumber, Timestamp, InLocalToTrackingTransform, InAlignmentTransform);
	Center = InCenter;
	Extent = InExtent;

	BoundaryPolygon.Empty(4);
	BoundaryPolygon.Add(FVector(-Extent.X, -Extent.Y, 0.0f));
	BoundaryPolygon.Add(FVector(Extent.X, -Extent.Y, 0.0f));
	BoundaryPolygon.Add(FVector(Extent.X, Extent.Y, 0.0f));
	BoundaryPolygon.Add(FVector(-Extent.X, Extent.Y, 0.0f));

	SubsumedBy = nullptr;
}

void UARPlaneGeometry::UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform, const FVector InCenter, const FVector InExtent, const TArray<FVector>& InBoundaryPolygon, UARPlaneGeometry* InSubsumedBy)
{
	UpdateTrackedGeometry(InTrackingSystem, FrameNumber, Timestamp, InLocalToTrackingTransform, InAlignmentTransform, InCenter, InExtent);
	BoundaryPolygon = InBoundaryPolygon;
	SubsumedBy = InSubsumedBy;
}

void UARPlaneGeometry::DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds ) const
{
	const FTransform LocalToWorldTransform = GetLocalToWorldTransform();
	const FVector WorldSpaceCenter = LocalToWorldTransform.TransformPosition(Center);
	DrawDebugBox( World, WorldSpaceCenter, Extent, LocalToWorldTransform.GetRotation(), OutlineColor.ToFColor(false), false, PersistForSeconds, 0, 0.1f*OutlineThickness );
	
	const FString CurAnchorDebugName = GetDebugName().ToString();
	ARDebugHelpers::DrawDebugString( World, WorldSpaceCenter, CurAnchorDebugName, 0.25f*OutlineThickness, OutlineColor.ToFColor(false), PersistForSeconds, true);
}


void UARFaceGeometry::UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform, FARBlendShapeMap& InBlendShapes, TArray<FVector>& InVertices, const TArray<int32>& Indices)
{
	Super::UpdateTrackedGeometry(InTrackingSystem, FrameNumber, Timestamp, InLocalToTrackingTransform, InAlignmentTransform);
	BlendShapes = MoveTemp(InBlendShapes);
	VertexBuffer = MoveTemp(InVertices);
	// This won't change ever so only copy first time
	if (IndexBuffer.Num() == 0)
	{
		IndexBuffer = Indices;
	}
}

void UARTrackedPoint::DebugDraw(UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds /*= 0.0f*/) const
{
	DrawDebugPoint(World, GetLocalToWorldTransform().GetLocation(), 0.5f, OutlineColor.ToFColor(false), false, PersistForSeconds, 0);
}

void UARTrackedPoint::UpdateTrackedGeometry(const TSharedRef<FARSystemBase, ESPMode::ThreadSafe>& InTrackingSystem, uint32 FrameNumber, double Timestamp, const FTransform& InLocalToTrackingTransform, const FTransform& InAlignmentTransform)
{
	Super::UpdateTrackedGeometry(InTrackingSystem, FrameNumber, Timestamp, InLocalToTrackingTransform, InAlignmentTransform);
}

void UARFaceGeometry::DebugDraw( UWorld* World, const FLinearColor& OutlineColor, float OutlineThickness, float PersistForSeconds ) const
{
	Super::DebugDraw(World, OutlineColor, OutlineThickness, PersistForSeconds);
}

float UARFaceGeometry::GetBlendShapeValue(EARFaceBlendShape BlendShape) const
{
	float Value = 0.f;
	if (BlendShapes.Contains(BlendShape))
	{
		Value = BlendShapes[BlendShape];
	}
	return Value;
}

const TMap<EARFaceBlendShape, float> UARFaceGeometry::GetBlendShapes() const
{
	return BlendShapes;
}
