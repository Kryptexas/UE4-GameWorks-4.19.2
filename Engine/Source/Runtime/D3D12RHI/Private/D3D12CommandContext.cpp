// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
D3D12CommandContext.cpp: RHI  Command Context implementation.
=============================================================================*/

#include "D3D12RHIPrivate.h"

int32 GCommandListBatchingMode = CLB_NormalBatching;
static FAutoConsoleVariableRef CVarCommandListBatchingMode(
	TEXT("D3D12.CommandListBatchingMode"),
	GCommandListBatchingMode,
	TEXT("Changes how command lists are batched and submitted to the GPU."),
	ECVF_RenderThreadSafe
	);

FD3D12CommandContext::FD3D12CommandContext(FD3D12Device* InParent, FD3D12SubAllocatedOnlineHeap::SubAllocationDesc& SubHeapDesc, bool InIsDefaultContext, bool InIsAsyncComputeContext) :
	OwningRHI(*InParent->GetOwningRHI()),
	bUsingTessellation(false),
	PendingNumVertices(0),
	PendingVertexDataStride(0),
	PendingPrimitiveType(0),
	PendingNumPrimitives(0),
	PendingMinVertexIndex(0),
	PendingNumIndices(0),
	PendingIndexDataStride(0),
	CurrentDepthTexture(nullptr),
	NumSimultaneousRenderTargets(0),
	NumUAVs(0),
	CurrentDSVAccessType(FExclusiveDepthStencil::DepthWrite_StencilWrite),
	bDiscardSharedConstants(false),
	bIsDefaultContext(InIsDefaultContext),
	bIsAsyncComputeContext(InIsAsyncComputeContext),
	CommandListHandle(),
	CommandAllocator(nullptr),
	CommandAllocatorManager(InParent, InIsAsyncComputeContext ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT),
	ConstantsAllocator(InParent, GetVisibilityMask(), (1024*1024) * 2),
	DynamicVB(InParent),
	DynamicIB(InParent),
	StateCache(InParent->GetNodeMask()),
	VSConstantBuffer(InParent, ConstantsAllocator),
	HSConstantBuffer(InParent, ConstantsAllocator),
	DSConstantBuffer(InParent, ConstantsAllocator),
	PSConstantBuffer(InParent, ConstantsAllocator),
	GSConstantBuffer(InParent, ConstantsAllocator),
	CSConstantBuffer(InParent, ConstantsAllocator),
	bIsMGPUAware(InParent->GetParentAdapter()->GetNumGPUNodes() > 1),
	CurrentDepthStencilTarget(nullptr),
	FD3D12DeviceChild(InParent),
	FD3D12SingleNodeGPUObject(InParent->GetNodeMask())
{
	FMemory::Memzero(DirtyUniformBuffers);
	FMemory::Memzero(BoundUniformBuffers);
	FMemory::Memzero(CurrentRenderTargets);
	FMemory::Memzero(CurrentUAVs);
	StateCache.Init(GetParentDevice(), this, nullptr, SubHeapDesc);

	ConstantsAllocator.Init();
}

FD3D12CommandContext::~FD3D12CommandContext()
{
	ClearState();
}

void FD3D12CommandContext::RHIPushEvent(const TCHAR* Name, FColor Color)
{
	if (IsDefaultContext())
	{
		GetParentDevice()->PushGPUEvent(Name, Color);
	}

#if USE_PIX
	PIXBeginEvent(CommandListHandle.GraphicsCommandList(), PIX_COLOR_DEFAULT, Name);
#endif
}

void FD3D12CommandContext::RHIPopEvent()
{
	if (IsDefaultContext())
	{
		GetParentDevice()->PopGPUEvent();
	}

#if USE_PIX
	PIXEndEvent(CommandListHandle.GraphicsCommandList());
#endif
}

void FD3D12CommandContext::RHIAutomaticCacheFlushAfterComputeShader(bool bEnable)
{
	StateCache.AutoFlushComputeShaderCache(bEnable);
}

void FD3D12CommandContext::RHIFlushComputeShaderCache()
{
	StateCache.FlushComputeShaderCache(true);
}

