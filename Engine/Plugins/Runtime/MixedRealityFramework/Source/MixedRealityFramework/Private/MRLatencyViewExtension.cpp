// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MRLatencyViewExtension.h"
#include "MotionDelayBuffer.h"
#include "MixedRealityCaptureComponent.h"
#include "HAL/IConsoleManager.h" // for TAutoConsoleVariable<>
#include "RenderingThread.h" // for ENQUEUE_RENDER_COMMAND

namespace MRLatencyViewExtension_Impl
{
	TAutoConsoleVariable<int32> CVarMotionCaptureLatencyOverride(
		TEXT("mr.MotionCaptureLatencyOverride"),
		0,
		TEXT("When set, will track historical motion data, using it to simulate latency when rendering to the MR capture view (helpful when trying to sync with a video feed).\n")
		TEXT("     0: don't use the override - default to the MR capture's calibrated delay (default)\n")
		TEXT(" [1:n]: use motion controller transforms from n milliseconds ago when rendering the MR capture"),
		ECVF_Default);

	static uint32 GetDesiredDelay(TWeakObjectPtr<UMixedRealityCaptureComponent> Target);
}

//------------------------------------------------------------------------------
static uint32 MRLatencyViewExtension_Impl::GetDesiredDelay(TWeakObjectPtr<UMixedRealityCaptureComponent> Target)
{
	uint32 DesiredDelay = CVarMotionCaptureLatencyOverride.GetValueOnGameThread();
	if (DesiredDelay <= 0 && Target.IsValid())
	{
		DesiredDelay = Target->TrackingLatency;
	}
	return DesiredDelay;
}

/* FMRLatencyViewExtension
 *****************************************************************************/

//------------------------------------------------------------------------------
FMRLatencyViewExtension::FMRLatencyViewExtension(const FAutoRegister& AutoRegister, UMixedRealityCaptureComponent* InOwner)
	: FMotionDelayClient(AutoRegister)
	, Owner(InOwner)
	, CachedRenderDelay(0)
{}

//------------------------------------------------------------------------------
bool FMRLatencyViewExtension::SetupPreCapture(FSceneInterface* Scene)
{
	const bool bSimulateLatency = (CachedRenderDelay > 0);
	if (bSimulateLatency)
	{
		TSharedPtr<FMRLatencyViewExtension, ESPMode::ThreadSafe> ThisPtr = SharedThis(this);
		ENQUEUE_RENDER_COMMAND(PreMRCaptureCommand)(
			[ThisPtr, Scene](FRHICommandListImmediate& /*RHICmdList*/)
			{
				ThisPtr->Apply_RenderThread(Scene);
			}
		);
	}
	return bSimulateLatency;
}

//------------------------------------------------------------------------------
void FMRLatencyViewExtension::SetupPostCapture(FSceneInterface* Scene)
{
	const bool bPreCommandEnqueued = (CachedRenderDelay > 0);
	if (bPreCommandEnqueued)
	{
		TSharedPtr<FMRLatencyViewExtension, ESPMode::ThreadSafe> ThisPtr = SharedThis(this);
		ENQUEUE_RENDER_COMMAND(PostMRCaptureCommand)(
			[ThisPtr, Scene](FRHICommandListImmediate& /*RHICmdList*/)
			{
				ThisPtr->Restore_RenderThread(Scene);
			}
		);
	}
}

//------------------------------------------------------------------------------
uint32 FMRLatencyViewExtension::GetDesiredDelay() const
{
	uint32 DesiredDelay = MRLatencyViewExtension_Impl::CVarMotionCaptureLatencyOverride.GetValueOnGameThread();
	if (DesiredDelay <= 0 && Owner.IsValid())
	{
		DesiredDelay = Owner->TrackingLatency;
	}
	return DesiredDelay;
}

//------------------------------------------------------------------------------
void FMRLatencyViewExtension::BeginRenderViewFamily(FSceneViewFamily& ViewFamily)
{
	CachedRenderDelay = GetDesiredDelay();
	FMotionDelayClient::BeginRenderViewFamily(ViewFamily);
}

//------------------------------------------------------------------------------
bool FMRLatencyViewExtension::IsActiveThisFrame(class FViewport* InViewport) const
{
	return Owner.IsValid() && Owner->bIsActive && FMotionDelayClient::IsActiveThisFrame(InViewport);
}
