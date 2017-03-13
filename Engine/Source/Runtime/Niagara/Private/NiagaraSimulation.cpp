// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraSimulation.h"
#include "Materials/Material.h"
#include "VectorVM.h"
#include "NiagaraStats.h"
#include "NiagaraConstants.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraEffectInstance.h"
#include "NiagaraDataInterface.h"

DECLARE_DWORD_COUNTER_STAT(TEXT("Num Custom Events"), STAT_NiagaraNumCustomEvents, STATGROUP_Niagara);

DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_NiagaraTick, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"), STAT_NiagaraSimulate, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn"), STAT_NiagaraSpawn, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn"), STAT_NiagaraEvents, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Kill"), STAT_NiagaraKill, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Constant Setup"), STAT_NiagaraConstants, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Register Setup"), STAT_NiagaraSimRegisterSetup, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Event Handling"), STAT_NiagaraEventHandle, STATGROUP_Niagara);

static int32 GbDumpParticleData = 0;
static FAutoConsoleVariableRef CVarNiagaraDumpParticleData(
	TEXT("fx.DumpParticleData"),
	GbDumpParticleData,
	TEXT("If > 0 current frame particle data will be dumped after simulation. \n"),
	ECVF_Default
	);


//////////////////////////////////////////////////////////////////////////

FNiagaraSimulation::FNiagaraSimulation(FNiagaraEffectInstance* InParentEffectInstance)
: CPUTimeMS(0.0f)
, SpawnRemainder(0.0f)
, CachedBounds(ForceInit)
, EffectRenderer(nullptr)
, ParentEffectInstance(InParentEffectInstance)
, EmitterAge(SYS_PARAM_EMITTER_AGE)
, DeltaTime(SYS_PARAM_DELTA_TIME)
, ExecCount(SYS_PARAM_EXEC_COUNT)
, SpawnRateParam(SYS_PARAM_SPAWNRATE)
, SpawnRateInvParam(SYS_PARAM_SPAWNRATE_INVERSE)
, SpawnRemainderParam(SYS_PARAM_SPAWNRATE_REMAINDER)
{
	bDumpAfterEvent = false;
}

void FNiagaraSimulation::Init(const FNiagaraEmitterHandle& InEmitterHandle, FName InEffectInstanceName)
{
	EmitterHandle = &InEmitterHandle;
	OwnerEffectInstanceName = InEffectInstanceName;
	Data = FNiagaraDataSet(FNiagaraDataSetID(EmitterHandle->GetIdName(), ENiagaraDataSetType::ParticleData));

	UNiagaraEmitterProperties* EmitterProps = EmitterHandle->GetInstance();
}