FD3D12CommandListManager& FD3D12CommandContext::GetCommandListManager()
{
	return bIsAsyncComputeContext ? GetParentDevice()->GetAsyncCommandListManager() : GetParentDevice()->GetCommandListManager();
}

void FD3D12CommandContext::ConditionalObtainCommandAllocator()
{
	if (CommandAllocator == nullptr)
	{
		// Obtain a command allocator if the context doesn't already have one.
		// This will check necessary fence values to ensure the returned command allocator isn't being used by the GPU, then reset it.
		CommandAllocator = CommandAllocatorManager.ObtainCommandAllocator();
	}
}

void FD3D12CommandContext::ReleaseCommandAllocator()
{
	if (CommandAllocator != nullptr)
	{
		// Release the command allocator so it can be reused.
		CommandAllocatorManager.ReleaseCommandAllocator(CommandAllocator);
		CommandAllocator = nullptr;
	}
}

void FD3D12CommandContext::OpenCommandList(bool bRestoreState)
{
	FD3D12CommandListManager& CommandListManager = GetCommandListManager();

	// Conditionally get a new command allocator.
	// Each command context uses a new allocator for all command lists with a single frame.
	ConditionalObtainCommandAllocator();

	// Get a new command list
	CommandListHandle = CommandListManager.ObtainCommandList(*CommandAllocator);

	CommandListHandle->SetDescriptorHeaps(DescriptorHeaps.Num(), DescriptorHeaps.GetData());
	StateCache.ForceSetGraphicsRootSignature();
	StateCache.ForceSetComputeRootSignature();

	StateCache.GetDescriptorCache()->NotifyCurrentCommandList(CommandListHandle);

	if (bRestoreState)
	{
		// The next time ApplyState is called, it will set all state on this new command list
		StateCache.RestoreState();
	}

	numDraws = 0;
	numDispatches = 0;
	numClears = 0;
	numBarriers = 0;
	numCopies = 0;
	otherWorkCounter = 0;
	CommandListHandle.SetCurrentOwningContext(this);
}

void FD3D12CommandContext::CloseCommandList()
{
	CommandListHandle.Close();
}

void FD3D12CommandContext::ExecuteCommandList(bool WaitForCompletion)
{
	check(CommandListHandle.IsClosed());

	// Only submit a command list if it does meaningful work or is expected to wait for completion.
	if (WaitForCompletion || HasDoneWork())
	{
		CommandListHandle.Execute(WaitForCompletion);
	}
}

FD3D12CommandListHandle FD3D12CommandContext::FlushCommands(bool WaitForCompletion)
{
	// We should only be flushing the default context
	check(IsDefaultContext());

	FD3D12Device* Device = GetParentDevice();
	const bool bExecutePendingWork = GCommandListBatchingMode == CLB_AggressiveBatching || GetParentDevice()->IsGPUIdle();
	const bool bHasPendingWork = bExecutePendingWork && (Device->PendingCommandListsTotalWorkCommands > 0);
	const bool bHasDoneWork = HasDoneWork() || bHasPendingWork;

	// Only submit a command list if it does meaningful work or the flush is expected to wait for completion.
	if (WaitForCompletion || bHasDoneWork)
	{
		// Close the current command list
		CloseCommandList();

		if (bHasPendingWork)
		{
			// Submit all pending command lists and the current command list
			Device->PendingCommandLists.Add(CommandListHandle);
			GetCommandListManager().ExecuteCommandLists(Device->PendingCommandLists, WaitForCompletion);
			Device->PendingCommandLists.Reset();
			Device->PendingCommandListsTotalWorkCommands = 0;
		}
		else
		{
			// Just submit the current command list
			CommandListHandle.Execute(WaitForCompletion);
		}

		// Get a new command list to replace the one we submitted for execution. 
		// Restore the state from the previous command list.
		OpenCommandList(true);
	}

	return CommandListHandle;
}

