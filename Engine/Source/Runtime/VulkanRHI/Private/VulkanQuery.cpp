// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanQuery.cpp: Vulkan query RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"

enum
{
	NUM_FRAMES_TO_WAIT_BEFORE_COMPACTING_QUERIES = 30,
};

struct FRHISubmitCmdBuffer : public FRHICommand<FRHISubmitCmdBuffer>
{
	FVulkanCommandBufferManager* CmdBufferMgr;
	FVulkanCmdBuffer* ActiveCmdBuffer;

	FORCEINLINE_DEBUGGABLE FRHISubmitCmdBuffer(FVulkanCommandBufferManager* InCmdMgr, FVulkanCmdBuffer* InActiveCmdBuffer)
		: CmdBufferMgr(InCmdMgr)
		, ActiveCmdBuffer(InActiveCmdBuffer)
	{
	}

	void Execute(FRHICommandListBase& CmdList)
	{
		if (CmdBufferMgr->HasPendingActiveCmdBuffer())
		{
			CmdBufferMgr->SubmitActiveCmdBuffer(true);
			CmdBufferMgr->PrepareForNewActiveCommandBuffer();
		}
	}
};


FVulkanRenderQuery::FVulkanRenderQuery(FVulkanDevice* Device, ERenderQueryType InQueryType)
	: QueryIndex(-1)
	, QueryType(InQueryType)
	, QueryPool(nullptr)
	, CurrentCmdBuffer(nullptr)
	, CurrentBatch(-1)
{
}

inline void FVulkanRenderQuery::Begin(FVulkanCmdBuffer* CmdBuffer)
{
	CurrentCmdBuffer = CmdBuffer;
	ensure(QueryIndex != -1);
	vkCmdBeginQuery(CmdBuffer->GetHandle(), QueryPool->GetHandle(), QueryIndex, 0);
}

inline void FVulkanRenderQuery::End(FVulkanCmdBuffer* CmdBuffer)
{
	ensure(CurrentCmdBuffer == CmdBuffer);
	ensure(QueryIndex != -1);
	vkCmdEndQuery(CmdBuffer->GetHandle(), QueryPool->GetHandle(), QueryIndex);
}


FVulkanQueryPool::FVulkanQueryPool(FVulkanDevice* InDevice, uint32 InNumQueries, VkQueryType InQueryType)
	: VulkanRHI::FDeviceChild(InDevice)
	, QueryPool(VK_NULL_HANDLE)
	, NumQueries(InNumQueries)
	, QueryType(InQueryType)
{
	check(InDevice);

	VkQueryPoolCreateInfo PoolCreateInfo;
	FMemory::Memzero(PoolCreateInfo);
	PoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	PoolCreateInfo.queryType = QueryType;
	PoolCreateInfo.queryCount = NumQueries;

	VERIFYVULKANRESULT(VulkanRHI::vkCreateQueryPool(Device->GetInstanceHandle(), &PoolCreateInfo, nullptr, &QueryPool));
}

FVulkanQueryPool::~FVulkanQueryPool()
{
	check(QueryPool == VK_NULL_HANDLE);
}

void FVulkanQueryPool::Destroy()
{
	VulkanRHI::vkDestroyQueryPool(Device->GetInstanceHandle(), QueryPool, nullptr);
	QueryPool = VK_NULL_HANDLE;
}

void FVulkanQueryPool::Reset(FVulkanCmdBuffer* CmdBuffer)
{
	VulkanRHI::vkCmdResetQueryPool(CmdBuffer->GetHandle(), QueryPool, 0, NumQueries);;
}


inline bool FVulkanOcclusionQueryPool::FBatch::HasFenceBeenSignaled() const
{
	switch (State)
	{
	case ERead:
		return true;

	case EBatchEnded:
		check(CmdBuffer);
		return Fence < CmdBuffer->GetFenceSignaledCounter();

	default:
		check(0);
		break;
	}
	return false;
}

