// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraEmitterInstance.h: Niagara emitter simulation class
==============================================================================*/
#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "NiagaraCommon.h"
#include "NiagaraDataSet.h"
#include "NiagaraEvents.h"
#include "NiagaraCollision.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraEmitter.h"
#include "NiagaraParameterStore.h"
#include "NiagaraTypes.h"

/** 
Storage class containing actual runtime buffers to be used by the VM and the GPU. 
Is not the actual source for any parameter data, rather just the final place it's gathered from various other places ready for execution.
*/
class FNiagaraScriptExecutionParameterStore : public FNiagaraParameterStore
{
public:
	FNiagaraScriptExecutionParameterStore();
	FNiagaraScriptExecutionParameterStore(const FNiagaraParameterStore& Other);
	FNiagaraScriptExecutionParameterStore& operator=(const FNiagaraParameterStore& Other);

	//TODO: This function can probably go away entirely when we replace the FNiagaraParameters and DataInterface info in the script with an FNiagaraParameterStore.
	//Special care with prev params and internal params will have to be taken there.
	void Init(UNiagaraScript* Script, ENiagaraSimTarget SimTarget);
	void AddScriptParams(UNiagaraScript* Script, ENiagaraSimTarget SimTarget);
	void CopyCurrToPrev();

	bool AddParameter(const FNiagaraVariable& Param, bool bInitInterfaces=true)
	{
		if (FNiagaraParameterStore::AddParameter(Param, bInitInterfaces))
		{
			AddPaddedParamSize(Param.GetType(), IndexOf(Param));
			return true;
		}
		return false;
	}

	bool RemoveParameter(FNiagaraVariable& Param)
	{
		check(0);//Not allowed to remove parameters from an execution store as it will adjust the table layout mess up the 
		return false;
	}

	void RenameParameter(FNiagaraVariable& Param, FName NewName)
	{
		check(0);//Can't rename parameters for an execution store.
	}

	// Just the external parameters, not previous or internal...
	uint32 GetExternalParameterSize() { return ParameterSize; }

	// The entire buffer padded out by the required alignment of the types..
	uint32 GetPaddedParameterSizeInBytes() { return PaddedParameterSize; }

	// Helper that converts the data from the base type array internally into the padded out renderer-ready format.
	void CopyParameterDataToPaddedBuffer(uint8* InTargetBuffer, uint32 InTargetBufferSizeInBytes);

	void Clear()
	{
		Empty();
		PaddedParameterSize = 0;
	}
protected:
	void AddPaddedParamSize(const FNiagaraTypeDefinition& InParamType, uint32 InOffset);

private:

	/** Size of the parameter data not including prev frame values or internal constants. Allows copying into previous parameter values for interpolated spawn scripts. */
	int32 ParameterSize;

	uint32 PaddedParameterSize;

	struct FPaddingInfo
	{
		FPaddingInfo(uint32 InSrcOffset, uint32 InDestOffset, uint32 InSize) : SrcOffset(InSrcOffset), DestOffset(InDestOffset), Size(InSize){}
		uint32 SrcOffset;
		uint32 DestOffset;
		uint32 Size;
	};

	static void GenerateLayoutInfoInternal(TArray<FPaddingInfo>& Members, uint32& NextMemberOffset, const UStruct* InSrcStruct, uint32 InSrcOffset);
	
	TArray<FPaddingInfo> PaddingInfo;
};

struct FNiagaraDataSetExecutionInfo
{
	FNiagaraDataSetExecutionInfo()
		: DataSet(nullptr)
		, StartInstance(0)
		, bAllocate(false)
		, bUpdateInstanceCount(false)
	{
	}

	FNiagaraDataSetExecutionInfo(FNiagaraDataSet* InDataSet, int32 InStartInstance, bool bInAllocate, bool bInUpdateInstanceCount)
		: DataSet(InDataSet)
		, StartInstance(InStartInstance)
		, bAllocate(bInAllocate)
		, bUpdateInstanceCount(bInUpdateInstanceCount)
	{}

	FNiagaraDataSet* DataSet;
	int32 StartInstance;
	bool bAllocate;
	bool bUpdateInstanceCount;
};

struct FNiagaraScriptExecutionContext
{
	UNiagaraScript* Script;

	/** Table of external function delegates called from the VM. */
	TArray<FVMExternalFunction> FunctionTable;