void FD3D12CommandContext::Finish(TArray<FD3D12CommandListHandle>& CommandLists)
{
	CloseCommandList();

	if (HasDoneWork())
	{
		CommandLists.Add(CommandListHandle);
	}
	else
	{
		// Release the unused command list.
		GetCommandListManager().ReleaseCommandList(CommandListHandle);
	}

	// The context is done with this command list handle.
	CommandListHandle = FD3D12CommandListHandle();
}

void FD3D12CommandContext::RHIBeginFrame()
{
	FD3D12Device* Device = GetParentDevice();
	FD3D12Adapter* Adapter = Device->GetParentAdapter();

	check(IsDefaultContext());
	check(Adapter->GetCurrentNodeMask() == Device->GetNodeMask());
	RHIPrivateBeginFrame();

	FD3D12GlobalOnlineHeap& SamplerHeap = Device->GetGlobalSamplerHeap();
	const uint32 NumContexts = Device->GetNumContexts();

	if (SamplerHeap.DescriptorTablesDirty())
	{
		//Rearrange the set for better look-up performance
		SamplerHeap.GetUniqueDescriptorTables().Compact();
	}

	for (uint32 i = 0; i < NumContexts; ++i)
	{
		Device->GetCommandContext(i).StateCache.GetDescriptorCache()->BeginFrame();
	}

	Device->GetGlobalSamplerHeap().ToggleDescriptorTablesDirtyFlag(false);

	Adapter->GetGPUProfiler().BeginFrame(Device->GetOwningRHI());
}

void FD3D12CommandContext::ClearState()
{
	StateCache.ClearState();

	bDiscardSharedConstants = false;

	for (int32 Frequency = 0; Frequency < SF_NumFrequencies; Frequency++)
	{
		DirtyUniformBuffers[Frequency] = 0;
		for (int32 Index = 0; Index < MAX_UNIFORM_BUFFERS_PER_SHADER_STAGE; ++Index)
		{
			BoundUniformBuffers[Frequency][Index] = nullptr;
		}
	}

	for (int32 Index = 0; Index < _countof(CurrentUAVs); ++Index)
	{
		CurrentUAVs[Index] = nullptr;
	}
	NumUAVs = 0;

	if (!bIsAsyncComputeContext)
	{
		for (int32 Index = 0; Index < _countof(CurrentRenderTargets); ++Index)
		{
			CurrentRenderTargets[Index] = nullptr;
		}
		NumSimultaneousRenderTargets = 0;

		CurrentDepthStencilTarget = nullptr;
		CurrentDepthTexture = nullptr;

		CurrentDSVAccessType = FExclusiveDepthStencil::DepthWrite_StencilWrite;

		bUsingTessellation = false;

		CurrentBoundShaderState = nullptr;
	}

	// MSFT: Need to do anything with the constant buffers?
	/*
	for (int32 BufferIndex = 0; BufferIndex < VSConstantBuffers.Num(); BufferIndex++)
	{
	VSConstantBuffers[BufferIndex]->ClearConstantBuffer();
	}
	for (int32 BufferIndex = 0; BufferIndex < PSConstantBuffers.Num(); BufferIndex++)
	{
	PSConstantBuffers[BufferIndex]->ClearConstantBuffer();
	}
	for (int32 BufferIndex = 0; BufferIndex < HSConstantBuffers.Num(); BufferIndex++)
	{
	HSConstantBuffers[BufferIndex]->ClearConstantBuffer();
	}
	for (int32 BufferIndex = 0; BufferIndex < DSConstantBuffers.Num(); BufferIndex++)
	{
	DSConstantBuffers[BufferIndex]->ClearConstantBuffer();
	}
	for (int32 BufferIndex = 0; BufferIndex < GSConstantBuffers.Num(); BufferIndex++)
	{
	GSConstantBuffers[BufferIndex]->ClearConstantBuffer();
	}
	for (int32 BufferIndex = 0; BufferIndex < CSConstantBuffers.Num(); BufferIndex++)
	{
	CSConstantBuffers[BufferIndex]->ClearConstantBuffer();
	}
	*/

	CurrentComputeShader = nullptr;
}

