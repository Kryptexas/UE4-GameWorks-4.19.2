// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VulkanQuery.cpp: Vulkan query RHI implementation.
=============================================================================*/

#include "VulkanRHIPrivate.h"
#include "VulkanContext.h"
#include "VulkanCommandBuffer.h"
#include "EngineGlobals.h"

TAutoConsoleVariable<int32> GSubmitOcclusionBatchCmdBufferCVar(
	TEXT("r.Vulkan.SubmitOcclusionBatchCmdBuffer"),
	1,
	TEXT("1 to submit the cmd buffer after end occlusion query batch (default)"),
	ECVF_RenderThreadSafe
);

FCriticalSection GQueryLock;

struct FRHICommandWaitForFence final : public FRHICommand<FRHICommandWaitForFence>
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

FOLDVulkanRenderQuery::FOLDVulkanRenderQuery(FVulkanDevice* Device, ERenderQueryType InQueryType)
	: CurrentQueryIdx(0)
	, QueryType(InQueryType)
	, CurrentCmdBuffer(nullptr)
{
	INC_DWORD_STAT(STAT_VulkanNumQueries);
	for (int Index = 0; Index < NumQueries; ++Index)
	{
		QueryIndices[Index] = -1;
		QueryPools[Index] = nullptr;
	}
}

FOLDVulkanRenderQuery::~FOLDVulkanRenderQuery()
{
	DEC_DWORD_STAT(STAT_VulkanNumQueries);
	for (int Index = 0; Index < NumQueries; ++Index)
	{
		if (QueryIndices[Index] != -1)
		{
			FScopeLock Lock(&GQueryLock);
			((FOLDVulkanBufferedQueryPool*)QueryPools[Index])->ReleaseQuery(QueryIndices[Index]);
		}
	}
}

inline void FOLDVulkanRenderQuery::Begin(FVulkanCmdBuffer* CmdBuffer)
{
	CurrentCmdBuffer = CmdBuffer;
	ensure(GetActiveQueryIndex() != -1);
	if (QueryType == RQT_Occlusion)
	{
		VulkanRHI::vkCmdBeginQuery(CmdBuffer->GetHandle(), GetActiveQueryPool()->GetHandle(), GetActiveQueryIndex(), VK_QUERY_CONTROL_PRECISE_BIT);
	}
	else
	{
		ensure(0);
	}
}

inline void FOLDVulkanRenderQuery::End(FVulkanCmdBuffer* CmdBuffer)
{
	ensure(QueryType != RQT_Occlusion || CurrentCmdBuffer == CmdBuffer);
	ensure(GetActiveQueryIndex() != -1);

	if (QueryType == RQT_Occlusion)
	{
		VulkanRHI::vkCmdEndQuery(CmdBuffer->GetHandle(), GetActiveQueryPool()->GetHandle(), GetActiveQueryIndex());
	}
	else
	{
		VulkanRHI::vkCmdWriteTimestamp(CmdBuffer->GetHandle(), VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, GetActiveQueryPool()->GetHandle(), GetActiveQueryIndex());
	}
}

bool FOLDVulkanRenderQuery::GetResult(FVulkanDevice* Device, uint64& Result, bool bWait)
{
	if (GetActiveQueryIndex() != -1)
	{
		check(IsInRenderingThread() || IsInRHIThread());
		FVulkanCommandListContext& Context = Device->GetImmediateContext();
		FOLDVulkanBufferedQueryPool* Pool = (FOLDVulkanBufferedQueryPool*)GetActiveQueryPool();
		return Pool->GetResults(Context, this, bWait, Result);
	}
	return false;
}

FOLDVulkanQueryPool::FOLDVulkanQueryPool(FVulkanDevice* InDevice, uint32 InMaxQueries, VkQueryType InQueryType)
	: VulkanRHI::FDeviceChild(InDevice)
	, QueryPool(VK_NULL_HANDLE)
	, MaxQueries(InMaxQueries)
	, QueryType(InQueryType)
{
	check(InDevice);

	VkQueryPoolCreateInfo PoolCreateInfo;
	FMemory::Memzero(PoolCreateInfo);
	PoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	PoolCreateInfo.queryType = QueryType;
	PoolCreateInfo.queryCount = MaxQueries;

	VERIFYVULKANRESULT(VulkanRHI::vkCreateQueryPool(Device->GetInstanceHandle(), &PoolCreateInfo, nullptr, &QueryPool));
}

