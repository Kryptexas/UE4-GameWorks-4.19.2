// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanUtil.cpp: Vulkan Utility implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanUtil.h"
#include "VulkanPendingState.h"
#include "VulkanContext.h"
#include "VulkanMemory.h"
#include "PipelineStateCache.h"

extern CORE_API bool GIsGPUCrashed;

/**
 * Initializes the static variables, if necessary.
 */
void FVulkanGPUTiming::PlatformStaticInitialize(void* UserData)
{
	// Are the static variables initialized?
	check( !GAreGlobalsInitialized );

	FVulkanGPUTiming* Caller = (FVulkanGPUTiming*)UserData;
	if (Caller && Caller->Device)
	{
		const VkPhysicalDeviceLimits& Limits = Caller->Device->GetDeviceProperties().limits;
		bool bSupportsTimestamps = (Limits.timestampComputeAndGraphics == VK_TRUE);
		if (!bSupportsTimestamps)
		{
			UE_LOG(LogVulkanRHI, Warning, TEXT("Timestamps not supported on Device"));
			return;
		}
		GTimingFrequency = 1;
	}
}

/**
 * Initializes all Vulkan resources and if necessary, the static variables.
 */
void FVulkanGPUTiming::Initialize()
{
	StaticInitialize(this, PlatformStaticInitialize);

	bIsTiming = false;

	// Now initialize the queries for this timing object.
	if ( GIsSupported )
	{
		for (int32 Index = 0; Index < MaxTimers; ++Index)
		{
			Timers[Index].Begin = new FOLDVulkanRenderQuery(CmdContext->GetDevice(), RQT_AbsoluteTime);
			Timers[Index].End = new FOLDVulkanRenderQuery(CmdContext->GetDevice(), RQT_AbsoluteTime);
		}
	}
}

/**
 * Releases all Vulkan resources.
 */
void FVulkanGPUTiming::Release()
{
	for (int32 Index = 0; Index < MaxTimers; ++Index)
	{
		delete Timers[Index].Begin;
		delete Timers[Index].End;
	}

	FMemory::Memzero(Timers);
}

/**
 * Start a GPU timing measurement.
 */
void FVulkanGPUTiming::StartTiming(FVulkanCmdBuffer* CmdBuffer)
{
	// Issue a timestamp query for the 'start' time.
	if ( GIsSupported && !bIsTiming )
	{
		CurrentTimerIndex = (CurrentTimerIndex + 1) % MaxTimers;
		NumActiveTimers = FMath::Min(NumActiveTimers + 1, (int32)MaxTimers);
		if (CmdBuffer == nullptr)
		{
			CmdBuffer = CmdContext->GetCommandBufferManager()->GetActiveCmdBuffer();
		}
		CmdContext->EndRenderQueryInternal(CmdBuffer, Timers[CurrentTimerIndex].Begin);
		bIsTiming = true;
	}
}

/**
 * End a GPU timing measurement.
 * The timing for this particular measurement will be resolved at a later time by the GPU.
 */
void FVulkanGPUTiming::EndTiming(FVulkanCmdBuffer* CmdBuffer)
{
	// Issue a timestamp query for the 'end' time.
	if (GIsSupported && bIsTiming)
	{
		if (CmdBuffer == nullptr)
		{
			CmdBuffer = CmdContext->GetCommandBufferManager()->GetActiveCmdBuffer();
		}
		CmdContext->EndRenderQueryInternal(CmdBuffer, Timers[CurrentTimerIndex].End);
		bIsTiming = false;
		bEndTimestampIssued = true;
	}
}

/**
 * Retrieves the most recently resolved timing measurement.
 * The unit is the same as for FPlatformTime::Cycles(). Returns 0 if there are no resolved measurements.
 *
 * @return	Value of the most recently resolved timing, or 0 if no measurements have been resolved by the GPU yet.
 */
