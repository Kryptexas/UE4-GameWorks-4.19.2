// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	MetalCommandQueue.cpp: Metal command queue wrapper.
=============================================================================*/

#include "MetalRHIPrivate.h"

#include "MetalCommandQueue.h"
#include "MetalCommandList.h"
#include "MetalProfiler.h"
#if METAL_STATISTICS
#include "MetalStatistics.h"
#include "ModuleManager.h"
#endif

#pragma mark - Public C++ Boilerplate -

FMetalCommandQueue::FMetalCommandQueue(id<MTLDevice> Device, uint32 const MaxNumCommandBuffers /* = 0 */)
: CommandQueue(nil)
#if METAL_STATISTICS
, Statistics(nullptr)
#endif
{
	if(MaxNumCommandBuffers == 0)
	{
		CommandQueue = [Device newCommandQueue];
	}
	else
	{
		CommandQueue = [Device newCommandQueueWithMaxCommandBufferCount: MaxNumCommandBuffers];
	}
	check(CommandQueue);

#if METAL_STATISTICS
	IMetalStatisticsModule* StatsModule = FModuleManager::Get().LoadModulePtr<IMetalStatisticsModule>(TEXT("MetalStatistics"));
	
	if(StatsModule && (![Device.name containsString:@"AMD"] || FParse::Param(FCommandLine::Get(),TEXT("metalstats"))))
	{
		Statistics = StatsModule->CreateMetalStatistics(CommandQueue);
		if(!Statistics->SupportsStatistics())
		{
			delete Statistics;
			Statistics = nullptr;
		}
	}
#endif
}

FMetalCommandQueue::~FMetalCommandQueue(void)
{
#if METAL_STATISTICS
	delete Statistics;
#endif
	
	[CommandQueue release];
	CommandQueue = nil;
}
	
#pragma mark - Public Command Buffer Mutators -

id<MTLCommandBuffer> FMetalCommandQueue::CreateRetainedCommandBuffer(void)
{
	@autoreleasepool
	{
		id<MTLCommandBuffer> CmdBuffer = [[CommandQueue commandBuffer] retain];
		TRACK_OBJECT(STAT_MetalCommandBufferCount, CmdBuffer);
		return CmdBuffer;
	}
}

id<MTLCommandBuffer> FMetalCommandQueue::CreateUnretainedCommandBuffer(void)
{
	static bool bUnretainedRefs = !FParse::Param(FCommandLine::Get(),TEXT("metalretainrefs"));
	id<MTLCommandBuffer> CmdBuffer = nil;
	@autoreleasepool
	{
		CmdBuffer = bUnretainedRefs ? [[CommandQueue commandBufferWithUnretainedReferences] retain] : [[CommandQueue commandBuffer] retain];
	}
	TRACK_OBJECT(STAT_MetalCommandBufferCount, CmdBuffer);
	return CmdBuffer;
}

void FMetalCommandQueue::CommitCommandBuffer(id<MTLCommandBuffer> const CommandBuffer)
{
	check(CommandBuffer);
	UNTRACK_OBJECT(STAT_MetalCommandBufferCount, CommandBuffer);
	
	[CommandBuffer commit];
	[CommandBuffer release];
}

void FMetalCommandQueue::SubmitCommandBuffers(NSArray<id<MTLCommandBuffer>>* BufferList, uint32 Index, uint32 Count)
{
	check(BufferList);
	CommandBuffers.SetNumZeroed(Count);
	CommandBuffers[Index] = BufferList;
	bool bComplete = true;
	for (uint32 i = 0; i < Count; i++)
	{
		bComplete &= (CommandBuffers[i] != nullptr);
	}
	if (bComplete)
	{
		GetMetalDeviceContext().SubmitCommandsHint();
		
		for (uint32 i = 0; i < Count; i++)
		{
			NSArray<id<MTLCommandBuffer>>* CmdBuffers = CommandBuffers[i];
			check(CmdBuffers);
			for (id<MTLCommandBuffer> Buffer in CmdBuffers)
			{
				check(Buffer);
				CommitCommandBuffer(Buffer);
			}
			[CommandBuffers[i] release];
			CommandBuffers[i] = nullptr;
		}
	}
}

#pragma mark - Public Command Queue Accessors -
	
id<MTLDevice> FMetalCommandQueue::GetDevice(void) const
{
	return CommandQueue.device;
}

#pragma mark - Public Debug Support -

void FMetalCommandQueue::InsertDebugCaptureBoundary(void)
{
	[CommandQueue insertDebugCaptureBoundary];
}

#if METAL_STATISTICS
#pragma mark - Public Statistics Extensions -

IMetalStatistics* FMetalCommandQueue::GetStatistics(void)
{
	return Statistics;
}
#endif