FOLDVulkanQueryPool::~FOLDVulkanQueryPool()
{
	check(QueryPool == VK_NULL_HANDLE);
}

void FOLDVulkanQueryPool::Destroy()
{
	VulkanRHI::vkDestroyQueryPool(Device->GetInstanceHandle(), QueryPool, nullptr);
	QueryPool = VK_NULL_HANDLE;
}

void FOLDVulkanQueryPool::Reset(FVulkanCmdBuffer* InCmdBuffer)
{
	VulkanRHI::vkCmdResetQueryPool(InCmdBuffer->GetHandle(), QueryPool, 0, MaxQueries);
}

inline bool FOLDVulkanBufferedQueryPool::GetResults(FVulkanCommandListContext& Context, FOLDVulkanRenderQuery* Query, bool bWait, uint64& OutResult)
{
	VkQueryResultFlags Flags = VK_QUERY_RESULT_64_BIT;
#if VULKAN_USE_QUERY_WAIT
	if (bWait)
	{
		Flags |= VK_QUERY_RESULT_WAIT_BIT;
	}
#endif
	{
		uint64 Bit = (uint64)(Query->GetActiveQueryIndex() % 64);
		uint64 BitMask = (uint64)1 << Bit;

		// Try to read in bulk if possible
		const uint64 AllUsedMask = (uint64)-1;
		uint32 Word = Query->GetActiveQueryIndex() / 64;

		if ((StartedQueryBits[Word] & BitMask) == 0)
		{
			// query never started/ended so how can we get a result ?
			OutResult = 0;
			return true;
		}

#if VULKAN_USE_QUERY_WAIT
		if ((ReadResultsBits[Word] & BitMask) == 0)
		{
			SCOPE_CYCLE_COUNTER(STAT_VulkanWaitQuery);
			VkResult ScopedResult = VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), QueryPool, Query->GetActiveQueryIndex(),
				1, sizeof(uint64), &QueryOutput[Query->GetActiveQueryIndex()], sizeof(uint64), Flags);
			if (ScopedResult != VK_SUCCESS)
			{
				if (bWait == false && ScopedResult == VK_NOT_READY)
				{
					OutResult = 0;
					return false;
				}
				else
				{
					VulkanRHI::VerifyVulkanResult(ScopedResult, "vkGetQueryPoolResults", __FILE__, __LINE__);
				}
			}

			ReadResultsBits[Word] = ReadResultsBits[Word] | BitMask;
		}
#else
		if ((ReadResultsBits[Word] & BitMask) == 0)
		{
			VkResult ScopedResult = VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), QueryPool, Query->GetActiveQueryIndex(),
				1, sizeof(uint64), &QueryOutput[Query->GetActiveQueryIndex()], sizeof(uint64), Flags);
			if (ScopedResult == VK_SUCCESS)
			{
				ReadResultsBits[Word] = ReadResultsBits[Word] | BitMask;
			}
			else if (ScopedResult == VK_NOT_READY)
			{
				if (bWait)
				{
					uint32 IdleStart = FPlatformTime::Cycles();

					SCOPE_CYCLE_COUNTER(STAT_VulkanWaitQuery);

					// We'll do manual wait
					double StartTime = FPlatformTime::Seconds();

					ENamedThreads::Type RenderThread_Local = ENamedThreads::GetRenderThread_Local();
					bool bSuccess = false;
					while (!bSuccess)
					{
						FPlatformProcess::SleepNoStats(0);

						// pump RHIThread to make sure these queries have actually been submitted to the GPU.
						if (IsInActualRenderingThread())
						{
							FTaskGraphInterface::Get().ProcessThreadUntilIdle(RenderThread_Local);
						}

						ScopedResult = VulkanRHI::vkGetQueryPoolResults(Device->GetInstanceHandle(), QueryPool, Query->GetActiveQueryIndex(),
							1, sizeof(uint64), &QueryOutput[Query->GetActiveQueryIndex()], sizeof(uint64), Flags);
						if (ScopedResult == VK_SUCCESS)
						{
							bSuccess = true;
							break;
						}
						else if (ScopedResult == VK_NOT_READY)
						{
							bSuccess = false;
						}
						else
						{
							bSuccess = false;
							VulkanRHI::VerifyVulkanResult(ScopedResult, "vkGetQueryPoolResults", __FILE__, __LINE__);
						}

						// timer queries are used for Benchmarks which can stall a bit more
						const double TimeoutValue = (QueryType == VK_QUERY_TYPE_TIMESTAMP) ? 2.0 : 0.5;
						// look for gpu stuck/crashed
						if ((FPlatformTime::Seconds() - StartTime) > TimeoutValue)
						{
							if (QueryType == VK_QUERY_TYPE_OCCLUSION)
							{
								UE_LOG(LogRHI, Log, TEXT("Timed out while waiting for GPU to catch up on occlusion results. (%.1f s)"), TimeoutValue);
							}
							else
							{
								UE_LOG(LogRHI, Log, TEXT("Timed out while waiting for GPU to catch up on occlusion/timer results. (%.1f s)"), TimeoutValue);
							}
							return false;
						}
					}

					GRenderThreadIdle[ERenderThreadIdleTypes::WaitingForGPUQuery] += FPlatformTime::Cycles() - IdleStart;
					GRenderThreadNumIdle[ERenderThreadIdleTypes::WaitingForGPUQuery]++;
					check(bSuccess);
					ReadResultsBits[Word] = ReadResultsBits[Word] | BitMask;
				}
				else
				{
					OutResult = 0;
					return false;
				}
			}
			else
			{
				VulkanRHI::VerifyVulkanResult(ScopedResult, "vkGetQueryPoolResults", __FILE__, __LINE__);
			}
		}