void FVulkanOcclusionQueryPool::FBatch::End(FVulkanCmdBuffer* InCmdBuffer)
{
	ensure(State == EBatchBegun);

	CmdBuffer = InCmdBuffer;
	Fence = CmdBuffer->GetFenceSignaledCounter();

	State = EBatchEnded;
}

void FVulkanOcclusionQueryPool::Compact()
{
	bool bAllRead = true;
	for (int32 Index = 0; Index < Batches.Num(); ++Index)
	{
		if (Batches[Index].State != FBatch::ERead && Batches[Index].State != FBatch::EBatchEnded && Batches[Index].State != FBatch::ESubmittedWithoutWait)
		{
			return;
		}
	}

	Batches.SetNum(0, false);
	Batches.Add(FBatch());
	CurrentBatchIndex = 0;
	NextIndexForBatch = 0;
}

bool FVulkanOcclusionQueryPool::GetResults(FVulkanCommandListContext& Context, FVulkanRenderQuery* Query, bool bWait, uint64& OutNumPixels)
{
	FBatch& Batch = Batches[Query->CurrentBatch];
	if (Batch.State == FBatch::EBatchEnded)
	{
		check(this == Query->QueryPool);
		if (bWait)
		{
			FPlatformProcess::SleepNoStats(0);

			// Pump RHIThread to make sure these queries have actually been submitted to the GPU.
			if (IsInActualRenderingThread())
			{
				FTaskGraphInterface::Get().ProcessThreadUntilIdle(ENamedThreads::RenderThread_Local);
			}

			if (!Batch.HasFenceBeenSignaled())
			{
				//UE_LOG(LogRHI, Log, TEXT("Trying to check occlusion query before it's actually been submitted to the GPU!"));
				return false;
			}

			VERIFYVULKANRESULT(VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), QueryPool, Batch.StartIndex, Batch.NumIndices, Batch.NumIndices * sizeof(uint64), &QueryOutput[Batch.StartIndex], sizeof(uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
			Batch.State = FBatch::ERead;
			LastFrameResultsRead = Context.GetFrameCounter();
		}
		else
		{
			ensure(0);
			VkResult Result = VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), QueryPool, Batch.StartIndex, Batch.NumIndices, Batch.NumIndices * sizeof(uint64), &QueryOutput[Batch.StartIndex], sizeof(uint64), VK_QUERY_RESULT_64_BIT);
			VERIFYVULKANRESULT(Result);
			Batch.State = FBatch::ESubmittedWithoutWait;
		}
	}
	else
	{
		check(Batch.State == FBatch::ERead);
		OutNumPixels = QueryOutput[Query->QueryIndex];
		return true;
	}

	return false;
}

FVulkanTimestampQueryPool::FVulkanTimestampQueryPool(FVulkanDevice* InDevice) :
	FVulkanQueryPool(InDevice, TotalQueries, VK_QUERY_TYPE_TIMESTAMP),
	TimeStampsPerSeconds(0),
	SecondsPerTimestamp(0),
	UsedUserQueries(0),
	bFirst(true),
	LastEndCmdBuffer(nullptr),
	LastFenceSignaledCounter(0)
{
	// The number of nanoseconds it takes for a timestamp value to be incremented by 1 can be obtained from VkPhysicalDeviceLimits::timestampPeriod after a call to vkGetPhysicalDeviceProperties.
	double NanoSecondsPerTimestamp = Device->GetDeviceProperties().limits.timestampPeriod;
	checkf(NanoSecondsPerTimestamp > 0, TEXT("Driver said it allowed timestamps but returned invalid period %f!"), NanoSecondsPerTimestamp);
	SecondsPerTimestamp = NanoSecondsPerTimestamp / 1e9;
	TimeStampsPerSeconds = 1e9 / NanoSecondsPerTimestamp;
}