void FD3D12CommandContext::CheckIfSRVIsResolved(FD3D12ShaderResourceView* SRV)
{
	// MSFT: Seb: TODO: Make this work on 12
#if CHECK_SRV_TRANSITIONS
	if (GRHIThread || !SRV)
	{
		return;
	}

	static const auto CheckSRVCVar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("r.CheckSRVTransitions"));
	if (!CheckSRVCVar->GetValueOnRenderThread())
	{
		return;
	}

	ID3D11Resource* SRVResource = nullptr;
	SRV->GetResource(&SRVResource);

	int32 MipLevel;
	int32 NumMips;
	int32 ArraySlice;
	int32 NumSlices;
	GetMipAndSliceInfoFromSRV(SRV, MipLevel, NumMips, ArraySlice, NumSlices);

	//d3d uses -1 to mean 'all mips'
	int32 LastMip = MipLevel + NumMips - 1;
	int32 LastSlice = ArraySlice + NumSlices - 1;

	TArray<FUnresolvedRTInfo> RTInfoArray;
	UnresolvedTargets.MultiFind(SRVResource, RTInfoArray);

	for (int32 InfoIndex = 0; InfoIndex < RTInfoArray.Num(); ++InfoIndex)
	{
		const FUnresolvedRTInfo& RTInfo = RTInfoArray[InfoIndex];
		int32 RTLastMip = RTInfo.MipLevel + RTInfo.NumMips - 1;
		ensureMsgf((MipLevel == -1 || NumMips == -1) || (LastMip < RTInfo.MipLevel || MipLevel > RTLastMip), TEXT("SRV is set to read mips in range %i to %i.  Target %s is unresolved for mip %i"), MipLevel, LastMip, *RTInfo.ResourceName.ToString(), RTInfo.MipLevel);
		ensureMsgf(NumMips != -1, TEXT("SRV is set to read all mips.  Target %s is unresolved for mip %i"), *RTInfo.ResourceName.ToString(), RTInfo.MipLevel);

		int32 RTLastSlice = RTInfo.ArraySlice + RTInfo.ArraySize - 1;
		ensureMsgf((ArraySlice == -1 || LastSlice == -1) || (LastSlice < RTInfo.ArraySlice || ArraySlice > RTLastSlice), TEXT("SRV is set to read slices in range %i to %i.  Target %s is unresolved for mip %i"), ArraySlice, LastSlice, *RTInfo.ResourceName.ToString(), RTInfo.ArraySlice);
		ensureMsgf(ArraySlice == -1 || NumSlices != -1, TEXT("SRV is set to read all slices.  Target %s is unresolved for slice %i"));
	}
	SRVResource->Release();
#endif
}

void FD3D12CommandContext::ConditionalClearShaderResource(FD3D12ResourceLocation* Resource)
{
	SCOPE_CYCLE_COUNTER(STAT_D3D12ClearShaderResourceTime);
	check(Resource);
	ClearShaderResourceViews<SF_Vertex>(Resource);
	ClearShaderResourceViews<SF_Hull>(Resource);
	ClearShaderResourceViews<SF_Domain>(Resource);
	ClearShaderResourceViews<SF_Pixel>(Resource);
	ClearShaderResourceViews<SF_Geometry>(Resource);
	ClearShaderResourceViews<SF_Compute>(Resource);
}

void FD3D12CommandContext::ClearAllShaderResources()
{
	StateCache.ClearSRVs();
}

void FD3D12CommandContext::RHIEndFrame()
{
#if PLATFORM_SUPPORTS_MGPU
	FD3D12Device* Device = GetParentDevice();
	FD3D12Adapter* Adapter = Device->GetParentAdapter();

	if (Adapter->AlternateFrameRenderingEnabled())
	{
		Adapter->SignalEndOfFrame(GetCommandListManager().GetD3DCommandQueue(), false);

		// When doing AFR rendering we need to switch to the next GPU
		Adapter->SwitchToNextGPU();

		// Update the default context redirector so that the next frame will work on the correct context
		Adapter->GetDefaultContextRedirector().SetCurrentDeviceIndex(Adapter->GetCurrentDevice()->GetNodeIndex());
		Adapter->GetDefaultAsyncComputeContextRedirector().SetCurrentDeviceIndex(Adapter->GetCurrentDevice()->GetNodeIndex());
	}
#endif
}

