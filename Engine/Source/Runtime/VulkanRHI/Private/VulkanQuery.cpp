// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanQuery.cpp: Vulkan query RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "EngineGlobals.h"

struct FRHICommandWaitForFence : public FRHICommand<FRHICommandWaitForFence>
{
	FVulkanCommandBufferManager* CmdBufferMgr;
	FVulkanCmdBuffer* CmdBuffer;
	uint64 FenceCounter;

	FORCEINLINE_DEBUGGABLE FRHICommandWaitForFence(FVulkanCommandBufferManager* InCmdBufferMgr, FVulkanCmdBuffer* InCmdBuffer, uint64 InFenceCounter)
		: CmdBufferMgr(InCmdBufferMgr)
		, CmdBuffer(InCmdBuffer)
		, FenceCounter(InFenceCounter)
	{
	}

	void Execute(FRHICommandListBase& CmdList)
	{
		if (FenceCounter == CmdBuffer->GetFenceSignaledCounter())
		{
			check(CmdBuffer->IsSubmitted());
			CmdBufferMgr->WaitForCmdBuffer(CmdBuffer);
		}
	}
};


FVulkanRenderQuery::FVulkanRenderQuery(FVulkanDevice* Device, ERenderQueryType InQueryType)
	: CurrentQueryIdx(0)
	, QueryType(InQueryType)
	, CurrentCmdBuffer(nullptr)
{
	for (int Index = 0; Index < NumQueries; ++Index)
	{
		QueryIndices[Index] = -1;
		QueryPools[Index] = nullptr;
	}
}

FVulkanRenderQuery::~FVulkanRenderQuery()
{
	for (int Index = 0; Index < NumQueries; ++Index)
	{
		if (QueryIndices[Index] != -1)
		{
			((FVulkanBufferedQueryPool*)QueryPools[Index])->ReleaseQuery(QueryIndices[Index]);
		}
	}
}

inline void FVulkanRenderQuery::Begin(FVulkanCmdBuffer* CmdBuffer)
{
	CurrentCmdBuffer = CmdBuffer;
	ensure(GetActiveQueryIndex() != -1);
	if (QueryType == RQT_Occlusion)
	{
		vkCmdBeginQuery(CmdBuffer->GetHandle(), GetActiveQueryPool()->GetHandle(), GetActiveQueryIndex(), VK_QUERY_CONTROL_PRECISE_BIT);
	}
	else
	{
		ensure(0);
	}
}

inline void FVulkanRenderQuery::End(FVulkanCmdBuffer* CmdBuffer)
{
	ensure(QueryType != RQT_Occlusion || CurrentCmdBuffer == CmdBuffer);
	ensure(GetActiveQueryIndex() != -1);

	if (QueryType == RQT_Occlusion)
	{
		vkCmdEndQuery(CmdBuffer->GetHandle(), GetActiveQueryPool()->GetHandle(), GetActiveQueryIndex());
	}
	else
	{
		VulkanRHI::vkCmdWriteTimestamp(CmdBuffer->GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, GetActiveQueryPool()->GetHandle(), GetActiveQueryIndex());
	}
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

inline bool FVulkanBufferedQueryPool::GetResults(FVulkanCommandListContext& Context, FVulkanRenderQuery* Query, bool bWait, uint64& OutResult)
{
	if (bWait)
	{
		uint64 Bit = (uint64)(Query->GetActiveQueryIndex() % 64);
		uint64 BitMask = (uint64)1 << Bit;

		// Try to read in bulk if possible
		const uint64 AllUsedMask = (uint64)-1;
		uint32 Word = Query->GetActiveQueryIndex() / 64;
#if 0
		if ((UsedQueryBits[Word] & AllUsedMask) == AllUsedMask && ReadResultsBits[Word] == 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_VulkanWaitQuery);
			uint32 StartIndex = Word * 64;
			check((Query->GetActiveQueryIndex() - Bit) % 64 == 0);
			VERIFYVULKANRESULT(VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), QueryPool, Query->GetActiveQueryIndex() - Bit, 64, 64 * sizeof(uint64), &QueryOutput[Query->GetActiveQueryIndex() - Bit], sizeof(uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
			ReadResultsBits[Word] = AllUsedMask;
		}
		else
#endif
		if ((ReadResultsBits[Word] & BitMask) == 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_VulkanWaitQuery);
			VERIFYVULKANRESULT(VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), QueryPool, Query->GetActiveQueryIndex(), 1, sizeof(uint64), &QueryOutput[Query->GetActiveQueryIndex()], sizeof(uint64), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT));
			ReadResultsBits[Word] = ReadResultsBits[Word] | BitMask;
		}

		OutResult = QueryOutput[Query->GetActiveQueryIndex()];
		if (QueryType == VK_QUERY_TYPE_TIMESTAMP)
		{
			double NanoSecondsPerTimestamp = Device->GetDeviceProperties().limits.timestampPeriod;
			checkf(NanoSecondsPerTimestamp > 0, TEXT("Driver said it allowed timestamps but returned invalid period %f!"), NanoSecondsPerTimestamp);
			OutResult *= (NanoSecondsPerTimestamp / 1e3);
		}
	}
	else
	{
		// Not implemented!
		ensure(0);
	}

	return true;
}
FVulkanTimestampPool::FVulkanTimestampPool(FVulkanDevice* InDevice, uint32 InNumQueries)
	: FVulkanQueryPool(InDevice, 2 * NUM_RENDER_BUFFERS, VK_QUERY_TYPE_TIMESTAMP)
	, BeginCounter(0)
	, SecondsPerTimestamp(0)
	, TimeStampsPerSeconds(0)
{
	QueryOutput.SetNum(InNumQueries);

	// The number of nanoseconds it takes for a timestamp value to be incremented by 1 can be obtained from VkPhysicalDeviceLimits::timestampPeriod after a call to vkGetPhysicalDeviceProperties.
	double NanoSecondsPerTimestamp = Device->GetDeviceProperties().limits.timestampPeriod;
	checkf(NanoSecondsPerTimestamp > 0, TEXT("Driver said it allowed timestamps but returned invalid period %f!"), NanoSecondsPerTimestamp);
	SecondsPerTimestamp = NanoSecondsPerTimestamp / 1e9;
	TimeStampsPerSeconds = 1e9 / NanoSecondsPerTimestamp;
}