void FNiagaraSimulation::ResetSimulation()
{
	bIsEnabled = EmitterHandle->GetIsEnabled();

	Age = 0;
	Loops = 0;

	Data.Reset();
	DataSetMap.Empty();
	BurstInstances.Empty();

	const UNiagaraEmitterProperties* PinnedProps = EmitterHandle->GetInstance();
	ExternalSpawnConstants.Set(PinnedProps->SpawnScriptProps.Script->Parameters);
	ExternalUpdateConstants.Set(PinnedProps->UpdateScriptProps.Script->Parameters);

	if (!bIsEnabled)
	{
		return;
	}

	if (!PinnedProps)
	{
		UE_LOG(LogNiagara, Error, TEXT("Unknown Error creating Niagara Simulation. Properties were null."));
		bIsEnabled = false;
		return;
	}

	//Check for various failure conditions and bail.
	if (!PinnedProps || !PinnedProps->UpdateScriptProps.Script || !PinnedProps->SpawnScriptProps.Script)
	{
		//TODO - Arbitrary named scripts. Would need some base functionality for Spawn/Udpate to be called that can be overriden in BPs for emitters with custom scripts.
		UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's doesn't have both an update and spawn script."));
		bIsEnabled = false;
		return;
	}
	if (PinnedProps->UpdateScriptProps.Script->ByteCode.Num() == 0 || PinnedProps->SpawnScriptProps.Script->ByteCode.Num() == 0)
	{
		UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's spawn or update script was not compiled correctly."));
		bIsEnabled = false;
		return;
	}

	if (PinnedProps->SpawnScriptProps.Script->DataUsage.bReadsAttriubteData)
	{
		UE_LOG(LogNiagara, Error, TEXT("%s reads attribute data and so cannot be used as a spawn script. The data being read would be invalid."), *PinnedProps->SpawnScriptProps.Script->GetName());
		bIsEnabled = false;
		return;
	}
	if (PinnedProps->UpdateScriptProps.Script->Attributes.Num() == 0 || PinnedProps->SpawnScriptProps.Script->Attributes.Num() == 0)
	{
		UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's spawn or update script doesn't have any attriubtes.."));
		bIsEnabled = false;
		return;
	}

	if (PinnedProps->EventHandlerScriptProps.Script)
	{
		if (PinnedProps->EventHandlerScriptProps.Script->ByteCode.Num() == 0)
		{
			UE_LOG(LogNiagara, Error, TEXT("%s has an event handler script hat didn't compile correctly."), *GetEmitterHandle().GetName().ToString());
			bIsEnabled = false;
			return;
		}
	}


	//Add the particle data to the data set map.
	//Currently just used for the tick loop but will also allow access directly to the particle data from other emitters.
	DataSetMap.Add(Data.GetID(), &Data);
	//Warn the user if there are any attributes used in the update script that are not initialized in the spawn script.
	//TODO: We need some window in the effect editor and possibly the graph editor for warnings and errors.
	for (FNiagaraVariable& Attr : PinnedProps->UpdateScriptProps.Script->Attributes)
	{
		int32 FoundIdx;
		if (!PinnedProps->SpawnScriptProps.Script->Attributes.Find(Attr, FoundIdx))
		{
			UE_LOG(LogNiagara, Warning, TEXT("Attribute %s is used in the Update script for %s but it is not initialised in the Spawn script!"), *Attr.GetName().ToString(), *EmitterHandle->GetName().ToString());
		}
		if (PinnedProps->EventHandlerScriptProps.Script && !PinnedProps->EventHandlerScriptProps.Script->Attributes.Find(Attr, FoundIdx))
		{
			UE_LOG(LogNiagara, Warning, TEXT("Attribute %s is used in the event handler script for %s but it is not initialised in the Spawn script!"), *Attr.GetName().ToString(), *EmitterHandle->GetName().ToString());
		}
	}
	Data.AddVariables(PinnedProps->UpdateScriptProps.Script->Attributes);
	Data.AddVariables(PinnedProps->SpawnScriptProps.Script->Attributes);
	Data.Finalize();

	CollisionBatch.Init(ParentEffectInstance->GetIDName(), EmitterHandle->GetIdName());

	for (const FNiagaraEmitterBurst& Burst : PinnedProps->Bursts)
	{
		FNiagaraBurstInstance BurstInstance;
		BurstInstance.Time = FMath::Max(0.0f, Burst.Time + ((FMath::FRand() - .5f) * Burst.TimeRange));
		BurstInstance.NumberToSpawn = FMath::RandRange((int32)Burst.SpawnMinimum, (int32)Burst.SpawnMaximum);
		BurstInstances.Add(BurstInstance);
	}
	BurstInstances.Sort([](const FNiagaraBurstInstance& BurstInstanceA, const FNiagaraBurstInstance& BurstInstanceB) { return BurstInstanceA.Time < BurstInstanceB.Time; });
	CurrentBurstInstanceIndex = 0;

	UpdateScriptEventDataSets.Empty();
	for (const FNiagaraEventGeneratorProperties &GeneratorProps : PinnedProps->UpdateScriptProps.EventGenerators)
	{
		FNiagaraDataSet *Set = FNiagaraEventDataSetMgr::CreateEventDataSet(ParentEffectInstance->GetIDName(), EmitterHandle->GetIdName(), GeneratorProps.SetProps.ID.Name);
		Set->Reset();
		Set->AddVariables(GeneratorProps.SetProps.Variables);
		Set->Finalize();
		UpdateScriptEventDataSets.Add(Set);
	}

}

