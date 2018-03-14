// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraEmitterInstanceBatcher.h: Queueing and batching for Niagara simulation;
use to reduce per-simulation overhead by batching together simulations using
the same VectorVM byte code / compute shader code
==============================================================================*/
#pragma once

#include "CoreMinimal.h"
#include "NiagaraCommon.h"
#include "RendererInterface.h"
#include "NiagaraParameters.h"
#include "NiagaraEmitter.h"
#include "Tickable.h"
#include "ModuleManager.h"
#include "ModuleManager.h"

struct FNiagaraScriptExecutionContext;
struct FNiagaraComputeExecutionContext;

class NiagaraEmitterInstanceBatcher : public FTickableGameObject, public FComputeDispatcher
{
public:
	NiagaraEmitterInstanceBatcher()
		: CurQueueIndex(0)
	{
		IRendererModule *RendererModule = FModuleManager::GetModulePtr<IRendererModule>("Renderer");
		if (RendererModule)
		{
			RendererModule->RegisterPostOpaqueComputeDispatcher(this);
		}

		if (DummyWriteIndexBuffer.Buffer == nullptr)
		{
			DummyWriteIndexBuffer.Initialize(sizeof(int32), 64, EPixelFormat::PF_R32_SINT);	// always allocate for up to 64 data sets
		}
	}

	~NiagaraEmitterInstanceBatcher()
	{
		IRendererModule *RendererModule = FModuleManager::GetModulePtr<IRendererModule>("Renderer");
		if (RendererModule)
		{
			RendererModule->UnRegisterPostOpaqueComputeDispatcher(this);
		}
	}

	static NiagaraEmitterInstanceBatcher *Get()
	{
		if (BatcherSingleton == nullptr)
		{
			BatcherSingleton = new NiagaraEmitterInstanceBatcher();
		}
		return BatcherSingleton;
	}

	void Queue(FNiagaraComputeExecutionContext *InContext);

	virtual ETickableTickType GetTickableTickType() const override
	{
		return ETickableTickType::Always;
	}

	virtual TStatId GetStatId() const override
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(NiagaraEmitterInstanceBatcher, STATGROUP_Tickables);
	}


	virtual void Tick(float DeltaTime) override
	{
		BuildBatches();
	}

	// TODO: process queue, build batches from context with the same script
	//  also need to figure out how to handle multiple sets of parameters across a batch
	//	for now this executes every single sim in the queue individually, which is terrible 
	//	in terms of overhead
	void BuildBatches()
	{
	}

	// from FComputeDispatcher; called once per frame by the render thread, swaps buffers and works down 
	// the queue submitted by the game thread; means we're one frame behind with this; we need a mechanism to determine execution order here.
	virtual void Execute(FRHICommandList &RHICmdList)
	{
		CurQueueIndex ^= 0x1;
		ExecuteAll(RHICmdList);
	}

	void ExecuteAll(FRHICommandList &RHICmdList);
	void TickSingle(const FNiagaraComputeExecutionContext *Context, FRHICommandList &RHICmdList) const;

	void SetPrevDataStrideParams(const FNiagaraDataSet *Set, FNiagaraShader *Shader, FRHICommandList &RHICmdList) const;

	void SetupEventUAVs(const FNiagaraComputeExecutionContext *Context, uint32 NumInstances, FRHICommandList &RHICmdList) const;
	void UnsetEventUAVs(const FNiagaraComputeExecutionContext *Context, FRHICommandList &RHICmdList) const;
	void SetupDataInterfaceBuffers(const TArray<FNiagaraScriptDataInterfaceInfo> &DIInfos, FNiagaraShader *Shader, FRHICommandList &RHICmdList) const;

	void Run(	FNiagaraDataSet *DataSet,
				uint32 Phase0StartInstance,
				uint32 Phase1StartInstance,
				const uint32 TotalNumInstances,
				FNiagaraShader *Shader,
				const TArray<uint8, TAlignedHeapAllocator<16>>  &Params,
				FRHICommandList &RHICmdList,
				const FRWBuffer &WriteIndexBuffer,
				bool bCopyBeforeStart = false) const;

	void RunEventHandlers(const FNiagaraComputeExecutionContext *Context, uint32 NumInstancesAfterSim, uint32 NumInstancesAfterSpawn, uint32 NumInstancesAfterNonEventSpawn, FRHICommandList &RhiCmdList) const;
	void ResolveDatasetWrites(uint32 *OutArray, const FNiagaraComputeExecutionContext *Context, FRWBuffer &WriteIndexBuffer) const;
private:
	static NiagaraEmitterInstanceBatcher* BatcherSingleton;

	uint32 CurQueueIndex;
	TArray<FNiagaraComputeExecutionContext*> SimulationQueue[2];
	static FRWBuffer DummyWriteIndexBuffer;
};
