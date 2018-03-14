// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
D3D12Stats.cpp:RHI Stats and timing implementation.
=============================================================================*/

#include "D3D12RHIPrivate.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

namespace D3D12RHI
{
	void FD3DGPUProfiler::BeginFrame(FD3D12DynamicRHI* InRHI)
	{
		CurrentEventNode = NULL;
		check(!bTrackingEvents);
		check(!CurrentEventNodeFrame); // this should have already been cleaned up and the end of the previous frame

									   // latch the bools from the game thread into our private copy
		bLatchedGProfilingGPU = GTriggerGPUProfile;
		bLatchedGProfilingGPUHitches = GTriggerGPUHitchProfile;
		if (bLatchedGProfilingGPUHitches)
		{
			bLatchedGProfilingGPU = false; // we do NOT permit an ordinary GPU profile during hitch profiles
		}

		// if we are starting a hitch profile or this frame is a gpu profile, then save off the state of the draw events
		if (bLatchedGProfilingGPU || (!bPreviousLatchedGProfilingGPUHitches && bLatchedGProfilingGPUHitches))
		{
			bOriginalGEmitDrawEvents = GetEmitDrawEvents();
		}

		if (bLatchedGProfilingGPU || bLatchedGProfilingGPUHitches)
		{
			if (bLatchedGProfilingGPUHitches && GPUHitchDebounce)
			{
				// if we are doing hitches and we had a recent hitch, wait to recover
				// the reasoning is that collecting the hitch report may itself hitch the GPU
				GPUHitchDebounce--;
			}
			else
			{
				SetEmitDrawEvents(true);  // thwart an attempt to turn this off on the game side
				bTrackingEvents = true;
				CurrentEventNodeFrame = new FD3D12EventNodeFrame(GetParentAdapter());
				CurrentEventNodeFrame->StartFrame();
			}
		}
		else if (bPreviousLatchedGProfilingGPUHitches)
		{
			// hitch profiler is turning off, clear history and restore draw events
			GPUHitchEventNodeFrames.Empty();
			SetEmitDrawEvents(bOriginalGEmitDrawEvents);
		}
		bPreviousLatchedGProfilingGPUHitches = bLatchedGProfilingGPUHitches;

		FrameTiming.StartTiming();

		if (GetEmitDrawEvents())
		{
			PushEvent(TEXT("FRAME"), FColor(0, 255, 0, 255));
		}
	}
}