inline void FVulkanTimestampPool::Begin(FVulkanCmdBuffer* CmdBuffer)
{
	++BeginCounter;
	uint32 InfoIndex = BeginCounter % NUM_RENDER_BUFFERS;
	FInfo& Info = Infos[InfoIndex];
	uint32 QueryIndex = InfoIndex * 2;
	if (Info.State == EState::Read)
	{
		VulkanRHI::vkCmdResetQueryPool(CmdBuffer->GetHandle(), QueryPool, QueryIndex, 2);
	}
	else
	{
		ensure(Info.State == EState::Undefined || Info.State == EState::WrittenEnd);
	}
	VulkanRHI::vkCmdWriteTimestamp(CmdBuffer->GetHandle(), VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, QueryPool, QueryIndex);
	Info.State = EState::WrittenBegin;
}

inline void FVulkanTimestampPool::End(FVulkanCmdBuffer* CmdBuffer)
{
	uint32 InfoIndex = BeginCounter % NUM_RENDER_BUFFERS;
	FInfo& Info = Infos[InfoIndex];
	ensure(Info.State == EState::WrittenBegin);
	uint32 QueryIndex = InfoIndex * 2 + 1;
	VulkanRHI::vkCmdWriteTimestamp(CmdBuffer->GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, QueryPool, QueryIndex);
	Info.State = EState::WrittenEnd;
	Info.CmdBuffer = CmdBuffer;
	Info.FenceCounter = CmdBuffer->GetFenceSignaledCounter();
}

bool FVulkanTimestampPool::ReadResults(uint64* OutBeginEnd)
{
	uint32 InfoIndex = (BeginCounter + 1) % NUM_RENDER_BUFFERS;
	FVulkanTimestampPool::FInfo& Info = Infos[InfoIndex];
	if (Info.State == EState::WrittenEnd && Info.FenceCounter < Info.CmdBuffer->GetFenceSignaledCounter())
	{
		VkResult Result = VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), GetHandle(), InfoIndex * 2, 2, sizeof(uint64) * 2, &QueryOutput[InfoIndex * 2], sizeof(uint64), VK_QUERY_RESULT_64_BIT);
		if (Result == VK_SUCCESS)
		{
			Info.State = EState::Read;
			OutBeginEnd[0] = QueryOutput[InfoIndex * 2];
			OutBeginEnd[1] = QueryOutput[InfoIndex * 2 + 1];
			return true;
		}
		else
		{
			ensure(0);
		}
	}
	else if (Info.State == EState::Read)
	{
		OutBeginEnd[0] = QueryOutput[InfoIndex * 2];
		OutBeginEnd[1] = QueryOutput[InfoIndex * 2 + 1];
		return true;
	}

	return false;
}