void FNiagaraSimulation::PostResetSimulation()
{
	if (bIsEnabled)
	{
		check(ParentEffectInstance);
		const UNiagaraEmitterProperties* Props = EmitterHandle->GetInstance();

		//Go through all our receivers and grab their generator sets so that the source emitters can do any init work they need to do.
		for (const FNiagaraEventReceiverProperties& Receiver : Props->SpawnScriptProps.EventReceivers)
		{
			//FNiagaraDataSet* ReceiverSet = ParentEffectInstance->GetDataSet(FNiagaraDataSetID(Receiver.SourceEventGenerator, ENiagaraDataSetType::Event), Receiver.SourceEmitter);
			const FNiagaraDataSet* ReceiverSet = FNiagaraEventDataSetMgr::GetEventDataSet(ParentEffectInstance->GetIDName(), Receiver.SourceEmitter, Receiver.SourceEventGenerator);

		}

		for (const FNiagaraEventReceiverProperties& Receiver : Props->UpdateScriptProps.EventReceivers)
		{
			//FNiagaraDataSet* ReceiverSet = ParentEffectInstance->GetDataSet(FNiagaraDataSetID(Receiver.SourceEventGenerator, ENiagaraDataSetType::Event), Receiver.SourceEmitter);
			const FNiagaraDataSet* ReceiverSet = FNiagaraEventDataSetMgr::GetEventDataSet(ParentEffectInstance->GetIDName(), Receiver.SourceEmitter, Receiver.SourceEventGenerator);
		}

		// add the collision event set
		// TODO: add spawn and death event data sets (if enabled)
		if (Props->CollisionMode != ENiagaraCollisionMode::None)
		{
			CollisionBatch.Init(ParentEffectInstance->GetIDName(), EmitterHandle->GetIdName());	// creates and sets up the data set for the events
		}
	}
}

FNiagaraDataSet* FNiagaraSimulation::GetDataSet(FNiagaraDataSetID SetID)
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

float FNiagaraSimulation::GetTotalCPUTime()
{
	float Total = CPUTimeMS;
	if (EffectRenderer)
	{
		Total += EffectRenderer->GetCPUTimeMS();
	}

	return Total;
}

int FNiagaraSimulation::GetTotalBytesUsed()
{
	int32 BytesUsed = Data.GetSizeBytes();
	/*
	for (FNiagaraDataSet& Set : DataSets)
	{
		BytesUsed += Set.GetSizeBytes();
	}
	*/
	return BytesUsed;
}


/** Look for dead particles and move from the end of the list to the dead location, compacting in the process
  */
void FNiagaraSimulation::KillParticles()
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraKill);
	int32 OrigNumParticles = Data.GetNumInstances();
	int32 CurNumParticles = OrigNumParticles;

	//TODO: REPLACE AGE/DEATH METHOD WITH A CONDITIONAL WRITE FROM THE SIMULATE.
	FNiagaraVariable AgeVar = FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Age"));
	FNiagaraDataSetIterator<float> AgeItr(Data, AgeVar);
	while (AgeItr.IsValid())
	{
		float AgeVal;
		AgeItr.Get(AgeVal);
		if (AgeVal >= 1.0f)
		{
			//UE_LOG(LogNiagara, Log, TEXT("Killed %d"),AgeItr.GetCurrIndex());
			Data.KillInstance(AgeItr.GetCurrIndex());
		}
		AgeItr.Advance();
	}

	// check if the emitter has officially died
	if (GetTickState() == NTS_Dieing && CurNumParticles == 0)
	{
		SetTickState(NTS_Dead);
	}
}

/** 
  * PreTick - handles killing dead particles, emitter death, and buffer swaps
  */
void FNiagaraSimulation::PreTick()
{
	const UNiagaraEmitterProperties* PinnedProps = EmitterHandle->GetInstance();

	if (!PinnedProps || !bIsEnabled
		|| TickState == NTS_Suspended || TickState == NTS_Dead
		|| Data.GetNumInstances() == 0)
	{
		return;
	}

	check(Data.GetNumVariables() > 0);
	check(PinnedProps->SpawnScriptProps.Script);
	check(PinnedProps->UpdateScriptProps.Script);

	KillParticles();

	// generate events from collisions
	if (PinnedProps->CollisionMode == ENiagaraCollisionMode::SceneGeometry)
	{
		CollisionBatch.GenerateEventsFromResults(this);
	}


	//Swap all data set buffers before doing the main tick on any simulation.
	for (TPair<FNiagaraDataSetID, FNiagaraDataSet*> SetPair : DataSetMap)
	{
		SetPair.Value->Tick();
	}
	CollisionBatch.Tick();

	for (FNiagaraDataSet* Set : UpdateScriptEventDataSets)
	{
		Set->Tick();
	}
}


