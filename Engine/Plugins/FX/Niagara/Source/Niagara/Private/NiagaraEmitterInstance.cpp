// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEmitterInstance.h"
#include "Materials/Material.h"
#include "VectorVM.h"
#include "NiagaraStats.h"
#include "NiagaraConstants.h"
#include "NiagaraRenderer.h"
#include "NiagaraSystemInstance.h"
#include "NiagaraDataInterface.h"
#include "NiagaraEmitterInstanceBatcher.h"
#include "NiagaraScriptExecutionContext.h"
#include "NiagaraParameterCollection.h"
#include "NiagaraScriptExecutionContext.h"
#include "NiagaraWorldManager.h"

DECLARE_DWORD_COUNTER_STAT(TEXT("Num Custom Events"), STAT_NiagaraNumCustomEvents, STATGROUP_Niagara);

//DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_NiagaraTick, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"), STAT_NiagaraSimulate, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn"), STAT_NiagaraSpawn, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn"), STAT_NiagaraEvents, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Kill"), STAT_NiagaraKill, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Event Handling"), STAT_NiagaraEventHandle, STATGROUP_Niagara);

static int32 GbDumpParticleData = 0;
static FAutoConsoleVariableRef CVarNiagaraDumpParticleData(
	TEXT("fx.DumpParticleData"),
	GbDumpParticleData,
	TEXT("If > 0 current frame particle data will be dumped after simulation. \n"),
	ECVF_Default
	);

//////////////////////////////////////////////////////////////////////////

FNiagaraEmitterInstance::FNiagaraEmitterInstance(FNiagaraSystemInstance* InParentSystemInstance)
: bError(false)
, CPUTimeMS(0.0f)
, ExecutionState(ENiagaraExecutionState::Inactive)
, CachedBounds(ForceInit)
, ParentSystemInstance(InParentSystemInstance)
{
	bDumpAfterEvent = false;
	ParticleDataSet = new FNiagaraDataSet();
}

FNiagaraEmitterInstance::~FNiagaraEmitterInstance()
{
	//UE_LOG(LogNiagara, Warning, TEXT("~Simulator %p"), this);
	ClearRenderer();
	CachedBounds.Init();
	UnbindParameters();

	/** We defer the deletion of the particle dataset to the RT to be sure all in-flight RT commands have finished using it.*/
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(FDeleteParticleDataSetCommand,
		FNiagaraDataSet*, DataSet, ParticleDataSet,
		{
			delete DataSet;
		});
}

void FNiagaraEmitterInstance::ClearRenderer()
{
	for (int32 i = 0; i < EmitterRenderer.Num(); i++)
	{
		if (EmitterRenderer[i])
		{
			//UE_LOG(LogNiagara, Warning, TEXT("ClearRenderer %p"), EmitterRenderer);
			// This queues up the renderer for deletion on the render thread..
			EmitterRenderer[i]->Release();
			EmitterRenderer[i] = nullptr;
		}
	}
}

FBox FNiagaraEmitterInstance::GetBounds()
{
	if (UNiagaraEmitter* Emitter = GetEmitterHandle().GetInstance())
	{
		if (Emitter->bFixedBounds || Emitter->SimTarget == ENiagaraSimTarget::GPUComputeSim)//We must use fixed bounds on GPU sim.
		{
			return Emitter->FixedBounds.TransformBy(ParentSystemInstance->GetComponent()->GetComponentToWorld());
		}
	}
	return CachedBounds;
}

bool FNiagaraEmitterInstance::IsReadyToRun() const
{
	const UNiagaraEmitter* PinnedProps = GetEmitterHandle().GetInstance();

	if (!PinnedProps || !PinnedProps->IsReadyToRun())
	{
		return false;
	}

	return true;
}