void FVulkanTimestampQueryPool::WriteStartFrame(VkCommandBuffer CmdBuffer)
{
	if (bFirst)
	{
		bFirst = false;
	}
	static_assert(StartFrame + 1 == EndFrame, "Enums required to be contiguous!");

	VulkanRHI::vkCmdResetQueryPool(CmdBuffer, QueryPool, StartFrame, TotalQueries);

	// Start Frame Timestamp
	VulkanRHI::vkCmdWriteTimestamp(CmdBuffer, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, QueryPool, StartFrame);
}

void FVulkanTimestampQueryPool::WriteEndFrame(FVulkanCmdBuffer* CmdBuffer)
{
	if (!bFirst)
	{
		// End Frame Timestamp
		VulkanRHI::vkCmdWriteTimestamp(CmdBuffer->GetHandle(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, QueryPool, EndFrame);
		LastEndCmdBuffer = CmdBuffer;
		LastFenceSignaledCounter = CmdBuffer->GetFenceSignaledCounter();
	}
}

void FVulkanTimestampQueryPool::CalculateFrameTime()
{
	uint64_t Results[2] = { 0, 0 };
	if (!bFirst)
	{
		if (LastEndCmdBuffer && LastFenceSignaledCounter < LastEndCmdBuffer->GetFenceSignaledCounter())
		{
			VkDevice DeviceHandle = Device->GetInstanceHandle();
			VkResult Result;
			Result = VulkanRHI::vkGetQueryPoolResults(DeviceHandle, QueryPool, StartFrame, 2, sizeof(Results), Results, sizeof(uint64), /*VK_QUERY_RESULT_PARTIAL_BIT |*/ VK_QUERY_RESULT_64_BIT);
			if (Result != VK_SUCCESS)
			{
				GGPUFrameTime = 0;
				return;
			}
		}
		else
		{
			return;
		}
	}

	const uint64 Delta = Results[1] - Results[0];
	double ValueInSeconds = (double)Delta * SecondsPerTimestamp;
	GGPUFrameTime = (uint32)(ValueInSeconds / FPlatformTime::GetSecondsPerCycle());
}

void FVulkanTimestampQueryPool::ResetUserQueries()
{
	UsedUserQueries = 0;
}

int32 FVulkanTimestampQueryPool::AllocateUserQuery()
{
	if (UsedUserQueries < MaxNumUser)
	{
		return UsedUserQueries++;
	}

	return -1;
}

void FVulkanTimestampQueryPool::WriteTimestamp(VkCommandBuffer CmdBuffer, int32 UserQuery, VkPipelineStageFlagBits PipelineStageBits)
{
	check(UserQuery != -1);
	VulkanRHI::vkCmdWriteTimestamp(CmdBuffer, PipelineStageBits, QueryPool, StartUser + UserQuery);
}

uint32 FVulkanTimestampQueryPool::CalculateTimeFromUserQueries(int32 UserBegin, int32 UserEnd, bool bWait)
{
	check(UserBegin >= 0 && UserEnd >= 0);
	uint64_t Begin = 0;
	uint64_t End = 0;
	VkDevice DeviceHandle = Device->GetInstanceHandle();
	VkResult Result;

	Result = VulkanRHI::vkGetQueryPoolResults(DeviceHandle, QueryPool, StartUser + UserBegin, 1, sizeof(uint64_t), &Begin, 0, /*(bWait ? VK_QUERY_RESULT_WAIT_BIT : 0) |*/ VK_QUERY_RESULT_64_BIT);
	if (Result != VK_SUCCESS)
	{
		return 0;
	}

	Result = VulkanRHI::vkGetQueryPoolResults(DeviceHandle, QueryPool, StartUser + UserEnd, 1, sizeof(uint64_t), &End, 0, (bWait ? VK_QUERY_RESULT_WAIT_BIT : 0) | VK_QUERY_RESULT_64_BIT);
	if (Result != VK_SUCCESS)
	{
		return 0;
	}

	return End > Begin ? (uint32)(End - Begin) : 0;
}

FRenderQueryRHIRef FVulkanDynamicRHI::RHICreateRenderQuery(ERenderQueryType QueryType)
{
	FVulkanRenderQuery* Query = new FVulkanRenderQuery(Device, QueryType);
	return Query;
}

bool FVulkanDynamicRHI::RHIGetRenderQueryResult(FRenderQueryRHIParamRef QueryRHI,uint64& OutNumPixels,bool bWait)
{
	check(IsInRenderingThread());

	FVulkanRenderQuery* Query = ResourceCast(QueryRHI);
	if (Query->QueryType == RQT_Occlusion)
	{
		FVulkanOcclusionQueryPool* Pool = (FVulkanOcclusionQueryPool*)Query->QueryPool;
		return Pool->GetResults(Device->GetImmediateContext(), Query, bWait, OutNumPixels);
	};

	return false;
}

// Occlusion/Timer queries.
void FVulkanCommandListContext::RHIBeginRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FVulkanRenderQuery* Query = ResourceCast(QueryRHI);

	if (Query->QueryType == RQT_Occlusion)
	{
		FVulkanOcclusionQueryPool& PoolRef = Device->GetAvailableOcclusionQueryPool();
		FVulkanOcclusionQueryPool* Pool = &PoolRef;
		if (CurrentOcclusionQueryPool != Pool)
		{
			FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
			CurrentOcclusionQueryPool->EndBatch(CmdBuffer, FrameCounter);
			Pool->StartBatch();
			CurrentOcclusionQueryPool = Pool;
		}

		Query->QueryPool = Pool;
		Query->CurrentBatch = Pool->CurrentBatchIndex;
		Query->QueryIndex = Pool->Batches[Pool->CurrentBatchIndex].StartIndex + Pool->Batches[Pool->CurrentBatchIndex].NumIndices;
		++Pool->Batches[Pool->CurrentBatchIndex].NumIndices;

		Query->Begin(CommandBufferManager->GetActiveCmdBuffer());
	}
}