void FNiagaraSimulation::Tick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraTick);
	SimpleTimer TickTime;

	const UNiagaraEmitterProperties* PinnedProps = EmitterHandle->GetInstance();
	const FNiagaraEventScriptProperties &EventHandlerProps = PinnedProps->EventHandlerScriptProps;
	if (!PinnedProps || !bIsEnabled || TickState == NTS_Suspended || TickState == NTS_Dead)
	{
		return;
	}

	Age += DeltaSeconds;

	check(Data.GetNumVariables() > 0);
	check(PinnedProps->SpawnScriptProps.Script);
	check(PinnedProps->UpdateScriptProps.Script);
	
	//TickEvents(DeltaSeconds);

	FNiagaraParameters SpawnConstants;
	FNiagaraParameters UpdateConstants;
	FNiagaraParameters EventHandlerConstants;

	// add system constants
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraConstants);

		// copy over script property constants (these are the parameters, serialized and outside accessible)
		// to our internal constant set
		SpawnConstants.Set(ExternalSpawnConstants);
		UpdateConstants.Set(ExternalUpdateConstants);
		EventHandlerConstants.Set(ExternalEventHandlerConstants);

		//Pull effect params.
		FNiagaraParameters& EffectParams = GetParentEffectInstance()->GetEffectParameters();
		SpawnConstants.Merge(EffectParams);
		UpdateConstants.Merge(EffectParams);

		//Set emitter params.
		//TODO: THIS IS ALL REALLY PANTS.
		//Need to rework so the array to be used for execution is directly updatable here with the values themselves rather than these intermediate FNiagaraVariables.
		DeltaTime.SetValue(DeltaSeconds);
		EmitterAge.SetValue(Age);
		float SpawnRate = EmitterHandle->GetInstance()->SpawnRate;
		SpawnRateParam.SetValue(SpawnRate);
		SpawnRateInvParam.SetValue(1.0f / SpawnRate);
		SpawnRemainderParam.SetValue(SpawnRemainder);

		SpawnConstants.SetOrAdd(DeltaTime);
		SpawnConstants.SetOrAdd(EmitterAge);
		SpawnConstants.SetOrAdd(SpawnRateParam);
		SpawnConstants.SetOrAdd(SpawnRateInvParam);
		SpawnConstants.SetOrAdd(SpawnRemainderParam);

		UpdateConstants.SetOrAdd(DeltaTime);
		UpdateConstants.SetOrAdd(EmitterAge);

		EventHandlerConstants.SetOrAdd(DeltaTime);
		EventHandlerConstants.SetOrAdd(EmitterAge);
	}
	
	// Calculate number of new particles from regular spawning 
	int32 OrigNumParticles = Data.GetPrevNumInstances();
	int32 NumToSpawn = CalcNumToSpawn(DeltaSeconds);
	int32 NewParticles = OrigNumParticles + NumToSpawn;

	// Calculate number of new particles from all event related spawns
	TArray<int32> EventSpawnCounts;
	uint32 EventSpawnTotal = 0;
	FNiagaraDataSet *EventSet = nullptr;
	FGuid SourceEmitterGuid = EventHandlerProps.SourceEmitterID;
	FName SourceEmitterName = SourceEmitterGuid.IsValid() ? *SourceEmitterGuid.ToString() : EmitterHandle->GetIdName();
	EventSet = FNiagaraEventDataSetMgr::GetEventDataSet(ParentEffectInstance->GetIDName(), SourceEmitterName, EventHandlerProps.SourceEventName);
	bool bPerformEventSpawning = (TickState == NTS_Running && EventHandlerProps.Script && EventHandlerProps.ExecutionMode == EScriptExecutionMode::SpawnedParticles);
	if (bPerformEventSpawning)
	{
		EventSpawnTotal = CalculateEventSpawnCount(EventHandlerProps, EventSpawnCounts, EventSet);
	}

	//Allocate space for prev frames particles and any new one's we're going to spawn.
	Data.Allocate(NewParticles + EventSpawnTotal);

	// Simulate existing particles forward by DeltaSeconds.
	if ((TickState==NTS_Running || TickState==NTS_Dieing) && OrigNumParticles > 0)
	{
		/*
		if (bDumpAfterEvent)
		{
			Data.Dump(false);
			bDumpAfterEvent = false;
		}
		*/
		ExecCount.SetValue(OrigNumParticles);
		UpdateConstants.SetOrAdd(ExecCount);

		Data.SetNumInstances(OrigNumParticles);
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimulate);
		RunVMScript(PinnedProps->UpdateScriptProps, EUnusedAttributeBehaviour::PassThrough, UpdateConstants);
	}


	if (GbDumpParticleData)
	{
		UE_LOG(LogNiagara, Log, TEXT("===POST SPAWN==="));
		Data.Dump(true);
	}


	//Init new particles with the spawn script.
	if (TickState==NTS_Running && NewParticles > OrigNumParticles)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSpawn);

		ExecCount.SetValue(NewParticles);
		SpawnConstants.SetOrAdd(ExecCount);

		Data.SetNumInstances(NewParticles);
		RunVMScript(PinnedProps->SpawnScriptProps, EUnusedAttributeBehaviour::Zero, OrigNumParticles, NumToSpawn, SpawnConstants);
	}


	// handle event based spawning
	if (bPerformEventSpawning && EventSet && EventSpawnCounts.Num())
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraEventHandle);
		Data.SetNumInstances(NewParticles + EventSpawnTotal);
		for (int32 i = 0; i < EventSpawnCounts.Num(); i++)
		{
			int32 EventNumToSpawn = EventSpawnCounts[i];

			ExecCount.SetValue(EventNumToSpawn);
			EventHandlerConstants.SetOrAdd(ExecCount);

			RunEventHandlerScript(PinnedProps->EventHandlerScriptProps, EventHandlerConstants, EventSet, NewParticles, EventNumToSpawn, i);
			NewParticles += EventNumToSpawn;
		}
	}


	// handle all-particle events
	if (EventHandlerProps.Script && EventHandlerProps.ExecutionMode == EScriptExecutionMode::EveryParticle && EventSet)
	{
		if (EventSet->GetPrevNumInstances())
		{
			SCOPE_CYCLE_COUNTER(STAT_NiagaraEventHandle);

			for (uint32 i = 0; i < EventSet->GetPrevNumInstances(); i++)
			{
				// If we have events, Swap buffers, to make sure we don't overwrite previous script results
				// and copy prev to cur, because the event script isn't likely to write all attributes
				// TODO: copy only event script's unused attributes
				Data.Tick();
				Data.CopyPrevToCur();

				ExecCount.SetValue(Data.GetPrevNumInstances());
				EventHandlerConstants.SetOrAdd(ExecCount);

				RunEventHandlerScript(PinnedProps->EventHandlerScriptProps, EventHandlerConstants, EventSet, 0, Data.GetPrevNumInstances(), i);
			}
		}
	}


	// handle single-particle events
	// TODO: we'll need a way to either skip execution of the VM if an index comes back as invalid, or we'll have to pre-process
	// event/particle arrays; this is currently a very naive (and comparatively slow) implementation, until full indexed reads work
	if (EventHandlerProps.Script && EventHandlerProps.ExecutionMode == EScriptExecutionMode::SingleParticle && EventSet)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraEventHandle);
		FNiagaraVariable IndexVar(FNiagaraTypeDefinition::GetIntDef(), "ParticleIndex");
		FNiagaraDataSetIterator<int32> IndexItr(*EventSet, IndexVar, 0, false);
		if (IndexItr.IsValid())
		{
			ExecCount.SetValue(1);//Eek a VM run per instance. Need to rework things so this can be one run for them all.
			EventHandlerConstants.SetOrAdd(ExecCount);

			for (uint32 i = 0; i < EventSet->GetPrevNumInstances(); i++)
			{
				int32 Index = *IndexItr;
				IndexItr.Advance();
				RunEventHandlerScript(PinnedProps->EventHandlerScriptProps, EventHandlerConstants, EventSet, Index, 1, i);
			}
		}
	}


	// kick off collision tests from this emitter
	if (PinnedProps->CollisionMode == ENiagaraCollisionMode::SceneGeometry)
	{
		CollisionBatch.KickoffNewBatch(this, DeltaSeconds);
	}


	if (GbDumpParticleData)
	{
		UE_LOG(LogNiagara, Log, TEXT("===POST UPDATE==="));
		Data.Dump(true);
	}

	CPUTimeMS = TickTime.GetElapsedMilliseconds();

	INC_DWORD_STAT_BY(STAT_NiagaraNumParticles, Data.GetNumInstances());
}