void FNiagaraEmitterInstance::Init(int32 InEmitterIdx, FName InSystemInstanceName)
{
	check(ParticleDataSet);
	FNiagaraDataSet& Data = *ParticleDataSet;

	EmitterIdx = InEmitterIdx;
	OwnerSystemInstanceName = InSystemInstanceName;
	Data.Init(FNiagaraDataSetID(GetEmitterHandle().GetIdName(), ENiagaraDataSetType::ParticleData));

	//Init the spawn infos to the correct number for this system.
	const TArray<FNiagaraEmitterSpawnAttributes>& EmitterSpawnInfoAttrs = ParentSystemInstance->GetSystem()->GetEmitterSpawnAttributes();
	if (EmitterSpawnInfoAttrs.IsValidIndex(EmitterIdx))
	{
		SpawnInfos.SetNum(EmitterSpawnInfoAttrs[EmitterIdx].SpawnAttributes.Num());
	}
	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();

	ResetSimulation();

	if (IsDisabled())
	{
		return;
	}

	Data.Reset();
	DataSetMap.Empty();

	const UNiagaraEmitter* PinnedProps = EmitterHandle.GetInstance();

	//Add the particle data to the data set map.
	//Currently just used for the tick loop but will also allow access directly to the particle data from other emitters.
	DataSetMap.Add(Data.GetID(), &Data);
	//Warn the user if there are any attributes used in the update script that are not initialized in the spawn script.
	//TODO: We need some window in the System editor and possibly the graph editor for warnings and errors.

	const bool bVerboseAttributeLogging = false;

	if (bVerboseAttributeLogging)
	{
		for (FNiagaraVariable& Attr : PinnedProps->UpdateScriptProps.Script->Attributes)
		{
			int32 FoundIdx;
			if (!PinnedProps->SpawnScriptProps.Script->Attributes.Find(Attr, FoundIdx))
			{
				UE_LOG(LogNiagara, Warning, TEXT("Attribute %s is used in the Update script for %s but it is not initialised in the Spawn script!"), *Attr.GetName().ToString(), *EmitterHandle.GetName().ToString());
			}
			for (int32 i = 0; i < PinnedProps->GetEventHandlers().Num(); i++)
			{
				if (PinnedProps->GetEventHandlers()[i].Script && !PinnedProps->GetEventHandlers()[i].Script->Attributes.Find(Attr, FoundIdx))
				{
					UE_LOG(LogNiagara, Warning, TEXT("Attribute %s is used in the event handler script for %s but it is not initialised in the Spawn script!"), *Attr.GetName().ToString(), *EmitterHandle.GetName().ToString());
				}
			}
		}
	}
	Data.AddVariables(PinnedProps->UpdateScriptProps.Script->Attributes);
	Data.AddVariables(PinnedProps->SpawnScriptProps.Script->Attributes);
	Data.Finalize();

	ensure(PinnedProps->UpdateScriptProps.DataSetAccessSynchronized());
	UpdateScriptEventDataSets.Empty();
	for (const FNiagaraEventGeneratorProperties &GeneratorProps : PinnedProps->UpdateScriptProps.EventGenerators)
	{
		FNiagaraDataSet *Set = FNiagaraEventDataSetMgr::CreateEventDataSet(ParentSystemInstance->GetIDName(), EmitterHandle.GetIdName(), GeneratorProps.SetProps.ID.Name);
		Set->Reset();
		Set->AddVariables(GeneratorProps.SetProps.Variables);
		Set->Finalize();
		UpdateScriptEventDataSets.Add(Set);
	}

	ensure(PinnedProps->SpawnScriptProps.DataSetAccessSynchronized());
	SpawnScriptEventDataSets.Empty();
	for (const FNiagaraEventGeneratorProperties &GeneratorProps : PinnedProps->SpawnScriptProps.EventGenerators)
	{
		FNiagaraDataSet *Set = FNiagaraEventDataSetMgr::CreateEventDataSet(ParentSystemInstance->GetIDName(), EmitterHandle.GetIdName(), GeneratorProps.SetProps.ID.Name);
		Set->Reset();
		Set->AddVariables(GeneratorProps.SetProps.Variables);
		Set->Finalize();
		SpawnScriptEventDataSets.Add(Set);
	}

	SpawnExecContext.Init(PinnedProps->SpawnScriptProps.Script, PinnedProps->SimTarget);
	UpdateExecContext.Init(PinnedProps->UpdateScriptProps.Script, PinnedProps->SimTarget);
	
	// setup the parameer store for the GPU execution context; since spawn and update are combined here, we build one with params from both script props
	if (PinnedProps->SimTarget == ENiagaraSimTarget::GPUComputeSim)
	{
		GPUExecContext.InitParams(PinnedProps->SpawnScriptProps.Script, PinnedProps->UpdateScriptProps.Script, PinnedProps->SimTarget);
		SpawnExecContext.Parameters.Bind(&GPUExecContext.CombinedParamStore);
		UpdateExecContext.Parameters.Bind(&GPUExecContext.CombinedParamStore);
	}

	EventExecContexts.SetNum(PinnedProps->GetEventHandlers().Num());
	int32 NumEvents = PinnedProps->GetEventHandlers().Num();
	for (int32 i = 0; i < NumEvents; i++)
	{
		ensure(PinnedProps->GetEventHandlers()[i].DataSetAccessSynchronized());

		UNiagaraScript* EventScript = PinnedProps->GetEventHandlers()[i].Script;

		//This is cpu explicitly? Are we doing event handlers on GPU?
		EventExecContexts[i].Init(EventScript, ENiagaraSimTarget::CPUSim);
	}

	UNiagaraEmitter* Emitter = GetEmitterHandle().GetInstance();

	//Setup direct bindings for setting parameter values.
	SpawnIntervalBinding.Init(SpawnExecContext.Parameters, Emitter->ToEmitterParameter(SYS_PARAM_EMITTER_SPAWN_INTERVAL));
	InterpSpawnStartBinding.Init(SpawnExecContext.Parameters, Emitter->ToEmitterParameter(SYS_PARAM_EMITTER_INTERP_SPAWN_START_DT));

	FNiagaraVariable EmitterAgeParam = Emitter->ToEmitterParameter(SYS_PARAM_EMITTER_AGE);
	SpawnEmitterAgeBinding.Init(SpawnExecContext.Parameters, EmitterAgeParam);
	UpdateEmitterAgeBinding.Init(UpdateExecContext.Parameters, EmitterAgeParam);
	EventEmitterAgeBindings.SetNum(NumEvents);
	for (int32 i = 0; i < NumEvents; i++)
	{
		EventEmitterAgeBindings[i].Init(EventExecContexts[i].Parameters, EmitterAgeParam);
	}

	FNiagaraVariable EmitterLocalSpaceParam = Emitter->ToEmitterParameter(SYS_PARAM_EMITTER_LOCALSPACE);
	SpawnEmitterLocalSpaceBinding.Init(SpawnExecContext.Parameters, EmitterLocalSpaceParam);
	UpdateEmitterLocalSpaceBinding.Init(UpdateExecContext.Parameters, EmitterLocalSpaceParam);
	EventEmitterLocalSpaceBindings.SetNum(NumEvents);
	for (int32 i = 0; i < NumEvents; i++)
	{
		EventEmitterLocalSpaceBindings[i].Init(EventExecContexts[i].Parameters, EmitterLocalSpaceParam);
	}

	SpawnExecCountBinding.Init(SpawnExecContext.Parameters, SYS_PARAM_ENGINE_EXEC_COUNT);
	UpdateExecCountBinding.Init(UpdateExecContext.Parameters, SYS_PARAM_ENGINE_EXEC_COUNT);
	EventExecCountBindings.SetNum(NumEvents);
	for (int32 i = 0; i < NumEvents; i++)
	{
		EventExecCountBindings[i].Init(EventExecContexts[i].Parameters, SYS_PARAM_ENGINE_EXEC_COUNT);
	}

	if (PinnedProps->SimTarget == ENiagaraSimTarget::GPUComputeSim)
	{
		//Just ensure we've generated the singleton here on the GT as it throws a wobbler if we do this later in parallel.
		NiagaraEmitterInstanceBatcher::Get();
	}

	//Init accessors for PostProcessParticles
	PositionAccessor = FNiagaraDataSetAccessor<FVector>(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	SizeAccessor = FNiagaraDataSetAccessor<FVector2D>(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("SpriteSize")));
	MeshScaleAccessor = FNiagaraDataSetAccessor<FVector>(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Scale")));
}

void FNiagaraEmitterInstance::ResetSimulation()
{
	bResetPending = true;
	Age = 0;
	Loops = 0;
	bError = false;
	
	SetExecutionState(ENiagaraExecutionState::Active);

	if (IsDisabled())
	{
		return;
	}

	CheckForErrors();
}

void FNiagaraEmitterInstance::CheckForErrors()
{
	const UNiagaraEmitter* PinnedProps = GetEmitterHandle().GetInstance();
	if (!PinnedProps)
	{
		UE_LOG(LogNiagara, Error, TEXT("Unknown Error creating Niagara Simulation. Properties were null."));
		bError = true;
		return;
	}

	//Check for various failure conditions and bail.
	if (!PinnedProps->UpdateScriptProps.Script || !PinnedProps->SpawnScriptProps.Script)
	{
		//TODO - Arbitrary named scripts. Would need some base functionality for Spawn/Udpate to be called that can be overriden in BPs for emitters with custom scripts.
		UE_LOG(LogNiagara, Error, TEXT("Emitter cannot be enabled because it's doesn't have both an update and spawn script."), *PinnedProps->GetFullName());
		bError = true;
		return;
	}

	if (PinnedProps->SpawnScriptProps.Script->DataUsage.bReadsAttriubteData)
	{
		UE_LOG(LogNiagara, Error, TEXT("%s reads attribute data and so cannot be used as a spawn script. The data being read would be invalid."), *PinnedProps->SpawnScriptProps.Script->GetName());
		bError = true;
		return;
	}
	if (PinnedProps->UpdateScriptProps.Script->Attributes.Num() == 0 || PinnedProps->SpawnScriptProps.Script->Attributes.Num() == 0)
	{
		UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's spawn or update script doesn't have any attriubtes.."));
		bError = true;
		return;
	}

	if (PinnedProps->SimTarget == ENiagaraSimTarget::CPUSim || PinnedProps->SimTarget == ENiagaraSimTarget::DynamicLoadBalancedSim)
	{
		bool bFailed = false;
		if (!PinnedProps->SpawnScriptProps.Script->DidScriptCompilationSucceed(false))
		{
			bFailed = true;
			UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's CPU Spawn script failed to compile."));
		}

		if (!PinnedProps->UpdateScriptProps.Script->DidScriptCompilationSucceed(false))
		{
			bFailed = true;
			UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's CPU Update script failed to compile."));
		}

		if (PinnedProps->GetEventHandlers().Num() != 0)
		{
			for (int32 i = 0; i < PinnedProps->GetEventHandlers().Num(); i++)
			{
				if (!PinnedProps->GetEventHandlers()[i].Script->DidScriptCompilationSucceed(false))
				{
					bFailed = true;
					UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because one of it's CPU Event scripts failed to compile."));
				}
			}
		}

		if (bFailed)
		{
			bError = true;
			return;
		}
	}

	if (PinnedProps->SimTarget == ENiagaraSimTarget::GPUComputeSim || PinnedProps->SimTarget == ENiagaraSimTarget::DynamicLoadBalancedSim)
	{
		if (PinnedProps->SpawnScriptProps.Script->IsScriptCompilationPending(true))
		{
			UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's GPU script hasn't been compiled.."));
			bError = true;
			return;
		}
		if (!PinnedProps->SpawnScriptProps.Script->DidScriptCompilationSucceed(true))
		{
			UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's GPU script failed to compile."));
			bError = true;
			return;
		}
	}
}

void FNiagaraEmitterInstance::DirtyDataInterfaces()
{
	// Make sure that our function tables need to be regenerated...
	SpawnExecContext.DirtyDataInterfaces();
	UpdateExecContext.DirtyDataInterfaces();
	GPUExecContext.DirtyDataInterfaces();

	for (FNiagaraScriptExecutionContext& EventContext : EventExecContexts)
	{
		EventContext.DirtyDataInterfaces();
	}
}

//Unsure on usage of this atm. Possibly useful in future.
// void FNiagaraEmitterInstance::RebindParameterCollection(UNiagaraParameterCollectionInstance* OldInstance, UNiagaraParameterCollectionInstance* NewInstance)
// {
// 	OldInstance->GetParameterStore().Unbind(&SpawnExecContext.Parameters);
// 	NewInstance->GetParameterStore().Bind(&SpawnExecContext.Parameters);
// 
// 	OldInstance->GetParameterStore().Unbind(&UpdateExecContext.Parameters);
// 	NewInstance->GetParameterStore().Bind(&UpdateExecContext.Parameters);
// 
// 	for (FNiagaraScriptExecutionContext& EventContext : EventExecContexts)
// 	{
// 		OldInstance->GetParameterStore().Unbind(&EventContext.Parameters);
// 		NewInstance->GetParameterStore().Bind(&EventContext.Parameters);
// 	}
// }

void FNiagaraEmitterInstance::UnbindParameters()
{
	SpawnExecContext.Parameters.UnbindFromSourceStores();
	UpdateExecContext.Parameters.UnbindFromSourceStores();

	for (int32 EventIdx = 0; EventIdx < EventExecContexts.Num(); ++EventIdx)
	{
		EventExecContexts[EventIdx].Parameters.UnbindFromSourceStores();
	}
}

void FNiagaraEmitterInstance::BindParameters()
{
	if (IsDisabled())
	{
		return;
	}

	FNiagaraWorldManager* WorldMan = ParentSystemInstance->GetWorldManager();
	check(WorldMan);

	for (UNiagaraParameterCollection* Collection : SpawnExecContext.Script->ParameterCollections)
	{
		ParentSystemInstance->GetParameterCollectionInstance(Collection)->GetParameterStore().Bind(&SpawnExecContext.Parameters);
	}
	for (UNiagaraParameterCollection* Collection : UpdateExecContext.Script->ParameterCollections)
	{
		ParentSystemInstance->GetParameterCollectionInstance(Collection)->GetParameterStore().Bind(&UpdateExecContext.Parameters);
	}

	for (int32 EventIdx = 0; EventIdx < EventExecContexts.Num(); ++EventIdx)
	{
		for (UNiagaraParameterCollection* Collection : EventExecContexts[EventIdx].Script->ParameterCollections)
		{
			ParentSystemInstance->GetParameterCollectionInstance(Collection)->GetParameterStore().Bind(&EventExecContexts[EventIdx].Parameters);
		}
	}

	//Now bind parameters from the component and system.
	FNiagaraParameterStore& InstanceParams = ParentSystemInstance->GetParameters();
	InstanceParams.Bind(&SpawnExecContext.Parameters);
	InstanceParams.Bind(&UpdateExecContext.Parameters);
	for (FNiagaraScriptExecutionContext& EventContext : EventExecContexts)
	{
		InstanceParams.Bind(&EventContext.Parameters);
	}

	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	const UNiagaraEmitter* Props = EmitterHandle.GetInstance();
	Props->SpawnScriptProps.Script->RapidIterationParameters.Bind(&SpawnExecContext.Parameters);
	Props->UpdateScriptProps.Script->RapidIterationParameters.Bind(&UpdateExecContext.Parameters);
	ensure(Props->GetEventHandlers().Num() == EventExecContexts.Num());
	for (int32 i = 0; i < Props->GetEventHandlers().Num(); i++)
	{
		Props->GetEventHandlers()[i].Script->RapidIterationParameters.Bind(&EventExecContexts[i].Parameters);
	}
}

void FNiagaraEmitterInstance::PostInitSimulation()
{
	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	if (!bError)
	{
		check(ParentSystemInstance);
		const UNiagaraEmitter* Props = EmitterHandle.GetInstance();

		//Go through all our receivers and grab their generator sets so that the source emitters can do any init work they need to do.
		for (const FNiagaraEventReceiverProperties& Receiver : Props->SpawnScriptProps.EventReceivers)
		{
			//FNiagaraDataSet* ReceiverSet = ParentSystemInstance->GetDataSet(FNiagaraDataSetID(Receiver.SourceEventGenerator, ENiagaraDataSetType::Event), Receiver.SourceEmitter);
			const FNiagaraDataSet* ReceiverSet = FNiagaraEventDataSetMgr::GetEventDataSet(ParentSystemInstance->GetIDName(), Receiver.SourceEmitter, Receiver.SourceEventGenerator);

		}

		for (const FNiagaraEventReceiverProperties& Receiver : Props->UpdateScriptProps.EventReceivers)
		{
			//FNiagaraDataSet* ReceiverSet = ParentSystemInstance->GetDataSet(FNiagaraDataSetID(Receiver.SourceEventGenerator, ENiagaraDataSetType::Event), Receiver.SourceEmitter);
			const FNiagaraDataSet* ReceiverSet = FNiagaraEventDataSetMgr::GetEventDataSet(ParentSystemInstance->GetIDName(), Receiver.SourceEmitter, Receiver.SourceEventGenerator);
		}
	}
}

FNiagaraDataSet* FNiagaraEmitterInstance::GetDataSet(FNiagaraDataSetID SetID)
{
	FNiagaraDataSet** SetPtr = DataSetMap.Find(SetID);
	FNiagaraDataSet* Ret = NULL;
	if (SetPtr)
	{
		Ret = *SetPtr;
	}
	else
	{
		// TODO: keep track of data sets generated by the scripts (event writers) and find here
	}

	return Ret;
}

const FNiagaraEmitterHandle& FNiagaraEmitterInstance::GetEmitterHandle() const
{
	return ParentSystemInstance->GetSystem()->GetEmitterHandles()[EmitterIdx];
}

float FNiagaraEmitterInstance::GetTotalCPUTime()
{
	float Total = CPUTimeMS;
	for (int32 i = 0; i < EmitterRenderer.Num(); i++)
	{
		if (EmitterRenderer[i])
		{
			Total += EmitterRenderer[i]->GetCPUTimeMS();

		}
	}

	return Total;
}

int FNiagaraEmitterInstance::GetTotalBytesUsed()
{
	check(ParticleDataSet);
	int32 BytesUsed = ParticleDataSet->GetSizeBytes();
	/*
	for (FNiagaraDataSet& Set : DataSets)
	{
		BytesUsed += Set.GetSizeBytes();
	}
	*/
	return BytesUsed;
}

FBox FNiagaraEmitterInstance::CalculateDynamicBounds()
{
	check(ParticleDataSet);
	FNiagaraDataSet& Data = *ParticleDataSet;
	int32 NumParticles = Data.GetNumInstances();
	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	if (NumParticles == 0 || EmitterHandle.GetInstance()->SimTarget == ENiagaraSimTarget::GPUComputeSim)//TODO: Pull data back from gpu buffers to get bounds for GPU sims.
	{
		return FBox();
	}

	const UNiagaraEmitter* Emitter = EmitterHandle.GetInstance();
	FBox Ret;
	Ret.Init();

	PositionAccessor.InitForAccess(true);
	SizeAccessor.InitForAccess(true);
	MeshScaleAccessor.InitForAccess(true);
	int32 NumInstances = Data.GetNumInstances();

	FVector MaxSize(EForceInit::ForceInitToZero);

	FMatrix SystemLocalToWorld = GetParentSystemInstance()->GetComponent()->GetComponentTransform().ToMatrixNoScale();

	for (int32 InstIdx = 0; InstIdx < NumInstances; ++InstIdx)
	{
		FVector Position;
		PositionAccessor.Get(InstIdx, Position);

		// Some graphs have a tendency to divide by zero. This ContainsNaN has been added prophylactically
		// to keep us safe during GDC. It should be removed as soon as we feel safe that scripts are appropriately warned.
		if (!Position.ContainsNaN())
		{
			Ret += Position;

			// We advance the scale or size depending of if we use either.
			if (MeshScaleAccessor.IsValid())
			{
				MaxSize = MaxSize.ComponentMax(MeshScaleAccessor.Get(InstIdx));
			}
			else if (SizeAccessor.IsValid())
			{
				MaxSize = MaxSize.ComponentMax(FVector(SizeAccessor.Get(InstIdx).GetMax()));
			}
		}
		else
		{
			UE_LOG(LogNiagara, Warning, TEXT("Particle position data contains NaNs. Likely a divide by zero somewhere in your modules."));
		}
	}

	FVector MaxBaseSize = FVector(0.0001f, 0.0001f, 0.0001f);
	for (int32 i = 0; i < EmitterRenderer.Num(); i++)
	{
		if (EmitterRenderer[i])
		{
			FVector BaseExtents = EmitterRenderer[i]->GetBaseExtents();
			FVector ComponentMax;
			MaxBaseSize = BaseExtents.ComponentMax(MaxBaseSize);
		}
	}

	Ret = Ret.ExpandBy(MaxSize*MaxBaseSize);

	if (Emitter->bLocalSpace)
	{
		Ret = Ret.TransformBy(SystemLocalToWorld);
	}

	return Ret;
}

/** Look for dead particles and move from the end of the list to the dead location, compacting in the process
  * Also calculates bounds; Kill will be removed from this once we do conditional write
  */
void FNiagaraEmitterInstance::PostProcessParticles()
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraKill);
	check(ParticleDataSet);
	FNiagaraDataSet& Data = *ParticleDataSet;
	int32 NumParticles = Data.GetNumInstances();

	CachedBounds.Init();

	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	if (NumParticles == 0 || EmitterHandle.GetInstance()->SimTarget == ENiagaraSimTarget::GPUComputeSim)
	{
		return;
	}
	
	const UNiagaraEmitter* Emitter = EmitterHandle.GetInstance();
	if (Emitter->bFixedBounds)
	{
		CachedBounds = Emitter->FixedBounds;
	}
	else
	{
		CachedBounds = CalculateDynamicBounds();
	}
}