uint64 FVulkanGPUTiming::GetTiming(bool bGetCurrentResultsAndBlock)
{
	if (GIsSupported)
	{
		uint64 BeginTime, EndTime;
		int32 TimerIndex = CurrentTimerIndex;
		if (!bGetCurrentResultsAndBlock)
		{
			// Go backwards through the list
			for (int32 Index = 1; Index < NumActiveTimers; ++Index)
			{
				{
					check(Device == CmdContext->GetDevice());
					if (Timers[TimerIndex].Begin->GetResult(Device, BeginTime, false))
					{
						if (Timers[TimerIndex].End->GetResult(Device, EndTime, false))
						{
							if (BeginTime < EndTime)
							{
								return (EndTime - BeginTime);
							}
						}
						else
						{
							int i = 0;
							++i;
						}
					}
				}

				// Go back
				TimerIndex = (TimerIndex + MaxTimers - 1) % MaxTimers;
			}
		}

		if (bGetCurrentResultsAndBlock)
		{
			TimerIndex = CurrentTimerIndex;
			if (Timers[TimerIndex].Begin->GetResult(Device, BeginTime, true))
			{
				if (Timers[TimerIndex].End->GetResult(Device, EndTime, true))
				{
					if (BeginTime < EndTime)
					{
						return (EndTime - BeginTime);
					}
				}
				else
				{
					checkf(0, TEXT("Could not wait for End timer query result!"));
				}
			}
			else
			{
				checkf(0, TEXT("Could not wait for Begin timer query result!"));
			}
		}
	}

	return 0;
}

static double ConvertTiming(uint64 Delta)
{
	return Delta / 1e6;
}

/** Start this frame of per tracking */
void FVulkanEventNodeFrame::StartFrame()
{
	EventTree.Reset();
	RootEventTiming.StartTiming();
}

/** End this frame of per tracking, but do not block yet */
void FVulkanEventNodeFrame::EndFrame()
{
	RootEventTiming.EndTiming();
}

float FVulkanEventNodeFrame::GetRootTimingResults()
{
	double RootResult = 0.0f;
	if (RootEventTiming.IsSupported())
	{
		const uint64 GPUTiming = RootEventTiming.GetTiming(true);

		RootResult = ConvertTiming(GPUTiming);
	}

	return (float)RootResult;
}

float FVulkanEventNode::GetTiming()
{
	float Result = 0;

	if (Timing.IsSupported())
	{
		const uint64 GPUTiming = Timing.GetTiming(true);
		Result = ConvertTiming(GPUTiming);
	}

	return Result;
}


void FVulkanGPUProfiler::BeginFrame()
{
	bCommandlistSubmitted = false;
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
			CurrentEventNodeFrame = new FVulkanEventNodeFrame(CmdContext, Device);
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

	if (GetEmitDrawEvents())
	{
		PushEvent(TEXT("FRAME"), FColor(0, 255, 0, 255));
	}
}

void FVulkanGPUProfiler::EndFrameBeforeSubmit()
{
	if (GetEmitDrawEvents())
	{
		// Finish all open nodes
		// This is necessary because timestamps must be issued before SubmitDone(), and SubmitDone() happens in RHIEndDrawingViewport instead of RHIEndFrame
		while (CurrentEventNode)
		{
			UE_LOG(LogRHI, Warning, TEXT("POPPING BEFORE SUB"));
			PopEvent();
		}

		bCommandlistSubmitted = true;
	}

	// if we have a frame open, close it now.
	if (CurrentEventNodeFrame)
	{
		CurrentEventNodeFrame->EndFrame();
	}
}

void FVulkanGPUProfiler::EndFrame()
{
	EndFrameBeforeSubmit();

	check(!bTrackingEvents || bLatchedGProfilingGPU || bLatchedGProfilingGPUHitches);
	if (bLatchedGProfilingGPU)
	{
		if (bTrackingEvents)
		{
			CmdContext->GetDevice()->SubmitCommandsAndFlushGPU();

			SetEmitDrawEvents(bOriginalGEmitDrawEvents);
			UE_LOG(LogRHI, Warning, TEXT(""));
			UE_LOG(LogRHI, Warning, TEXT(""));
			check(CurrentEventNodeFrame);
			CurrentEventNodeFrame->DumpEventTree();
			GTriggerGPUProfile = false;
			bLatchedGProfilingGPU = false;
		}
	}
	else if (bLatchedGProfilingGPUHitches)
	{
		UE_LOG(LogRHI, Warning, TEXT("GPU hitch tracking not implemented on Vulkan"));
	}
	bTrackingEvents = false;
	if (CurrentEventNodeFrame)
	{
		delete CurrentEventNodeFrame;
		CurrentEventNodeFrame = nullptr;
	}
}