/** Calculate total number of spawned particles from events; these all come from event handler script with the SpawnedParticles execution mode
 *  We get the counts ahead of event processing time so we only have to allocate new particles once
 *  TODO: augment for multiple spawning event scripts
 */
uint32 FNiagaraSimulation::CalculateEventSpawnCount(const FNiagaraEventScriptProperties &EventHandlerProps, TArray<int32> &EventSpawnCounts, FNiagaraDataSet *EventSet)
{
	uint32 EventSpawnTotal = 0;
	int32 NumEventsToProcess = 0;

	if (EventSet)
	{
		NumEventsToProcess = FMath::Min<int32>(EventSet->GetPrevNumInstances(), EventHandlerProps.MaxEventsPerFrame);
		for (int32 i = 0; i < NumEventsToProcess; i++)
		{
			EventSpawnCounts.Add(EventHandlerProps.SpawnNumber);
			EventSpawnTotal += EventHandlerProps.SpawnNumber;
		}
	}

	return EventSpawnTotal;
}

void FNiagaraSimulation::TickEvents(float DeltaSeconds)
{
	//Handle Event Actions.
	auto CallEventActions = [&](const FNiagaraEventReceiverProperties& Receiver)
	{
		for (auto& Action : Receiver.EmitterActions)
		{
			if (Action)
			{
				Action->PerformAction(*this, Receiver);
			}
		}
	};

	const UNiagaraEmitterProperties* Props = EmitterHandle->GetInstance();
	for (const auto& Receiver : Props->SpawnScriptProps.EventReceivers) { CallEventActions(Receiver); }
	for (const auto& Receiver : Props->UpdateScriptProps.EventReceivers) { CallEventActions(Receiver); }
}