bool FNiagaraEmitterInstance::HandleCompletion(bool bForce)
{
	if (bForce)
	{
		SetExecutionState(ENiagaraExecutionState::Complete);
	}

	if (IsComplete())
	{
		//If we have any particles then clear out the buffers.
		if (ParticleDataSet->GetNumInstances() > 0 || ParticleDataSet->GetPrevNumInstances() > 0)
		{
			ParticleDataSet->ResetBuffers(true);
		}
		return true;
	}

	return false;
}

/** 
  * PreTick - handles killing dead particles, emitter death, and buffer swaps
  */
void FNiagaraEmitterInstance::PreTick()
{
	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	const UNiagaraEmitter* PinnedProps = EmitterHandle.GetInstance();

	if (!PinnedProps || bError || IsComplete())
	{
		return;
	}

	check(ParticleDataSet);
	FNiagaraDataSet& Data = *ParticleDataSet;

	PinnedProps->SpawnScriptProps.Script->RapidIterationParameters.Tick();
	PinnedProps->UpdateScriptProps.Script->RapidIterationParameters.Tick();
	ensure(PinnedProps->GetEventHandlers().Num() == EventExecContexts.Num());
	for (int32 i = 0; i < PinnedProps->GetEventHandlers().Num(); i++)
	{
		PinnedProps->GetEventHandlers()[i].Script->RapidIterationParameters.Tick();
	}

	bool bOk = true;
	bOk &= SpawnExecContext.Tick(ParentSystemInstance);
	bOk &= UpdateExecContext.Tick(ParentSystemInstance);
	if (PinnedProps->SimTarget == ENiagaraSimTarget::GPUComputeSim)
	{
		bOk &= GPUExecContext.Tick(ParentSystemInstance);
	}
	for (FNiagaraScriptExecutionContext& EventContext : EventExecContexts)
	{
		bOk &= EventContext.Tick(ParentSystemInstance);
	}

	if (!bOk)
	{
		ResetSimulation();
		bError = true;
		return;
	}

	check(Data.GetNumVariables() > 0);
	check(PinnedProps->SpawnScriptProps.Script);
	check(PinnedProps->UpdateScriptProps.Script);

	if (bResetPending)
	{
		Data.SetNumInstances(0);
		bResetPending = false;
	}

	//Swap all data set buffers before doing the main tick on any simulation.
	if (PinnedProps->SimTarget == ENiagaraSimTarget::CPUSim)
	{
		for (TPair<FNiagaraDataSetID, FNiagaraDataSet*> SetPair : DataSetMap)
		{
			SetPair.Value->Tick(PinnedProps->SimTarget);
		}

		for (FNiagaraDataSet* Set : UpdateScriptEventDataSets)
		{
			Set->Tick(PinnedProps->SimTarget);
		}

		for (FNiagaraDataSet* Set : SpawnScriptEventDataSets)
		{
			Set->Tick(PinnedProps->SimTarget);
		}
	}
}