#endif

		OutResult = QueryOutput[Query->GetActiveQueryIndex()];
		if (QueryType == VK_QUERY_TYPE_TIMESTAMP)
		{
			double NanoSecondsPerTimestamp = Device->GetDeviceProperties().limits.timestampPeriod;
			checkf(NanoSecondsPerTimestamp > 0, TEXT("Driver said it allowed timestamps but returned invalid period %f!"), NanoSecondsPerTimestamp);
			OutResult *= (NanoSecondsPerTimestamp / 1e3); // to microSec
		}
	}

	return true;
}

void FVulkanCommandListContext::ReadAndCalculateGPUFrameTime()
{
	check(IsImmediate());

	if (FrameTiming)
	{
		const uint64 Delta = FrameTiming->GetTiming(false);
		GGPUFrameTime = Delta ? (Delta / 1e6) / FPlatformTime::GetSecondsPerCycle() : 0;
	}
	else
	{
		GGPUFrameTime = 0;
	}

	static const auto CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Vulkan.ProfileCmdBuffers"));
	if (CVar->GetInt() != 0)
	{
		const uint64 Delta = GetCommandBufferManager()->CalculateGPUTime();
		GGPUFrameTime = Delta ? (Delta / 1e6) / FPlatformTime::GetSecondsPerCycle() : 0;
	}
}


FRenderQueryRHIRef FVulkanDynamicRHI::RHICreateRenderQuery(ERenderQueryType QueryType)
{
	FOLDVulkanRenderQuery* Query = new FOLDVulkanRenderQuery(Device, QueryType);
	return Query;
}

bool FVulkanDynamicRHI::RHIGetRenderQueryResult(FRenderQueryRHIParamRef QueryRHI, uint64& OutNumPixels, bool bWait)
{
	check(IsInRenderingThread());

	FOLDVulkanRenderQuery* Query = ResourceCast(QueryRHI);
	check(Query);
	return Query->GetResult(Device, OutNumPixels, bWait);
}

void FVulkanCommandListContext::RHIBeginRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FOLDVulkanRenderQuery* Query = ResourceCast(QueryRHI);
	if (Query->QueryType == RQT_Occlusion)
	{
		FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
		AdvanceQuery(Query);
		Query->Begin(CmdBuffer);
	}
	else
	{
		check(0);
	}
}

void FVulkanCommandListContext::AdvanceQuery(FOLDVulkanRenderQuery* Query)
{
	//reset prev query
	if (Query->GetActiveQueryIndex() != -1)
	{
		FOLDVulkanBufferedQueryPool* OcclusionPool = (FOLDVulkanBufferedQueryPool*)Query->GetActiveQueryPool();
		CurrentOcclusionQueryData.AddToResetList(OcclusionPool, Query->GetActiveQueryIndex());
	}

	// advance
	Query->AdvanceQueryIndex();

	// alloc if needed
	if (Query->GetActiveQueryIndex() == -1)
	{
		uint32 QueryIndex = 0;
		FOLDVulkanBufferedQueryPool* Pool = nullptr;

		{
			FScopeLock Lock(&GQueryLock);

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
		}
		
		Query->SetActiveQueryIndex(QueryIndex);
		Query->SetActiveQueryPool(Pool);
	}

	// mark at begin
	((FOLDVulkanBufferedQueryPool*)Query->GetActiveQueryPool())->MarkQueryAsStarted(Query->GetActiveQueryIndex());
}