void FD3D12CommandContext::UpdateMemoryStats()
{
#if PLATFORM_WINDOWS
	DXGI_QUERY_VIDEO_MEMORY_INFO LocalVideoMemoryInfo;

	GetParentDevice()->GetLocalVideoMemoryInfo(&LocalVideoMemoryInfo);

	int64 budget = LocalVideoMemoryInfo.Budget;

	int64 AvailableSpace = budget - int64(LocalVideoMemoryInfo.CurrentUsage);

	SET_MEMORY_STAT(STAT_D3D12UsedVideoMemory, LocalVideoMemoryInfo.CurrentUsage);
	SET_MEMORY_STAT(STAT_D3D12AvailableVideoMemory, AvailableSpace);
	SET_MEMORY_STAT(STAT_D3D12TotalVideoMemory, budget);
#endif
}

void FD3D12CommandContext::RHIBeginScene()
{
	check(IsDefaultContext());
	FD3D12DynamicRHI& RHI = OwningRHI;
	// Increment the frame counter. INDEX_NONE is a special value meaning "uninitialized", so if
	// we hit it just wrap around to zero.
	OwningRHI.SceneFrameCounter++;
	if (OwningRHI.SceneFrameCounter == INDEX_NONE)
	{
		OwningRHI.SceneFrameCounter++;
	}

	static auto* ResourceTableCachingCvar = IConsoleManager::Get().FindTConsoleVariableDataInt(TEXT("rhi.ResourceTableCaching"));
	if (ResourceTableCachingCvar == NULL || ResourceTableCachingCvar->GetValueOnAnyThread() == 1)
	{
		OwningRHI.ResourceTableFrameCounter = OwningRHI.SceneFrameCounter;
	}
}

void FD3D12CommandContext::RHIEndScene()
{
	OwningRHI.ResourceTableFrameCounter = INDEX_NONE;
}

#if D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE

//todo recycle these to avoid alloc

class FD3D12CommandContextContainer : public IRHICommandContextContainer
{
	FD3D12Adapter* Adapter;
	FD3D12CommandContext* CmdContext;
	TArray<FD3D12CommandListHandle> CommandLists;
	const int32 FrameIndex;

public:

	/** Custom new/delete with recycling */
	void* operator new(size_t Size);
	void operator delete(void* RawMemory);

	FD3D12CommandContextContainer(FD3D12Adapter* InAdapter, int32 Index)
		: Adapter(InAdapter), CmdContext(nullptr), FrameIndex(Index)
	{
		CommandLists.Reserve(16);
	}

	virtual ~FD3D12CommandContextContainer() override
	{
	}

	virtual IRHICommandContext* GetContext() override
	{
		FD3D12Device* Device = Adapter->GetDeviceByIndex(FrameIndex);

		check(CmdContext == nullptr);
		CmdContext = Device->ObtainCommandContext();
		check(CmdContext != nullptr);
		check(CmdContext->CommandListHandle == nullptr);

		CmdContext->OpenCommandList();
		CmdContext->ClearState();

		return CmdContext;
	}

	virtual void FinishContext() override
	{
		FD3D12Device* Device = Adapter->GetDeviceByIndex(FrameIndex);

		// We never "Finish" the default context. It gets submitted when FlushCommands() is called.
		check(!CmdContext->IsDefaultContext());

		CmdContext->Finish(CommandLists);

		Device->ReleaseCommandContext(CmdContext);
		CmdContext = nullptr;
	}