void FNiagaraEmitterInstance::Tick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraTick);
	SimpleTimer TickTime;

	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	const UNiagaraEmitter* PinnedProps = EmitterHandle.GetInstance();
	if (!PinnedProps || bError)
	{
		return;
	}

	check(ParticleDataSet);
	FNiagaraDataSet& Data = *ParticleDataSet;

	if (HandleCompletion())
	{
		return;
	}

	Age += DeltaSeconds;

	if (ExecutionState == ENiagaraExecutionState::InactiveClear)
	{
		Data.ResetBuffers(true);
		ExecutionState = ENiagaraExecutionState::Inactive;
		return;
	}

	int32 OrigNumParticles = Data.GetPrevNumInstances();
	if (OrigNumParticles == 0 && ExecutionState != ENiagaraExecutionState::Active)
	{
		//Clear out curr buffer in case it had some data in previously.
		Data.Allocate(0, PinnedProps->SimTarget);
		return;
	}

	check(Data.GetNumVariables() > 0);
	check(PinnedProps->SpawnScriptProps.Script);
	check(PinnedProps->UpdateScriptProps.Script);
	
	//TickEvents(DeltaSeconds);

	// add system constants
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraConstants);
		float InvDT = 1.0f / DeltaSeconds;

		//TODO: Create a binding helper object for these to avoid the search.
		SpawnEmitterAgeBinding.SetValue(Age);
		UpdateEmitterAgeBinding.SetValue(Age);
		for (FNiagaraParameterDirectBinding<float>& Binding : EventEmitterAgeBindings)
		{
			Binding.SetValue(Age);
		}
		
		SpawnEmitterLocalSpaceBinding.SetValue(PinnedProps->bLocalSpace);
		UpdateEmitterLocalSpaceBinding.SetValue(PinnedProps->bLocalSpace);
		for (FNiagaraParameterDirectBinding<FNiagaraBool>& Binding : EventEmitterLocalSpaceBindings)
		{
			Binding.SetValue(PinnedProps->bLocalSpace);
		}
	}
	
	// Calculate number of new particles from regular spawning 
	uint32 SpawnTotal = 0;
	if (ExecutionState == ENiagaraExecutionState::Active)
	{
		for (FNiagaraSpawnInfo& Info : SpawnInfos)
		{
			if (Info.Count > 0)
			{
				SpawnTotal += Info.Count;
			}
		}
	}

	// Calculate number of new particles from all event related spawns
	TArray<TArray<int32>> EventSpawnCounts;
	EventSpawnCounts.AddDefaulted(PinnedProps->GetEventHandlers().Num());
	TArray<int32> EventHandlerSpawnCounts;
	EventHandlerSpawnCounts.AddDefaulted(PinnedProps->GetEventHandlers().Num());
	uint32 EventSpawnTotal = 0;
	TArray<FNiagaraDataSet*> EventSet;
	EventSet.AddZeroed(PinnedProps->GetEventHandlers().Num());
	TArray<FGuid> SourceEmitterGuid;
	SourceEmitterGuid.AddDefaulted(PinnedProps->GetEventHandlers().Num());
	TArray<FName> SourceEmitterName;
	SourceEmitterName.AddDefaulted(PinnedProps->GetEventHandlers().Num());
	TArray<bool> bPerformEventSpawning;
	bPerformEventSpawning.AddDefaulted(PinnedProps->GetEventHandlers().Num());

	for (int32 i = 0; i < PinnedProps->GetEventHandlers().Num(); i++)
	{
		const FNiagaraEventScriptProperties &EventHandlerProps = PinnedProps->GetEventHandlers()[i];
		SourceEmitterGuid[i] = EventHandlerProps.SourceEmitterID;
		SourceEmitterName[i] = SourceEmitterGuid[i].IsValid() ? *SourceEmitterGuid[i].ToString() : EmitterHandle.GetIdName();
		EventSet[i] = FNiagaraEventDataSetMgr::GetEventDataSet(ParentSystemInstance->GetIDName(), SourceEmitterName[i], EventHandlerProps.SourceEventName);
		bPerformEventSpawning[i] = (ExecutionState == ENiagaraExecutionState::Active && EventHandlerProps.Script && EventHandlerProps.ExecutionMode == EScriptExecutionMode::SpawnedParticles);
		if (bPerformEventSpawning[i])
		{
			uint32 EventSpawnNum = CalculateEventSpawnCount(EventHandlerProps, EventSpawnCounts[i], EventSet[i]);
			EventSpawnTotal += EventSpawnNum;
			EventHandlerSpawnCounts[i] = EventSpawnNum;
		}
	}


	/* GPU simulation -  we just create an FNiagaraComputeExecutionContext, queue it, and let the batcher take care of the rest
	 */
	if (PinnedProps->SimTarget == ENiagaraSimTarget::GPUComputeSim)	
	{
		//FNiagaraComputeExecutionContext *ComputeContext = new FNiagaraComputeExecutionContext();
		GPUExecContext.MainDataSet = &Data;
		GPUExecContext.RTSpawnScript = PinnedProps->SpawnScriptProps.Script->GetRenderThreadScript();
		GPUExecContext.RTUpdateScript = PinnedProps->UpdateScriptProps.Script->GetRenderThreadScript();
		GPUExecContext.SpawnRateInstances = SpawnTotal;
		GPUExecContext.BurstInstances = 0;
		GPUExecContext.EventSpawnTotal = EventSpawnTotal;

		GPUExecContext.UpdateInterfaces = PinnedProps->UpdateScriptProps.Script->DataInterfaceInfo;

		// copy over the constants for the render thread
		//
		int32 ParmSize = GPUExecContext.CombinedParamStore.GetPaddedParameterSizeInBytes();
		if (ParmSize)
		{
#if WITH_EDITORONLY_DATA
			if (SpawnExecContext.Script->GetDebuggerInfo().bRequestDebugFrame)
			{
				SpawnExecContext.Script->GetDebuggerInfo().DebugFrame.Reset();
				SpawnExecContext.Script->GetDebuggerInfo().bRequestDebugFrame = false;
				SpawnExecContext.Script->GetDebuggerInfo().DebugFrameLastWriteId = SpawnExecContext.TickCounter;
				SpawnExecContext.Script->GetDebuggerInfo().DebugParameters = GPUExecContext.CombinedParamStore;
			}
#endif
		}
		GPUExecContext.SpawnParams.SetNumZeroed(ParmSize);
		GPUExecContext.CombinedParamStore.CopyParameterDataToPaddedBuffer(GPUExecContext.SpawnParams.GetData(), ParmSize);

		// push event data sets to the context
		for (FNiagaraDataSet *Set : UpdateScriptEventDataSets)
		{
			GPUExecContext.UpdateEventWriteDataSets.Add(Set);
		}

		GPUExecContext.EventHandlerScriptProps = PinnedProps->GetEventHandlers();
		GPUExecContext.EventSets = EventSet;
		GPUExecContext.EventSpawnCounts = EventHandlerSpawnCounts;
		NiagaraEmitterInstanceBatcher::Get()->Queue(&GPUExecContext);

		CachedBounds.Init();
		CachedBounds = CachedBounds.ExpandBy(FVector(20.0, 20.0, 20.0));	// temp until GPU sims update bounds
		return;
	}

	int32 AllocationSize = OrigNumParticles + SpawnTotal + EventSpawnTotal;
	//Allocate space for prev frames particles and any new one's we're going to spawn.
	Data.Allocate(AllocationSize, PinnedProps->SimTarget);


	TArray<FNiagaraDataSetExecutionInfo> DataSetExecInfos;
	DataSetExecInfos.Emplace(&Data, 0, false, true);

	// Simulate existing particles forward by DeltaSeconds.
	if (OrigNumParticles > 0)
	{
		/*
		if (bDumpAfterEvent)
		{
			Data.Dump(false);
			bDumpAfterEvent = false;
		}
		*/

		Data.SetNumInstances(OrigNumParticles);
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimulate);

		UpdateExecCountBinding.SetValue(OrigNumParticles);
		DataSetExecInfos.SetNum(1, false);
		DataSetExecInfos[0].StartInstance = 0;
		for (FNiagaraDataSet* EventDataSet : UpdateScriptEventDataSets)
		{
			DataSetExecInfos.Emplace(EventDataSet, 0, true, false);
		}
		UpdateExecContext.Execute(OrigNumParticles, DataSetExecInfos);
		int32 DeltaParticles = Data.GetNumInstances() - OrigNumParticles;

		ensure(DeltaParticles <= 0); // We either lose particles or stay the same, we should never add particles in update!

		if (GbDumpParticleData)
		{
			UE_LOG(LogNiagara, Log, TEXT("=== Update Parameters ===") );
			UpdateExecContext.Parameters.Dump();

			UE_LOG(LogNiagara, Log, TEXT("=== Updated %d Particles (%d Died) ==="), OrigNumParticles, -DeltaParticles);
			Data.Dump(true, 0, OrigNumParticles);
		}
	}

	uint32 EventSpawnStart = Data.GetNumInstances();

	//Init new particles with the spawn script.
	if (SpawnTotal + EventSpawnTotal > 0)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSpawn);

		//Handle main spawn rate spawning
		auto SpawnParticles = [&](int32 Num, FString DumpLabel)
		{
			if (Num > 0)
			{
				int32 OrigNum = Data.GetNumInstances();
				Data.SetNumInstances(OrigNum + Num);

				SpawnExecCountBinding.SetValue(Num);
				DataSetExecInfos.SetNum(1, false);
				DataSetExecInfos[0].StartInstance = OrigNum;
				//UE_LOG(LogNiagara, Log, TEXT("SpawnScriptEventDataSets: %d"), SpawnScriptEventDataSets.Num());
				for (FNiagaraDataSet* EventDataSet : SpawnScriptEventDataSets)
				{
					//UE_LOG(LogNiagara, Log, TEXT("SpawnScriptEventDataSets.. %d"), EventDataSet->GetNumVariables());
					DataSetExecInfos.Emplace(EventDataSet, OrigNum, true, false);
				}
				
				SpawnExecContext.Execute(Num, DataSetExecInfos);

				if (GbDumpParticleData)
				{
					UE_LOG(LogNiagara, Log, TEXT("=== %s Spawn Parameters ==="), *DumpLabel);
					SpawnExecContext.Parameters.Dump();
					UE_LOG(LogNiagara, Log, TEXT("===  %s Spawned %d Particles==="), *DumpLabel, Num);
					Data.Dump(true, OrigNum, Num);
				}
			}
		};

		//Perform all our regular spawning that's driven by our emitter script.
		for (FNiagaraSpawnInfo& Info : SpawnInfos)
		{
			SpawnIntervalBinding.SetValue(Info.IntervalDt);
			InterpSpawnStartBinding.SetValue(Info.InterpStartDt);

			SpawnParticles(Info.Count, TEXT("Regular Spawn"));
		};

		EventSpawnStart = Data.GetNumInstances();

		for (int32 EventScriptIdx = 0; EventScriptIdx < PinnedProps->GetEventHandlers().Num(); EventScriptIdx++)
		{
			//Spawn particles coming from events.
			for (int32 i = 0; i < EventSpawnCounts[EventScriptIdx].Num(); i++)
			{
				int32 EventNumToSpawn = EventSpawnCounts[EventScriptIdx][i];

				//Event spawns are instantaneous at the middle of the frame?
				SpawnIntervalBinding.SetValue(0.0f);
				InterpSpawnStartBinding.SetValue(DeltaSeconds * 0.5f);

				SpawnParticles(EventNumToSpawn, TEXT("Event Spawn"));
			}
		}
	}
	/*else if (SpawnTotal + EventSpawnTotal > 0)
	{
		UE_LOG(LogNiagara, Log, TEXT("Skipping spawning due to execution state! %d"), (uint32)ExecutionState)
	}*/

	// handle event based spawning
	for (int32 EventScriptIdx = 0;  EventScriptIdx < PinnedProps->GetEventHandlers().Num(); EventScriptIdx++)
	{
		const FNiagaraEventScriptProperties &EventHandlerProps = PinnedProps->GetEventHandlers()[EventScriptIdx];

		if (bPerformEventSpawning[EventScriptIdx] && EventSet[EventScriptIdx] && EventSpawnCounts[EventScriptIdx].Num())
		{
			uint32 NumParticles = Data.GetNumInstances();
			SCOPE_CYCLE_COUNTER(STAT_NiagaraEventHandle);

			Data.Tick();
			Data.CopyPrevToCur();

			for (int32 i = 0; i < EventSpawnCounts[EventScriptIdx].Num(); i++)
			{
				int32 EventNumToSpawn = EventSpawnCounts[EventScriptIdx][i];
				ensure(EventNumToSpawn + EventSpawnStart <= Data.GetNumInstances());
				EventExecCountBindings[EventScriptIdx].SetValue(EventNumToSpawn);

				DataSetExecInfos.SetNum(1, false);
				DataSetExecInfos[0].StartInstance = EventSpawnStart;
				DataSetExecInfos[0].bUpdateInstanceCount = false;
				DataSetExecInfos.Emplace(EventSet[EventScriptIdx], i, false, false);
				EventExecContexts[EventScriptIdx].Execute(EventNumToSpawn, DataSetExecInfos);
				
				if (GbDumpParticleData)
				{
					UE_LOG(LogNiagara, Log, TEXT("=== Event %d Parameters ==="), EventScriptIdx);
					EventExecContexts[EventScriptIdx].Parameters.Dump();
					UE_LOG(LogNiagara, Log, TEXT("=== Event %d %d Particles ==="), EventScriptIdx, EventNumToSpawn);
					Data.Dump(true, EventSpawnStart, EventNumToSpawn);
				}

				ensure(Data.GetNumInstances() == NumParticles);

				EventSpawnStart += EventNumToSpawn;
			}
		}


		// handle all-particle events
		if (EventHandlerProps.Script && EventHandlerProps.ExecutionMode == EScriptExecutionMode::EveryParticle && EventSet[EventScriptIdx])
		{
			uint32 NumParticles = Data.GetNumInstances();
			if (EventSet[EventScriptIdx]->GetPrevNumInstances())
			{
				SCOPE_CYCLE_COUNTER(STAT_NiagaraEventHandle);
				
				for (uint32 i = 0; i < EventSet[EventScriptIdx]->GetPrevNumInstances(); i++)
				{
					// If we have events, Swap buffers, to make sure we don't overwrite previous script results
					// and copy prev to cur, because the event script isn't likely to write all attributes
					// TODO: copy only event script's unused attributes
					Data.Tick();
					Data.CopyPrevToCur();

					uint32 NumInstancesPrev = Data.GetPrevNumInstances();
					EventExecCountBindings[EventScriptIdx].SetValue(NumInstancesPrev);
					DataSetExecInfos.SetNum(1, false);
					DataSetExecInfos[0].StartInstance = 0;
					DataSetExecInfos.Emplace(EventSet[EventScriptIdx], i, false, false);
					/*if (GbDumpParticleData)
					{
						UE_LOG(LogNiagara, Log, TEXT("=== Event %d [%d] Payload ==="), EventScriptIdx, i);
						DataSetExecInfos[1].DataSet->Dump(true, 0, 1);
					}*/

					EventExecContexts[EventScriptIdx].Execute(NumInstancesPrev, DataSetExecInfos);

					if (GbDumpParticleData)
					{
						UE_LOG(LogNiagara, Log, TEXT("=== Event %d [%d] Parameters ==="), EventScriptIdx, i);
						EventExecContexts[EventScriptIdx].Parameters.Dump();
						UE_LOG(LogNiagara, Log, TEXT("=== Event %d %d Particles ==="), EventScriptIdx, NumInstancesPrev);
						Data.Dump(true, 0, NumInstancesPrev);
					}

					ensure(NumParticles == Data.GetNumInstances());
				}
			}
		}


		// handle single-particle events
		// TODO: we'll need a way to either skip execution of the VM if an index comes back as invalid, or we'll have to pre-process
		// event/particle arrays; this is currently a very naive (and comparatively slow) implementation, until full indexed reads work
		if (EventHandlerProps.Script && EventHandlerProps.ExecutionMode == EScriptExecutionMode::SingleParticle && EventSet[EventScriptIdx])
		{

			SCOPE_CYCLE_COUNTER(STAT_NiagaraEventHandle);
			FNiagaraVariable IndexVar(FNiagaraTypeDefinition::GetIntDef(), "ParticleIndex");
			FNiagaraDataSetIterator<int32> IndexItr(*EventSet[EventScriptIdx], IndexVar, 0, false);
			if (IndexItr.IsValid() && EventSet[EventScriptIdx]->GetPrevNumInstances() > 0)
			{
				EventExecCountBindings[EventScriptIdx].SetValue(1);

				Data.Tick();
				Data.CopyPrevToCur();
				uint32 NumParticles = Data.GetNumInstances();

				for (uint32 i = 0; i < EventSet[EventScriptIdx]->GetPrevNumInstances(); i++)
				{
					int32 Index = *IndexItr;
					IndexItr.Advance();
					DataSetExecInfos.SetNum(1, false);
					DataSetExecInfos[0].StartInstance = Index;
					DataSetExecInfos[0].bUpdateInstanceCount = false;
					DataSetExecInfos.Emplace(EventSet[EventScriptIdx], i, false, false);
					EventExecContexts[EventScriptIdx].Execute(1, DataSetExecInfos);

					if (GbDumpParticleData)
					{
						ensure(EventHandlerProps.Script->RapidIterationParameters.VerifyBinding(&EventExecContexts[EventScriptIdx].Parameters));
						UE_LOG(LogNiagara, Log, TEXT("=== Event %d Src Parameters ==="), EventScriptIdx);
						EventHandlerProps.Script->RapidIterationParameters.Dump();
						UE_LOG(LogNiagara, Log, TEXT("=== Event %d Context Parameters ==="), EventScriptIdx);
						EventExecContexts[EventScriptIdx].Parameters.Dump();
						UE_LOG(LogNiagara, Log, TEXT("=== Event %d Particles (%d index written, %d total) ==="), EventScriptIdx, Index, Data.GetNumInstances());
						Data.Dump(true, Index, 1);
					}
					ensure(NumParticles == Data.GetNumInstances());
				}
			}
		}
	}

	PostProcessParticles();

	SpawnExecContext.PostTick();
	UpdateExecContext.PostTick();
	for (FNiagaraScriptExecutionContext& EventContext : EventExecContexts)
	{
		EventContext.PostTick();
	}

	CPUTimeMS = TickTime.GetElapsedMilliseconds();

	INC_DWORD_STAT_BY(STAT_NiagaraNumParticles, Data.GetNumInstances());
}