int32 FNiagaraSimulation::CalcNumToSpawn(float DeltaSeconds)
{
	if (TickState == NTS_Dead || TickState == NTS_Dieing || TickState == NTS_Suspended)
	{
		return 0;
	}

	float FloatNumToSpawn = SpawnRemainder + (DeltaSeconds * EmitterHandle->GetInstance()->SpawnRate);
	int32 NumToSpawn = FMath::FloorToInt(FloatNumToSpawn);
	SpawnRemainder = FloatNumToSpawn - NumToSpawn;

	// Handle bursts
	while (CurrentBurstInstanceIndex < BurstInstances.Num() && BurstInstances[CurrentBurstInstanceIndex].Time < Age)
	{
		NumToSpawn += BurstInstances[CurrentBurstInstanceIndex++].NumberToSpawn;
	}

	return NumToSpawn;
}

void FNiagaraSimulation::RunVMScript(const FNiagaraEmitterScriptProperties& ScriptProps, EUnusedAttributeBehaviour UnusedAttribBehaviour, FNiagaraParameters &Parameters)
{
	RunVMScript(ScriptProps, UnusedAttribBehaviour, 0, Data.GetNumInstances(), Parameters);
}

void FNiagaraSimulation::RunVMScript(const FNiagaraEmitterScriptProperties& ScriptProps, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle, FNiagaraParameters &Parameters)
{
	RunVMScript(ScriptProps, UnusedAttribBehaviour, StartParticle, Data.GetNumInstances() - StartParticle, Parameters);
}

