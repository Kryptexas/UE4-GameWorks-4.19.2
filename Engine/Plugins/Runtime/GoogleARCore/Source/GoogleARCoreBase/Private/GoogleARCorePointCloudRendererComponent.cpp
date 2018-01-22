// Copyright 2017 Google Inc.

#include "GoogleARCorePointCloudRendererComponent.h"
#include "DrawDebugHelpers.h"
#include "GoogleARCoreTypes.h"
#include "GoogleARCoreFunctionLibrary.h"

UGoogleARCorePointCloudRendererComponent::UGoogleARCorePointCloudRendererComponent()
{
	PreviousPointCloudTimestamp = 0.0;
	PrimaryComponentTick.bCanEverTick = true;
}

void UGoogleARCorePointCloudRendererComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction * ThisTickFunction)
{
	DrawPointCloud();
}

void UGoogleARCorePointCloudRendererComponent::DrawPointCloud()
{
	UWorld* World = GetWorld();
	if (UGoogleARCoreFrameFunctionLibrary::GetTrackingState() == EGoogleARCoreTrackingState::Tracking)
	{
		UGoogleARCorePointCloud* LatestPointCloud = nullptr;
		EGoogleARCoreFunctionStatus Status = UGoogleARCoreFrameFunctionLibrary::GetLatestPointCloud(LatestPointCloud);
		if (Status == EGoogleARCoreFunctionStatus::Success)
		{
			for (int i = 0; i < LatestPointCloud->GetPointNum(); i++)
			{
				DrawDebugPoint(World, LatestPointCloud->GetWorldSpacePoint(i), PointSize, PointColor, false);
			}
		}
	}
}