void FVulkanCommandListContext::RHIEndRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FVulkanRenderQuery* Query = ResourceCast(QueryRHI);

	if (Query->QueryType == RQT_Occlusion)
	{
		FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
		Query->End(CmdBuffer);
	}
}

void FVulkanCommandListContext::RHIBeginOcclusionQueryBatch()
{
	ensure(IsImmediate());

	ensure(!CurrentOcclusionQueryPool);

	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	ensure(CmdBuffer->IsInsideRenderPass());

	FVulkanOcclusionQueryPool& PoolRef = Device->GetAvailableOcclusionQueryPool();
	CurrentOcclusionQueryPool = &PoolRef;
	CurrentOcclusionQueryPool->StartBatch();
}

void FVulkanCommandListContext::RHIEndOcclusionQueryBatch()
{
	ensure(IsImmediate());

	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	CurrentOcclusionQueryPool->EndBatch(CmdBuffer, FrameCounter);

	RenderPassState.EndRenderPass(CmdBuffer);

	RequestSubmitCurrentCommands();
	SafePointSubmit();

	Device->CompactOcclusionQueryPools();

	CurrentOcclusionQueryPool = nullptr;
}

void FVulkanDevice::CompactOcclusionQueryPools()
{
	static auto* CVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.NumBufferedOcclusionQueries"));
	const int32 NumFrames = CVar ? CVar->GetValueOnRenderThread() : 1;
	uint32 FrameCounter = ImmediateContext->GetFrameCounter();
	for (int32 Index = 0; Index < OcclusionQueryPools.Num(); ++Index)
	{
		FVulkanOcclusionQueryPool* Pool = OcclusionQueryPools[Index];
		if (//GFrameNumberRenderThread > Pool->GetLastFrameResultsRead() + NumFrames ||
			FrameCounter > Pool->GetLastFrameResultsEnded() + NUM_FRAMES_TO_WAIT_BEFORE_COMPACTING_QUERIES)
		{
			Pool->Compact();
		}
	}
}