	/** Table of instance data for data interfaces that require it. */
	TArray<void*> DataInterfaceInstDataTable;

	/** Parameter store. Contains all data interfaces and a parameter buffer that can be used directly by the VM or GPU. */
	FNiagaraScriptExecutionParameterStore Parameters;

	TArray<FDataSetMeta> DataSetMetaTable;

	static uint32 TickCounter;

	FNiagaraScriptExecutionContext();
	~FNiagaraScriptExecutionContext();

	bool Init(UNiagaraScript* InScript, ENiagaraSimTarget InTarget);
	

	bool Tick(class FNiagaraSystemInstance* Instance, ENiagaraSimTarget SimTarget = ENiagaraSimTarget::CPUSim);
	void PostTick();

	bool Execute(uint32 NumInstances, TArray<FNiagaraDataSetExecutionInfo>& DataSetInfos);

	const TArray<UNiagaraDataInterface*>& GetDataInterfaces()const { return Parameters.GetDataInterfaces(); }

	void DirtyDataInterfaces();

	bool CanExecute()const;
};




struct FNiagaraComputeExecutionContext
{
	FNiagaraComputeExecutionContext()
		: MainDataSet(nullptr)
		, SpawnRateInstances(0)
		, BurstInstances(0)
		, EventSpawnTotal(0)
		, SpawnScript(nullptr)
		, UpdateScript(nullptr)
		, RTUpdateScript(0)
		, RTSpawnScript(0)
	{
	}

	void InitParams(UNiagaraScript* InSpawnScript, UNiagaraScript *InUpdateScript, ENiagaraSimTarget SimTarget)
	{
		CombinedParamStore.Init(InSpawnScript, SimTarget);
		CombinedParamStore.AddScriptParams(InUpdateScript, SimTarget);
		SpawnScript = InSpawnScript;
		UpdateScript = InUpdateScript;
		
		FNiagaraShader *Shader = InSpawnScript->GetRenderThreadScript()->GetShaderGameThread();
		if (Shader)
		{
			DataInterfaceDescriptors = Shader->GetDIBufferDescriptors();
		}
		else
		{
			DataInterfaceDescriptors = InSpawnScript->GetRenderThreadScript()->GetDataInterfaceBufferDescriptors();
		}

	}

	void DirtyDataInterfaces()
	{
		CombinedParamStore.MarkInterfacesDirty();
	}

	bool Tick(FNiagaraSystemInstance* ParentSystemInstance)
	{
		if (CombinedParamStore.GetInterfacesDirty())
		{
			const TArray<UNiagaraDataInterface*> &DataInterfaces = CombinedParamStore.GetDataInterfaces();
			// We must make sure that the data interfaces match up between the original script values and our overrides...
			if (DataInterfaceDescriptors.Num() != DataInterfaces.Num())
			{
				UE_LOG(LogNiagara, Warning, TEXT("Mismatch between Niagara GPU Execution Context data interfaces and those in its script!"));
				return false;
			}

			//Fill the instance data table.
			if (ParentSystemInstance)
			{
				for (int32 i = 0; i < DataInterfaces.Num(); i++)
				{
					UNiagaraDataInterface* Interface = DataInterfaces[i];
					// setup GPU interface; TODO: should probably move this to the GPU execution context
					Interface->SetupBuffers(DataInterfaceDescriptors[i]);
				}
			}

			CombinedParamStore.Tick();
		}

		return true;
	}

	class FNiagaraDataSet *MainDataSet;
	TArray<FNiagaraDataSet*>UpdateEventWriteDataSets;
	TArray<FNiagaraEventScriptProperties> EventHandlerScriptProps;
	TArray<FNiagaraDataSet*> EventSets;
	uint32 SpawnRateInstances;
	uint32 BurstInstances;

	TArray<int32> EventSpawnCounts;
	uint32 EventSpawnTotal;
	UNiagaraScript *SpawnScript;
	UNiagaraScript *UpdateScript;
	class FNiagaraScript *RTUpdateScript;
	class FNiagaraScript *RTSpawnScript;
	TArray<FNiagaraScriptDataInterfaceInfo> UpdateInterfaces;
	TArray<uint8, TAlignedHeapAllocator<16>> SpawnParams;		// RT side copy of the parameter data
	FNiagaraScriptExecutionParameterStore CombinedParamStore;
	TArray< FDIBufferDescriptorStore >  DataInterfaceDescriptors;
	static uint32 TickCounter;
};