void FVulkanCommandListContext::ReadAndCalculateGPUFrameTime()
{
	check(IsImmediate());
	FVulkanTimestampPool* Pool = Device->GetTimestampQueryPool();
	if (Pool)
	{
		uint64 Results[2];
		if (Pool->ReadResults(Results))
		{
			const uint64 Delta = Results[1] - Results[0];
			double ValueInSeconds = (double)Delta * Pool->SecondsPerTimestamp;
			GGPUFrameTime = (uint32)(ValueInSeconds / FPlatformTime::GetSecondsPerCycle());
		}
		else
		{
			GGPUFrameTime = 0;
		}
	}
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
	if (Query->GetActiveQueryIndex() != -1)
	{
		check(IsInActualRenderingThread());
		FVulkanCommandListContext& Context = Device->GetImmediateContext();
		FVulkanBufferedQueryPool* Pool = (FVulkanBufferedQueryPool*)Query->GetActiveQueryPool();
		return Pool->GetResults(Context, Query, bWait, OutNumPixels);
	}

	return false;
}

// Occlusion/Timer queries.
void FVulkanCommandListContext::RHIBeginRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FVulkanRenderQuery* Query = ResourceCast(QueryRHI);

	if (Query->QueryType == RQT_Occlusion)
	{
		FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();

		AdvanceQuery(Query);

		Query->Begin(CmdBuffer);
	}
	else
	{
		ensure(0);
	}
}

void FVulkanCommandListContext::AdvanceQuery(FVulkanRenderQuery* Query)
{
	//reset prev query
	if (Query->GetActiveQueryIndex() != -1)
	{
		FVulkanBufferedQueryPool* OcclusionPool = (FVulkanBufferedQueryPool*)Query->GetActiveQueryPool();
		CurrentOcclusionQueryData.AddToResetList(OcclusionPool, Query->GetActiveQueryIndex());
	}

	// advance
	Query->AdvanceQueryIndex();

	// alloc if needed
	if (Query->GetActiveQueryIndex() == -1)
	{
		uint32 QueryIndex = 0;
		FVulkanBufferedQueryPool* Pool = nullptr;
		if (Query->QueryType == RQT_AbsoluteTime)
		{
			Pool = &Device->FindAvailableTimestampQueryPool();
		}
		else
		{
			Pool = &Device->FindAvailableOcclusionQueryPool();
		}
		ensure(Pool);

		bool bResult = Pool->AcquireQuery(QueryIndex);
		check(bResult);

		Query->SetActiveQueryIndex(QueryIndex);
		Query->SetActiveQueryPool(Pool);
	}
}

void FVulkanCommandListContext::RHIEndRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FVulkanRenderQuery* Query = ResourceCast(QueryRHI);

	if (Query->QueryType == RQT_Occlusion)
	{
		if (Query->GetActiveQueryIndex() != -1)
		{
			FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
			Query->End(CmdBuffer);
		}
	}
	else
	{
		if (Device->GetDeviceProperties().limits.timestampComputeAndGraphics == VK_FALSE)
		{
			return;
		}
		FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();

		AdvanceQuery(Query);

		// now start it
		Query->End(CmdBuffer);
	}
}

void FVulkanCommandListContext::RHIBeginOcclusionQueryBatch()
{
	ensure(IsImmediate());

	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	ensure(CmdBuffer->IsInsideRenderPass());
}

static inline void ProcessByte(VkCommandBuffer InCmdBufferHandle, FVulkanBufferedQueryPool* Pool, uint8 Bits, int32 BaseStartIndex)
{
	if (Bits)
	{
		VkQueryPool QueryPool = Pool->GetHandle();
		if (Bits == 0xff)
		{
			VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 8);
			Pool->ResetReadResultBits(InCmdBufferHandle, BaseStartIndex, 8);
		}
		else
		{
			while (Bits)
			{
				if (Bits & 1)
				{
					//#todo-rco: Group these
					VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 1);
					Pool->ResetReadResultBits(InCmdBufferHandle, BaseStartIndex, 1);
				}

				Bits >>= 1;
				++BaseStartIndex;
			}
		}
	}
}