void FD3DGPUProfiler::EndFrame(FD3D12DynamicRHI* InRHI)
{
	if (GetEmitDrawEvents())
	{
		PopEvent();
		check(StackDepth == 0);
	}

	FrameTiming.EndTiming();

	if (FrameTiming.IsSupported())
	{
		uint64 GPUTiming = FrameTiming.GetTiming();
		uint64 GPUFreq = FrameTiming.GetTimingFrequency();
		GGPUFrameTime = FMath::TruncToInt(double(GPUTiming) / double(GPUFreq) / FPlatformTime::GetSecondsPerCycle());
	}
	else
	{
		GGPUFrameTime = 0;
	}

	double HwGpuFrameTime = 0.0;
	if (InRHI->GetHardwareGPUFrameTime(HwGpuFrameTime))
	{
		GGPUFrameTime = HwGpuFrameTime;
	}

	// if we have a frame open, close it now.
	if (CurrentEventNodeFrame)
	{
		CurrentEventNodeFrame->EndFrame();
	}

	check(!bTrackingEvents || bLatchedGProfilingGPU || bLatchedGProfilingGPUHitches);
	check(!bTrackingEvents || CurrentEventNodeFrame);
	if (bLatchedGProfilingGPU)
	{
		if (bTrackingEvents)
		{
			SetEmitDrawEvents(bOriginalGEmitDrawEvents);
			UE_LOG(LogD3D12RHI, Log, TEXT(""));
			UE_LOG(LogD3D12RHI, Log, TEXT(""));
			CurrentEventNodeFrame->DumpEventTree();
			GTriggerGPUProfile = false;
			bLatchedGProfilingGPU = false;

			if (RHIConfig::ShouldSaveScreenshotAfterProfilingGPU()
				&& GEngine->GameViewport)
			{
				GEngine->GameViewport->Exec(NULL, TEXT("SCREENSHOT"), *GLog);
			}
		}
	}
	else if (bLatchedGProfilingGPUHitches)
	{
		//@todo this really detects any hitch, even one on the game thread.
		// it would be nice to restrict the test to stalls on D3D, but for now...
		// this needs to be out here because bTrackingEvents is false during the hitch debounce
		static double LastTime = -1.0;
		double Now = FPlatformTime::Seconds();
		if (bTrackingEvents)
		{
			/** How long, in seconds a frame much be to be considered a hitch **/
			const float HitchThreshold = RHIConfig::GetGPUHitchThreshold();
			float ThisTime = Now - LastTime;
			bool bHitched = (ThisTime > HitchThreshold) && LastTime > 0.0 && CurrentEventNodeFrame;
			if (bHitched)
			{
				UE_LOG(LogD3D12RHI, Warning, TEXT("*******************************************************************************"));
				UE_LOG(LogD3D12RHI, Warning, TEXT("********** Hitch detected on CPU, frametime = %6.1fms"), ThisTime * 1000.0f);
				UE_LOG(LogD3D12RHI, Warning, TEXT("*******************************************************************************"));

				for (int32 Frame = 0; Frame < GPUHitchEventNodeFrames.Num(); Frame++)
				{
					UE_LOG(LogD3D12RHI, Warning, TEXT(""));
					UE_LOG(LogD3D12RHI, Warning, TEXT(""));
					UE_LOG(LogD3D12RHI, Warning, TEXT("********** GPU Frame: Current - %d"), GPUHitchEventNodeFrames.Num() - Frame);
					GPUHitchEventNodeFrames[Frame].DumpEventTree();
				}
				UE_LOG(LogD3D12RHI, Warning, TEXT(""));
				UE_LOG(LogD3D12RHI, Warning, TEXT(""));
				UE_LOG(LogD3D12RHI, Warning, TEXT("********** GPU Frame: Current"));
				CurrentEventNodeFrame->DumpEventTree();

				UE_LOG(LogD3D12RHI, Warning, TEXT("*******************************************************************************"));
				UE_LOG(LogD3D12RHI, Warning, TEXT("********** End Hitch GPU Profile"));
				UE_LOG(LogD3D12RHI, Warning, TEXT("*******************************************************************************"));
				if (GEngine->GameViewport)
				{
					GEngine->GameViewport->Exec(NULL, TEXT("SCREENSHOT"), *GLog);
				}

				GPUHitchDebounce = 5; // don't trigger this again for a while
				GPUHitchEventNodeFrames.Empty(); // clear history
			}
			else if (CurrentEventNodeFrame) // this will be null for discarded frames while recovering from a recent hitch
			{
				/** How many old frames to buffer for hitch reports **/
				static const int32 HitchHistorySize = 4;

				if (GPUHitchEventNodeFrames.Num() >= HitchHistorySize)
				{
					GPUHitchEventNodeFrames.RemoveAt(0);
				}
				GPUHitchEventNodeFrames.Add((FD3D12EventNodeFrame*)CurrentEventNodeFrame);
				CurrentEventNodeFrame = NULL;  // prevent deletion of this below; ke kept it in the history
			}
		}
		LastTime = Now;
	}
	bTrackingEvents = false;
	delete CurrentEventNodeFrame;
	CurrentEventNodeFrame = NULL;
}

void FD3DGPUProfiler::PushEvent(const TCHAR* Name, FColor Color)
{
#if WITH_DX_PERF
	D3DPERF_BeginEvent(Color.DWColor(), Name);
#endif

	FGPUProfiler::PushEvent(Name, Color);
}

void FD3DGPUProfiler::PopEvent()
{
#if WITH_DX_PERF
	D3DPERF_EndEvent();
#endif

	FGPUProfiler::PopEvent();
}

