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
		EGoogleARCoreFunctionStatus Status = UGoogleARCoreFrameFunctionLibrary::GetPointCloud(LatestPointCloud);
		if (Status == EGoogleARCoreFunctionStatus::Success && LatestPointCloud != nullptr && LatestPointCloud->GetPointNum() > 0)
		{
			for (int i = 0; i < LatestPointCloud->GetPointNum(); i++)
			{
				FVector PointPosition = FVector::ZeroVector;
				float PointConfidence = 0;
				LatestPointCloud->GetPoint(i, PointPosition, PointConfidence);
				DrawDebugPoint(World, PointPosition, PointSize, PointColor, false);
			}
		}
	}
}