#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "OneColorShader.h"
#include "VulkanRHI.h"
#include "StaticBoundShaderState.h"

namespace VulkanRHI
{
	VkBuffer CreateBuffer(FVulkanDevice* InDevice, VkDeviceSize Size, VkBufferUsageFlags BufferUsageFlags, VkMemoryRequirements& OutMemoryRequirements)
	{
		VkDevice Device = InDevice->GetInstanceHandle();
		VkBuffer Buffer = VK_NULL_HANDLE;

		VkBufferCreateInfo BufferCreateInfo;
		FMemory::Memzero(BufferCreateInfo);
		BufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		BufferCreateInfo.size = Size;
		BufferCreateInfo.usage = BufferUsageFlags;
		VERIFYVULKANRESULT_EXPANDED(VulkanRHI::vkCreateBuffer(Device, &BufferCreateInfo, nullptr, &Buffer));

		VulkanRHI::vkGetBufferMemoryRequirements(Device, Buffer, &OutMemoryRequirements);

		return Buffer;
	}

	/**
	 * Checks that the given result isn't a failure.  If it is, the application exits with an appropriate error message.
	 * @param	Result - The result code to check
	 * @param	Code - The code which yielded the result.
	 * @param	VkFunction - Tested function name.
	 * @param	Filename - The filename of the source file containing Code.
	 * @param	Line - The line number of Code within Filename.
	 */
	void VerifyVulkanResult(VkResult Result, const ANSICHAR* VkFunction, const ANSICHAR* Filename, uint32 Line)
	{
		FString ErrorString;
		switch (Result)
		{
#define VKERRORCASE(x)	case x: ErrorString = TEXT(#x)
		VKERRORCASE(VK_NOT_READY); break;
		VKERRORCASE(VK_TIMEOUT); break;
		VKERRORCASE(VK_EVENT_SET); break;
		VKERRORCASE(VK_EVENT_RESET); break;
		VKERRORCASE(VK_INCOMPLETE); break;
		VKERRORCASE(VK_ERROR_OUT_OF_HOST_MEMORY); break;
		VKERRORCASE(VK_ERROR_OUT_OF_DEVICE_MEMORY); break;
		VKERRORCASE(VK_ERROR_INITIALIZATION_FAILED); break;
		VKERRORCASE(VK_ERROR_DEVICE_LOST); GIsGPUCrashed = true; break;
		VKERRORCASE(VK_ERROR_MEMORY_MAP_FAILED); break;
		VKERRORCASE(VK_ERROR_LAYER_NOT_PRESENT); break;
		VKERRORCASE(VK_ERROR_EXTENSION_NOT_PRESENT); break;
		VKERRORCASE(VK_ERROR_FEATURE_NOT_PRESENT); break;
		VKERRORCASE(VK_ERROR_INCOMPATIBLE_DRIVER); break;
		VKERRORCASE(VK_ERROR_TOO_MANY_OBJECTS); break;
		VKERRORCASE(VK_ERROR_FORMAT_NOT_SUPPORTED); break;
		VKERRORCASE(VK_ERROR_SURFACE_LOST_KHR); break;
		VKERRORCASE(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR); break;
		VKERRORCASE(VK_SUBOPTIMAL_KHR); break;
		VKERRORCASE(VK_ERROR_OUT_OF_DATE_KHR); break;
		VKERRORCASE(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR); break;
		VKERRORCASE(VK_ERROR_VALIDATION_FAILED_EXT); break;
#if VK_HEADER_VERSION >= 13
		VKERRORCASE(VK_ERROR_INVALID_SHADER_NV); break;
#endif
#if VK_HEADER_VERSION >= 24
		VKERRORCASE(VK_ERROR_FRAGMENTED_POOL); break;
#endif
#if VK_HEADER_VERSION >= 39
		VKERRORCASE(VK_ERROR_OUT_OF_POOL_MEMORY_KHR); break;
#endif
#if VK_HEADER_VERSION >= 65
		VKERRORCASE(VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR); break;
		VKERRORCASE(VK_ERROR_NOT_PERMITTED_EXT); break;
#endif
#undef VKERRORCASE
		default:
			break;
		}

		UE_LOG(LogVulkanRHI, Error, TEXT("%s failed, VkResult=%d\n at %s:%u \n with error %s"),
			ANSI_TO_TCHAR(VkFunction), (int32)Result, ANSI_TO_TCHAR(Filename), Line, *ErrorString);

		//#todo-rco: Don't need this yet...
		//TerminateOnDeviceRemoved(Result);
		//TerminateOnOutOfMemory(Result, false);

		UE_LOG(LogVulkanRHI, Fatal, TEXT("%s failed, VkResult=%d\n at %s:%u \n with error %s"),
			ANSI_TO_TCHAR(VkFunction), (int32)Result, ANSI_TO_TCHAR(Filename), Line, *ErrorString);
	}
}