static inline void Process16Bits(VkCommandBuffer InCmdBufferHandle, FVulkanBufferedQueryPool* Pool, uint16 Bits, int32 BaseStartIndex)
{
	if (Bits)
	{
		VkQueryPool QueryPool = Pool->GetHandle();
		if (Bits == 0xffff)
		{
			VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 16);
			Pool->ResetReadResultBits(InCmdBufferHandle, BaseStartIndex, 16);
		}
		else
		{
			ProcessByte(InCmdBufferHandle, Pool, (uint8)((Bits >> 0) & 0xff), BaseStartIndex + 0);
			ProcessByte(InCmdBufferHandle, Pool, (uint8)((Bits >> 8) & 0xff), BaseStartIndex + 8);
		}
	}
}

static inline void Process32Bits(VkCommandBuffer InCmdBufferHandle, FVulkanBufferedQueryPool* Pool, uint32 Bits, int32 BaseStartIndex)
{
	if (Bits)
	{
		VkQueryPool QueryPool = Pool->GetHandle();
		if (Bits == 0xffffffff)
		{
			VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 32);
			Pool->ResetReadResultBits(InCmdBufferHandle, BaseStartIndex, 32);
		}
		else
		{
			Process16Bits(InCmdBufferHandle, Pool, (uint16)((Bits >> 0) & 0xffff), BaseStartIndex + 0);
			Process16Bits(InCmdBufferHandle, Pool, (uint16)((Bits >> 16) & 0xffff), BaseStartIndex + 16);
		}
	}
}

void FVulkanCommandListContext::FOcclusionQueryData::ResetQueries(FVulkanCmdBuffer* InCmdBuffer)
{
	ensure(InCmdBuffer->IsOutsideRenderPass());
	const uint64 AllBitsMask = (uint64)-1;
	VkCommandBuffer CmdBufferHandle = InCmdBuffer->GetHandle();

	for (auto& Pair : ResetList)
	{
		TArray<uint64>& ListPerPool = Pair.Value;
		FVulkanBufferedQueryPool* Pool = (FVulkanBufferedQueryPool*)Pair.Key;

		// Initial bit Index
		int32 WordIndex = 0;
		while (WordIndex < ListPerPool.Num())
		{
			uint64 Bits = ListPerPool[WordIndex];
			VkQueryPool PoolHandle = Pool->GetHandle();
			if (Bits)
			{
				if (Bits == AllBitsMask)
				{
					// Quick early out
					VulkanRHI::vkCmdResetQueryPool(CmdBufferHandle, PoolHandle, WordIndex * 64, 64);
					Pool->ResetReadResultBits(CmdBufferHandle, WordIndex * 64, 64);
				}
				else
				{
					Process32Bits(CmdBufferHandle, Pool, (uint32)((Bits >> (uint64)0) & (uint64)0xffffffff), WordIndex * 64 + 0);
					Process32Bits(CmdBufferHandle, Pool, (uint32)((Bits >> (uint64)32) & (uint64)0xffffffff), WordIndex * 64 + 32);
				}
			}
			++WordIndex;
		}
	}
}

void FVulkanCommandListContext::RHIEndOcclusionQueryBatch()
{
	ensure(IsImmediate());

	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	CurrentOcclusionQueryData.CmdBuffer = CmdBuffer;
	CurrentOcclusionQueryData.FenceCounter = CmdBuffer->GetFenceSignaledCounter();

	TransitionState.EndRenderPass(CmdBuffer);

	// Resetting queries has to happen outside a render pass
	FVulkanCmdBuffer* UploadCmdBuffer = CommandBufferManager->GetUploadCmdBuffer();
	{
		SCOPE_CYCLE_COUNTER(STAT_VulkanResetQuery);
		CurrentOcclusionQueryData.ResetQueries(UploadCmdBuffer);
		CurrentOcclusionQueryData.ClearResetList();
	}
	CommandBufferManager->SubmitUploadCmdBuffer(false);

	// Sync point
	RequestSubmitCurrentCommands();
	SafePointSubmit();
}

void FVulkanCommandListContext::WriteBeginTimestamp(FVulkanCmdBuffer* CmdBuffer)
{
	FVulkanTimestampPool* TimestampPool = Device->GetTimestampQueryPool();
	if (TimestampPool)
	{
		TimestampPool->Begin(CmdBuffer);
	}
}

void FVulkanCommandListContext::WriteEndTimestamp(FVulkanCmdBuffer* CmdBuffer)
{
	FVulkanTimestampPool* TimestampPool = Device->GetTimestampQueryPool();
	if (TimestampPool)
	{
		TimestampPool->End(CmdBuffer);
	}
}