void FVulkanCommandListContext::EndRenderQueryInternal(FVulkanCmdBuffer* CmdBuffer, FOLDVulkanRenderQuery* Query)
{
	if (Query->QueryType == RQT_Occlusion)
	{
		if (Query->GetActiveQueryIndex() != -1)
		{
			Query->End(CmdBuffer);
		}
	}
	else
	{
		if (Device->GetDeviceProperties().limits.timestampComputeAndGraphics == VK_FALSE)
		{
			return;
		}

		AdvanceQuery(Query);

		// now start it
		Query->End(CmdBuffer);
	}
}

void FVulkanCommandListContext::RHIEndRenderQuery(FRenderQueryRHIParamRef QueryRHI)
{
	FOLDVulkanRenderQuery* Query = ResourceCast(QueryRHI);
	return EndRenderQueryInternal(CommandBufferManager->GetActiveCmdBuffer(), Query);	
}

void FVulkanCommandListContext::RHIBeginOcclusionQueryBatch()
{
	ensure(IsImmediate());

	FVulkanCmdBuffer* CmdBuffer = CommandBufferManager->GetActiveCmdBuffer();
	ensure(CmdBuffer->IsInsideRenderPass());
}

static inline void ProcessByte(VkCommandBuffer InCmdBufferHandle, FOLDVulkanBufferedQueryPool* Pool, uint8 Bits, int32 BaseStartIndex)
{
	if (Bits)
	{
		VkQueryPool QueryPool = Pool->GetHandle();
		if (Bits == 0xff)
		{
			VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 8);
			Pool->ResetReadResultBits(BaseStartIndex, 8);
		}
		else
		{
			while (Bits)
			{
				if (Bits & 1)
				{
					//#todo-rco: Group these
					VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 1);
					Pool->ResetReadResultBits(BaseStartIndex, 1);
				}

				Bits >>= 1;
				++BaseStartIndex;
			}
		}
	}
}

static inline void Process16Bits(VkCommandBuffer InCmdBufferHandle, FOLDVulkanBufferedQueryPool* Pool, uint16 Bits, int32 BaseStartIndex)
{
	if (Bits)
	{
		VkQueryPool QueryPool = Pool->GetHandle();
		if (Bits == 0xffff)
		{
			VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 16);
			Pool->ResetReadResultBits(BaseStartIndex, 16);
		}
		else
		{
			ProcessByte(InCmdBufferHandle, Pool, (uint8)((Bits >> 0) & 0xff), BaseStartIndex + 0);
			ProcessByte(InCmdBufferHandle, Pool, (uint8)((Bits >> 8) & 0xff), BaseStartIndex + 8);
		}
	}
}

static inline void Process32Bits(VkCommandBuffer InCmdBufferHandle, FOLDVulkanBufferedQueryPool* Pool, uint32 Bits, int32 BaseStartIndex)
{
	if (Bits)
	{
		VkQueryPool QueryPool = Pool->GetHandle();
		if (Bits == 0xffffffff)
		{
			VulkanRHI::vkCmdResetQueryPool(InCmdBufferHandle, QueryPool, BaseStartIndex, 32);
			Pool->ResetReadResultBits(BaseStartIndex, 32);
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
		FOLDVulkanBufferedQueryPool* Pool = (FOLDVulkanBufferedQueryPool*)Pair.Key;

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
					Pool->ResetReadResultBits(WordIndex * 64, 64);
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
	if (GSubmitOcclusionBatchCmdBufferCVar.GetValueOnAnyThread())
	{
		RequestSubmitCurrentCommands();
		SafePointSubmit();
	}
}

void FVulkanCommandListContext::WriteBeginTimestamp(FVulkanCmdBuffer* CmdBuffer)
{
	FrameTiming->StartTiming(CmdBuffer);
}

void FVulkanCommandListContext::WriteEndTimestamp(FVulkanCmdBuffer* CmdBuffer)
{
	FrameTiming->EndTiming();
}
