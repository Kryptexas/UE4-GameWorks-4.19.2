// Copyright 2017 Google Inc.

#include "GoogleARCorePlaneRendererComponent.h"
#include "ARBlueprintLibrary.h"
#include "ARSystem.h"
#include "ARTrackable.h"
#include "Templates/Casts.h"

UGoogleARCorePlaneRendererComponent::UGoogleARCorePlaneRendererComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	PlaneIndices.AddUninitialized(6);
	PlaneIndices[0] = 0; PlaneIndices[1] = 1; PlaneIndices[2] = 2;
	PlaneIndices[3] = 0; PlaneIndices[4] = 2; PlaneIndices[5] = 3;
}

void UGoogleARCorePlaneRendererComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	DrawPlanes();
}

void UGoogleARCorePlaneRendererComponent::DrawPlanes()
{
	UWorld* World = GetWorld();
	if (UARBlueprintLibrary::GetTrackingQuality() == EARTrackingQuality::OrientationAndPosition)
	{
		TArray<UARTrackedGeometry*> PlaneList;
		PlaneList = UARBlueprintLibrary::GetAllGeometries();
		for (UARTrackedGeometry* Plane : PlaneList)
		{
			if (!Plane->IsA(UARPlaneGeometry::StaticClass()))
			{
				continue;
			}

			if (Plane->GetTrackingState() != EARTrackingState::Tracking)
			{
				continue;
			}

			if (bRenderPlane)
			{
				CastChecked<UARPlaneGeometry>(Plane)->DebugDraw(World, BoundaryPolygonColor, BoundaryPolygonThickness);
			}
		}
	}
}