void FNiagaraSimulation::RunVMScript(const FNiagaraEmitterScriptProperties& ScriptProps, EUnusedAttributeBehaviour UnusedAttribBehaviour, uint32 StartParticle, uint32 NumParticles, FNiagaraParameters &Parameters)
{
	uint32 CurrNumParticles = Data.GetNumInstances();
	if (CurrNumParticles == 0 || NumParticles == 0)
	{
		return;
	}
	
	UNiagaraScript* Script = ScriptProps.Script;

	check(Script);
	check(Script->ByteCode.Num() > 0);
	check(Script->Attributes.Num() > 0);
	check(Data.GetNumVariables() > 0);

	check(StartParticle + NumParticles <= CurrNumParticles);

	uint8* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
	uint8* OutputRegisters[VectorVM::MaxOutputRegisters] = { 0 };
	uint8 InputRegisterSizes[VectorVM::MaxInputRegisters] = { 0 };
	uint8 OutputRegisterSizes[VectorVM::MaxOutputRegisters] = { 0 };
	
	//The current script run will be using NumScriptAttrs. We must pick out the Attributes form the simulation data in the order that they appear in the script.
	//The remaining attributes being handled according to UnusedAttribBehaviour
	const int32 NumScriptAttrs = Script->Attributes.Num();

	check(NumScriptAttrs < VectorVM::MaxInputRegisters);
	check(NumScriptAttrs < VectorVM::MaxOutputRegisters);

	int32 NumInputRegisters = 0;
	int32 NumOutputRegitsers = 0;

	TArray<FDataSetMeta> DataSetMetaTable;

	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimRegisterSetup);

		FDataSetMeta AttribSetMeta(this->Data.GetSizeBytes(), &InputRegisters[NumInputRegisters], NumInputRegisters);
		DataSetMetaTable.Add(AttribSetMeta);
		for (int32 AttrIdx = 0; AttrIdx < Script->Attributes.Num(); ++AttrIdx)
		{
			Data.AppendToRegisterTable(Script->Attributes[AttrIdx], InputRegisters, InputRegisterSizes, NumInputRegisters, OutputRegisters, OutputRegisterSizes, NumOutputRegitsers, StartParticle);
		}
	}

	// Fill constant table with required emitter constants and internal script constants.
	TArray<uint8, TAlignedHeapAllocator<VECTOR_WIDTH_BYTES>> ConstantTable;
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraConstants);
		int32 ParamSize = Script->Parameters.GetTableSize();
		int32 InternalConstSize = Script->InternalParameters.GetTableSize();

		ConstantTable.AddUninitialized(ParamSize + InternalConstSize);
		Script->Parameters.AppendToConstantsTable(ConstantTable.GetData(), Parameters);
		Script->InternalParameters.AppendToConstantsTable(ConstantTable.GetData() + ParamSize);
	}

	
	// add secondary data sets to the register table and build the data set meta table (contains set index counters and register table offsets)
	for (FNiagaraDataSet *EventSet : UpdateScriptEventDataSets)
	{
		FDataSetMeta EventSetMeta(EventSet->GetSizeBytes(), &InputRegisters[NumInputRegisters], NumInputRegisters);
		DataSetMetaTable.Add(EventSetMeta);
		EventSet->Allocate(Data.GetNumInstances());
		EventSet->SetNumInstances(Data.GetNumInstances());
		for (const FNiagaraVariable &Var : EventSet->GetVariables())
		{
			EventSet->AppendToRegisterTable(Var, InputRegisters, InputRegisterSizes, NumInputRegisters, OutputRegisters, OutputRegisterSizes, NumOutputRegitsers, StartParticle);
		}
	}

	//TODO: Change this to have these bind into a pre-made table on init. Virtual calls every frame, eugh. Same with the uniforms above. We can generate most of this on init.
	//Can also likely do something lighter than the current register allocations too.
	TArray<FVMExternalFunction> FunctionTable;
	for (FNiagaraScriptDataInterfaceInfo& Info : Script->DataInterfaceInfo)
	{
		for (FNiagaraFunctionSignature& Sig : Info.ExternalFunctions)
		{
			FunctionTable.Add(Info.DataInterface->GetVMExternalFunction(Sig));
		}
	}

	VectorVM::Exec(
		Script->ByteCode.GetData(),
		InputRegisters,
		InputRegisterSizes,
		NumInputRegisters,
		OutputRegisters,
		OutputRegisterSizes,
		NumOutputRegitsers,
		ConstantTable.GetData(),
		DataSetMetaTable,
		FunctionTable.GetData(),
		NumParticles
		);	
}