	virtual void SubmitAndFreeContextContainer(int32 Index, int32 Num) override
	{
		FD3D12Device* Device = Adapter->GetDeviceByIndex(FrameIndex);

		if (Index == 0)
		{
			check((IsInRenderingThread() || IsInRHIThread()));

			FD3D12CommandContext& DefaultContext = Device->GetDefaultCommandContext();

			// Don't really submit the default context yet, just start a new command list.
			// Close the command list, add it to the pending command lists, then open a new command list (with the previous state restored).
			DefaultContext.CloseCommandList();

			Device->PendingCommandLists.Add(DefaultContext.CommandListHandle);
			Device->PendingCommandListsTotalWorkCommands +=
				DefaultContext.numClears +
				DefaultContext.numCopies +
				DefaultContext.numDraws;

			DefaultContext.OpenCommandList(true);
		}

		// Add the current lists for execution (now or possibly later depending on the command list batching mode).
		for (int32 i = 0; i < CommandLists.Num(); ++i)
		{
			Device->PendingCommandLists.Add(CommandLists[i]);
			Device->PendingCommandListsTotalWorkCommands +=
				CommandLists[i].GetCurrentOwningContext()->numClears +
				CommandLists[i].GetCurrentOwningContext()->numCopies +
				CommandLists[i].GetCurrentOwningContext()->numDraws;
		}

		CommandLists.Reset();

		bool Flush = false;
		// If the GPU is starving (i.e. we are CPU bound) feed it asap!
		if (Device->IsGPUIdle() && Device->PendingCommandLists.Num() > 0)
		{
			Flush = true;
		}
		else
		{
			if (GCommandListBatchingMode != CLB_AggressiveBatching)
			{
				// Submit when the batch is finished.
				const bool FinalCommandListInBatch = Index == (Num - 1);
				if (FinalCommandListInBatch && Device->PendingCommandLists.Num() > 0)
				{
					Flush = true;
				}
			}
		}

		if (Flush)
		{
			Device->GetCommandListManager().ExecuteCommandLists(Device->PendingCommandLists);
			Device->PendingCommandLists.Reset();
			Device->PendingCommandListsTotalWorkCommands = 0;
		}

		delete this;
	}
};

void* FD3D12CommandContextContainer::operator new(size_t Size)
{
	return FMemory::Malloc(Size);
}

void FD3D12CommandContextContainer::operator delete(void* RawMemory)
{
	FMemory::Free(RawMemory);
}

IRHICommandContextContainer* FD3D12DynamicRHI::RHIGetCommandContextContainer()
{
	return new FD3D12CommandContextContainer(&GetAdapter(), GFrameNumberRenderThread % GetAdapter().GetNumGPUNodes());
}

#endif // D3D12_SUPPORTS_PARALLEL_RHI_EXECUTE


//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FD3D12CommandContextRedirector
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

FD3D12CommandContextRedirector::FD3D12CommandContextRedirector(class FD3D12Adapter* InParent)
	: CurrentDeviceIndex(0)
	, FD3D12AdapterChild(InParent)
{
	FMemory::Memzero(PhysicalContexts, sizeof(PhysicalContexts[0]) * MAX_NUM_LDA_NODES);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// FD3D12TemporalEffect
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////

FD3D12TemporalEffect::FD3D12TemporalEffect()
	: EffectFence(nullptr, "TemporalEffectFence")
	, FD3D12AdapterChild(nullptr)
{}

FD3D12TemporalEffect::FD3D12TemporalEffect(FD3D12Adapter* Parent, const FName& InEffectName)
	: EffectFence(Parent, InEffectName.GetPlainANSIString())
	, FD3D12AdapterChild(Parent)
{}

FD3D12TemporalEffect::FD3D12TemporalEffect(const FD3D12TemporalEffect& Other)
{
	FMemory::Memcpy(EffectFence, Other.EffectFence);
}

void FD3D12TemporalEffect::Init()
{
	EffectFence.CreateFence(1);
}

void FD3D12TemporalEffect::Destroy()
{
	EffectFence.Destroy();
}

void FD3D12TemporalEffect::WaitForPrevious(ID3D12CommandQueue* Queue)
{
	const uint64 CurrentFence = EffectFence.GetCurrentFence();
	if (CurrentFence > 1)
	{
		EffectFence.GpuWait(Queue, CurrentFence - 1);
	}
}

void FD3D12TemporalEffect::SignalSyncComplete(ID3D12CommandQueue* Queue)
{
	EffectFence.Signal(Queue);
}

