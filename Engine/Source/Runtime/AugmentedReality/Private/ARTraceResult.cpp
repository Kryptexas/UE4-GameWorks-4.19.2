// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ARTraceResult.h"
#include "ARSystem.h"


//
//
//
FARTraceResult::FARTraceResult()
: FARTraceResult(nullptr, 0.0f, EARLineTraceChannels::None, FTransform(), nullptr)
{
	
}


FARTraceResult::FARTraceResult( const TSharedPtr<FARSystemBase, ESPMode::ThreadSafe>& InARSystem, float InDistanceFromCamera, EARLineTraceChannels InTraceChannel, const FTransform& InLocalToTrackingTransform, UARTrackedGeometry* InTrackedGeometry )
: DistanceFromCamera(InDistanceFromCamera)
, TraceChannel(InTraceChannel)
, LocalToTrackingTransform(InLocalToTrackingTransform)
, TrackedGeometry(InTrackedGeometry)
, ARSystem(InARSystem)
{
	
}

float FARTraceResult::GetDistanceFromCamera() const
{
	return DistanceFromCamera;
}

FTransform FARTraceResult::GetLocalToTrackingTransform() const
{
	return LocalToTrackingTransform;
}


FTransform FARTraceResult::GetLocalToWorldTransform() const
{
	return LocalToTrackingTransform * ARSystem->GetTrackingToWorldTransform();
}


UARTrackedGeometry* FARTraceResult::GetTrackedGeometry() const
{
	return TrackedGeometry;
}

EARLineTraceChannels FARTraceResult::GetTraceChannel() const
{
	return TraceChannel;
}