DEFINE_STAT(STAT_VulkanDrawCallTime);
DEFINE_STAT(STAT_VulkanDispatchCallTime);
DEFINE_STAT(STAT_VulkanDrawCallPrepareTime);
DEFINE_STAT(STAT_VulkanDispatchCallPrepareTime);
DEFINE_STAT(STAT_VulkanGetOrCreatePipeline);
DEFINE_STAT(STAT_VulkanGetDescriptorSet);
DEFINE_STAT(STAT_VulkanPipelineBind);
DEFINE_STAT(STAT_VulkanNumBoundShaderState);
DEFINE_STAT(STAT_VulkanNumRenderPasses);
DEFINE_STAT(STAT_VulkanNumFrameBuffers);
DEFINE_STAT(STAT_VulkanNumBufferViews);
DEFINE_STAT(STAT_VulkanNumImageViews);
DEFINE_STAT(STAT_VulkanNumPhysicalMemAllocations);
DEFINE_STAT(STAT_VulkanDynamicVBSize);
DEFINE_STAT(STAT_VulkanDynamicIBSize);
DEFINE_STAT(STAT_VulkanDynamicVBLockTime);
DEFINE_STAT(STAT_VulkanDynamicIBLockTime);
DEFINE_STAT(STAT_VulkanUPPrepTime);
DEFINE_STAT(STAT_VulkanUniformBufferCreateTime);
DEFINE_STAT(STAT_VulkanApplyDSUniformBuffers);
DEFINE_STAT(STAT_VulkanSRVUpdateTime);
DEFINE_STAT(STAT_VulkanUAVUpdateTime);
DEFINE_STAT(STAT_VulkanDeletionQueue);
DEFINE_STAT(STAT_VulkanQueueSubmit);
DEFINE_STAT(STAT_VulkanQueuePresent);
DEFINE_STAT(STAT_VulkanNumQueries);
DEFINE_STAT(STAT_VulkanWaitQuery);
DEFINE_STAT(STAT_VulkanWaitFence);
DEFINE_STAT(STAT_VulkanResetQuery);
DEFINE_STAT(STAT_VulkanWaitSwapchain);
DEFINE_STAT(STAT_VulkanAcquireBackBuffer);
DEFINE_STAT(STAT_VulkanStagingBuffer);
DEFINE_STAT(STAT_VulkanVkCreateDescriptorPool);
DEFINE_STAT(STAT_VulkanNumDescPools);
DEFINE_STAT(STAT_VulkanDescriptorSetAllocator);
#if VULKAN_ENABLE_AGGRESSIVE_STATS
DEFINE_STAT(STAT_VulkanUpdateDescriptorSets);
DEFINE_STAT(STAT_VulkanNumUpdateDescriptors);
DEFINE_STAT(STAT_VulkanNumDescSets);
DEFINE_STAT(STAT_VulkanSetUniformBufferTime);
DEFINE_STAT(STAT_VulkanVkUpdateDS);
DEFINE_STAT(STAT_VulkanBindVertexStreamsTime);
#endif
DEFINE_STAT(STAT_VulkanNumDescSetsTotal);