/** Calculate total number of spawned particles from events; these all come from event handler script with the SpawnedParticles execution mode
 *  We get the counts ahead of event processing time so we only have to allocate new particles once
 *  TODO: augment for multiple spawning event scripts
 */
uint32 FNiagaraEmitterInstance::CalculateEventSpawnCount(const FNiagaraEventScriptProperties &EventHandlerProps, TArray<int32> &EventSpawnCounts, FNiagaraDataSet *EventSet)
{
	uint32 EventSpawnTotal = 0;
	int32 NumEventsToProcess = 0;

	if (EventSet)
	{
		NumEventsToProcess = EventSet->GetPrevNumInstances();
		if(EventHandlerProps.MaxEventsPerFrame > 0)
		{
			NumEventsToProcess = FMath::Min<int32>(EventSet->GetPrevNumInstances(), EventHandlerProps.MaxEventsPerFrame);
		}

		for (int32 i = 0; i < NumEventsToProcess; i++)
		{
			if (ExecutionState == ENiagaraExecutionState::Active && EventHandlerProps.SpawnNumber > 0)
			{
				EventSpawnCounts.Add(EventHandlerProps.SpawnNumber);
				EventSpawnTotal += EventHandlerProps.SpawnNumber;
			}
		}
	}

	return EventSpawnTotal;
}