void FNiagaraSimulation::RunEventHandlerScript(const FNiagaraEmitterScriptProperties& ScriptProps, FNiagaraParameters &Parameters, FNiagaraDataSet *InEventDataSet, uint32 StartParticle, uint32 NumParticles, uint32 EventIndex)
{
	uint32 CurrNumParticles = Data.GetNumInstances();
	uint32 NumEvents = InEventDataSet->GetPrevNumInstances();
	check(NumEvents > 0);


	UNiagaraScript* Script = ScriptProps.Script;

	check(Script);
	check(Script->ByteCode.Num() > 0);
	check(Script->Attributes.Num() > 0);
	check(Data.GetNumVariables() > 0);

	uint8* InputRegisters[VectorVM::MaxInputRegisters] = { 0 };
	uint8* OutputRegisters[VectorVM::MaxOutputRegisters] = { 0 };
	uint8 InputRegisterSizes[VectorVM::MaxInputRegisters] = { 0 };
	uint8 OutputRegisterSizes[VectorVM::MaxOutputRegisters] = { 0 };

	int32 NumInputRegisters = 0;
	int32 NumOutputRegisters = 0;

	// Fill constant table with required emitter constants and internal script constants.
	TArray<uint8, TAlignedHeapAllocator<VECTOR_WIDTH_BYTES>> ConstantTable;
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraConstants);
		int32 ParamSize = Script->Parameters.GetTableSize();
		int32 InternalConstSize = Script->InternalParameters.GetTableSize();

		ConstantTable.AddUninitialized(ParamSize + InternalConstSize);
		Script->Parameters.AppendToConstantsTable(ConstantTable.GetData(), Parameters);
		Script->InternalParameters.AppendToConstantsTable(ConstantTable.GetData() + ParamSize);
	}


	// add data sets to the register table and build the data set meta table (contains set index counters and register table offsets)
	TArray<FDataSetMeta> DataSetMetaTable;
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimRegisterSetup);

		// the particle data set first
		FDataSetMeta MainSetMeta(Data.GetSizeBytes(), &InputRegisters[0], 0);
		DataSetMetaTable.Add(MainSetMeta);
		for (int32 AttrIdx = 0; AttrIdx < Script->Attributes.Num(); ++AttrIdx)
		{
			Data.AppendToRegisterTable(Script->Attributes[AttrIdx], InputRegisters, InputRegisterSizes, NumInputRegisters, OutputRegisters, OutputRegisterSizes, NumOutputRegisters, StartParticle);
		}

		// now the event payload; we only add the one instance we're interested in
		FDataSetMeta EventSetMeta(InEventDataSet->GetSizeBytes(), &InputRegisters[NumInputRegisters], NumInputRegisters);
		DataSetMetaTable.Add(EventSetMeta);
		for (const FNiagaraVariable &Var : InEventDataSet->GetVariables())
		{

			InEventDataSet->AppendToRegisterTable(Var, InputRegisters, InputRegisterSizes, NumInputRegisters, OutputRegisters, OutputRegisterSizes, NumOutputRegisters, EventIndex);
		}
		//Currently not handling unused attributes at all!
	}


	VectorVM::Exec(
		Script->ByteCode.GetData(),
		InputRegisters,
		InputRegisterSizes,
		NumInputRegisters,
		OutputRegisters,
		OutputRegisterSizes,
		NumOutputRegisters,
		ConstantTable.GetData(),
		DataSetMetaTable,
		NULL,
		NumParticles
		);
}


bool FNiagaraSimulation::CheckAttributesForRenderer()
{
	bool bOk = true;
	if (EffectRenderer)
	{
		const TArray<FNiagaraVariable>& RequiredAttrs = EffectRenderer->GetRequiredAttributes();

		for (auto& Attr : RequiredAttrs)
		{
			if (!Data.HasVariable(Attr))
			{
				bOk = false;
				UE_LOG(LogNiagara, Error, TEXT("Cannot render %s because it does not define attribute %s %s."), *EmitterHandle->GetName().ToString(), *Attr.GetType().GetNameText().ToString() , *Attr.GetName().ToString());
			}
		}
	}
	return bOk;
}

/** Replace the current effect renderer with a new one of Type.
Don't forget to call RenderModuleUpdate on the SceneProxy after calling this! 
 */
void FNiagaraSimulation::UpdateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	const UNiagaraEmitterProperties* EmitterProperties = EmitterHandle->GetInstance();
	if (bIsEnabled && EmitterProperties != nullptr && EmitterProperties->RendererProperties != nullptr)
	{
		UMaterial *Material = nullptr;

		if (EmitterProperties->Material)
		{
			Material = EmitterProperties->Material;
		}
		else
		{
			Material = UMaterial::GetDefaultMaterial(MD_Surface);
		}

		if (EffectRenderer)
		{
			delete EffectRenderer;
		}

		EffectRenderer = EmitterProperties->RendererProperties->CreateEffectRenderer(FeatureLevel);
		EffectRenderer->SetMaterial(Material, FeatureLevel);
		EffectRenderer->SetLocalSpace(EmitterProperties->bLocalSpace);

		CheckAttributesForRenderer();
	}
}