/** Start this frame of per tracking */
void FD3D12EventNodeFrame::StartFrame()
{
	EventTree.Reset();
	RootEventTiming.StartTiming();
}

/** End this frame of per tracking, but do not block yet */
void FD3D12EventNodeFrame::EndFrame()
{
	RootEventTiming.EndTiming();
}

float FD3D12EventNodeFrame::GetRootTimingResults()
{
	double RootResult = 0.0f;
	if (RootEventTiming.IsSupported())
	{
		const uint64 GPUTiming = RootEventTiming.GetTiming(true);
		const uint64 GPUFreq = RootEventTiming.GetTimingFrequency();

		RootResult = double(GPUTiming) / double(GPUFreq);
	}

	return (float)RootResult;
}

void FD3D12EventNodeFrame::LogDisjointQuery()
{
}

float FD3D12EventNode::GetTiming()
{
	float Result = 0;

	if (Timing.IsSupported())
	{
		// Get the timing result and block the CPU until it is ready
		const uint64 GPUTiming = Timing.GetTiming(true);
		const uint64 GPUFreq = Timing.GetTimingFrequency();

		Result = double(GPUTiming) / double(GPUFreq);
	}

	return Result;
}

void UpdateBufferStats(FD3D12ResourceLocation* ResourceLocation, bool bAllocating, uint32 BufferType)
{
	uint64 RequestedSize = ResourceLocation->GetSize();

	const bool bUniformBuffer = BufferType == D3D12_BUFFER_TYPE_CONSTANT;
	const bool bIndexBuffer = BufferType == D3D12_BUFFER_TYPE_INDEX;
	const bool bVertexBuffer = BufferType == D3D12_BUFFER_TYPE_VERTEX;

	if (bAllocating)
	{
		if (bUniformBuffer)
		{
			INC_MEMORY_STAT_BY(STAT_UniformBufferMemory, RequestedSize);
		}
		else if (bIndexBuffer)
		{
			INC_MEMORY_STAT_BY(STAT_IndexBufferMemory, RequestedSize);
		}
		else if (bVertexBuffer)
		{
			INC_MEMORY_STAT_BY(STAT_VertexBufferMemory, RequestedSize);
		}
		else
		{
			INC_MEMORY_STAT_BY(STAT_StructuredBufferMemory, RequestedSize);
		}

#if PLATFORM_WINDOWS
		// this is a work-around on Windows. Due to the fact that there is no way
		// to hook the actual d3d allocations it is very difficult to track memory
		// in the normal way. The problem is that some buffers are allocated from
		// the allocators and some are allocated from the device. Ideally this
		// tracking would be moved to where the actual d3d resource is created and
		// released and the tracking could be re-enabled in the buddy allocator.
		// The problem is that the releasing of resources happens in a generic way
		// (see FD3D12ResourceLocation) 
		LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ELLMTag::Meshes, RequestedSize, ELLMTracker::Default, ELLMAllocType::None);
		LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ELLMTag::GraphicsPlatform, RequestedSize, ELLMTracker::Platform, ELLMAllocType::None);
#endif
	}
	else
	{
		if (bUniformBuffer)
		{
			DEC_MEMORY_STAT_BY(STAT_UniformBufferMemory, RequestedSize);
		}
		else if (bIndexBuffer)
		{
			DEC_MEMORY_STAT_BY(STAT_IndexBufferMemory, RequestedSize);
		}
		else if (bVertexBuffer)
		{
			DEC_MEMORY_STAT_BY(STAT_VertexBufferMemory, RequestedSize);
		}
		else
		{
			DEC_MEMORY_STAT_BY(STAT_StructuredBufferMemory, RequestedSize);
		}

#if PLATFORM_WINDOWS
		// this is a work-around on Windows. See comment above.
		LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ELLMTag::Meshes, -(int64)RequestedSize, ELLMTracker::Default, ELLMAllocType::None);
		LLM_SCOPED_PAUSE_TRACKING_WITH_ENUM_AND_AMOUNT(ELLMTag::GraphicsPlatform, -(int64)RequestedSize, ELLMTracker::Platform, ELLMAllocType::None);
#endif
	}
}