void FNiagaraEmitterInstance::SetExecutionState(ENiagaraExecutionState InState)
{
	/*if (InState != ExecutionState)
	{
		const UEnum* EnumPtr = FNiagaraTypeDefinition::GetExecutionStateEnum();
		UE_LOG(LogNiagara, Log, TEXT("Emitter \"%s\" change state: %s to %s"), *GetEmitterHandle().GetName().ToString(), *EnumPtr->GetNameStringByValue((int64)ExecutionState),
			*EnumPtr->GetNameStringByValue((int64)InState));
	}*/

	/*if (InState == ENiagaraExecutionState::Active && ExecutionState == ENiagaraExecutionState::Inactive)
	{
		UE_LOG(LogNiagara, Log, TEXT("Emitter \"%s\" change state N O O O O O "), *GetEmitterHandle().GetName().ToString());
	}*/

	if (InState != ENiagaraExecutionState::Disabled && GetEmitterHandle().GetInstance()->IsAllowedByDetailLevel() && GetEmitterHandle().GetIsEnabled())
	{
		ExecutionState = InState;
	}
	else
	{
		ExecutionState = ENiagaraExecutionState::Disabled;
	}
}


#if WITH_EDITORONLY_DATA

bool FNiagaraEmitterInstance::CheckAttributesForRenderer(int32 Index)
{
	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	if (Index > EmitterRenderer.Num())
	{
		return false;
	}

	check(ParticleDataSet);
	FNiagaraDataSet& Data = *ParticleDataSet;
	bool bOk = true;
	if (EmitterRenderer[Index])
	{
		const TArray<FNiagaraVariable>& RequiredAttrs = EmitterRenderer[Index]->GetRequiredAttributes();

		for (FNiagaraVariable Attr : RequiredAttrs)
		{
			// TODO .. should we always be namespaced?
			FString AttrName = Attr.GetName().ToString();
			if (AttrName.RemoveFromStart(TEXT("Particles.")))
			{
				Attr.SetName(*AttrName);
			}

			if (!Data.HasVariable(Attr))
			{
				bOk = false;
				UE_LOG(LogNiagara, Error, TEXT("Cannot render %s because it does not define attribute %s %s."), *EmitterHandle.GetName().ToString(), *Attr.GetType().GetNameText().ToString() , *Attr.GetName().ToString());
			}
		}

		EmitterRenderer[Index]->SetEnabled(bOk);
	}
	return bOk;
}

#endif

/** Replace the current System renderer with a new one of Type.
Don't forget to call RenderModuleUpdate on the SceneProxy after calling this! 
 */
void FNiagaraEmitterInstance::UpdateEmitterRenderer(ERHIFeatureLevel::Type FeatureLevel, TArray<NiagaraRenderer*>& ToBeAddedList, TArray<NiagaraRenderer*>& ToBeRemovedList)
{
	const FNiagaraEmitterHandle& EmitterHandle = GetEmitterHandle();
	const UNiagaraEmitter* EmitterProperties = EmitterHandle.GetInstance();

	if (EmitterProperties != nullptr)
	{
		// Add all the old to be purged..
		for (int32 SubIdx = 0; SubIdx < EmitterRenderer.Num(); SubIdx++)
		{
			if (EmitterRenderer[SubIdx] != nullptr)
			{
				ToBeRemovedList.Add(EmitterRenderer[SubIdx]);
				EmitterRenderer[SubIdx] = nullptr;
			}
		}

		if (!IsComplete() && !bError)
		{
			EmitterRenderer.Empty();
			EmitterRenderer.AddZeroed(EmitterProperties->GetRenderers().Num());
			for (int32 SubIdx = 0; SubIdx < EmitterProperties->GetRenderers().Num(); SubIdx++)
			{
				UMaterialInterface *Material = nullptr;

				TArray<UMaterialInterface*> UsedMats;
				if (EmitterProperties->GetRenderers()[SubIdx] != nullptr)
				{
					EmitterProperties->GetRenderers()[SubIdx]->GetUsedMaterials(UsedMats);
					if (UsedMats.Num() != 0)
					{
						Material = UsedMats[0];
					}
				}

				if (Material == nullptr)
				{
					Material = UMaterial::GetDefaultMaterial(MD_Surface);
				}

				if (EmitterProperties->GetRenderers()[SubIdx] != nullptr)
				{
					EmitterRenderer[SubIdx] = EmitterProperties->GetRenderers()[SubIdx]->CreateEmitterRenderer(FeatureLevel);
					EmitterRenderer[SubIdx]->SetMaterial(Material, FeatureLevel);
					EmitterRenderer[SubIdx]->SetLocalSpace(EmitterProperties->bLocalSpace);
					ToBeAddedList.Add(EmitterRenderer[SubIdx]);

					//UE_LOG(LogNiagara, Warning, TEXT("CreateRenderer %p"), EmitterRenderer);
#if WITH_EDITORONLY_DATA
					CheckAttributesForRenderer(SubIdx);
#endif
				}
				else
				{
					EmitterRenderer[SubIdx] = nullptr;
				}
			}
		}
	}
}
