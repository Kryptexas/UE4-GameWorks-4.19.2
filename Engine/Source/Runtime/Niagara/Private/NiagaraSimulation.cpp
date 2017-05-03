// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraSimulation.h"
#include "Materials/Material.h"
#include "VectorVM.h"
#include "NiagaraStats.h"
#include "NiagaraConstants.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraEffectInstance.h"
#include "NiagaraDataInterface.h"

#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("Niagara Detailed"), STATGROUP_NiagaraDetailed, STATCAT_Advanced);

DECLARE_DWORD_COUNTER_STAT(TEXT("Num Custom Events"), STAT_NiagaraNumCustomEvents, STATGROUP_Niagara);

DECLARE_CYCLE_STAT(TEXT("Tick"), STAT_NiagaraTick, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Simulate"), STAT_NiagaraSimulate, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn"), STAT_NiagaraSpawn, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Spawn"), STAT_NiagaraEvents, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Kill"), STAT_NiagaraKill, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Constant Setup"), STAT_NiagaraConstants, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Register Setup"), STAT_NiagaraSimRegisterSetup, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("Event Handling"), STAT_NiagaraEventHandle, STATGROUP_Niagara);


#if STATS
#define SPAWN_STAT_SCOPES_PARAM ,SpawnStatScopes
#define UPDATE_STAT_SCOPES_PARAM ,UpdateStatScopes
#define EVENT_STAT_SCOPES_PARAM ,EventHandlerStatScopes
#else
#define SPAWN_STAT_SCOPES_PARAM 
#define UPDATE_STAT_SCOPES_PARAM 
#define EVENT_STAT_SCOPES_PARAM 
#endif

static int32 GbDumpParticleData = 0;
static FAutoConsoleVariableRef CVarNiagaraDumpParticleData(
	TEXT("fx.DumpParticleData"),
	GbDumpParticleData,
	TEXT("If > 0 current frame particle data will be dumped after simulation. \n"),
	ECVF_Default
	);

//////////////////////////////////////////////////////////////////////////

//Todo: this is slightly neater than the previous setup and execution but there's plenty that could be better and faster!.
struct FNiagaraScriptExecutionContext
{
	TArray<FVMExternalFunction>& FunctionTable;
	uint32 StartParticle;
	uint32 NumParticles;

	UNiagaraScript* Script;
	int32 NumScriptAttrs;

	int32 NumInputRegisters;
	int32 NumOutputRegisters;
	uint8* InputRegisters[VectorVM::MaxInputRegisters];
	uint8* OutputRegisters[VectorVM::MaxOutputRegisters];
	uint8 InputRegisterSizes[VectorVM::MaxInputRegisters];
	uint8 OutputRegisterSizes[VectorVM::MaxOutputRegisters];

	TArray<FDataSetMeta> DataSetMetaTable;
	TArray<FNiagaraDataSet*, TInlineAllocator<16>> DataSets;

	TArray<uint8, TAlignedHeapAllocator<VECTOR_WIDTH_BYTES>> ConstantTable;

#if STATS
	TArray<TStatId>& StatScopes;
#endif

	FNiagaraScriptExecutionContext(UNiagaraScript* InScript, TArray<FVMExternalFunction>& InFunctionTable, uint32 InStartParticle, uint32 InNumParticles
#if STATS
		, TArray<TStatId>& InStatScopes
#endif
	)
		: FunctionTable(InFunctionTable)
		, StartParticle(InStartParticle)
		, NumParticles(InNumParticles)
		, Script(InScript)
		, NumScriptAttrs(Script ? Script->Attributes.Num() : 0)
		, NumInputRegisters(0)
		, NumOutputRegisters(0)
#if STATS
		, StatScopes(InStatScopes)
#endif
	{
		check(Script);
		check(Script->ByteCode.Num() > 0);
		check(Script->Attributes.Num() > 0);

		check(NumScriptAttrs < VectorVM::MaxInputRegisters);
		check(NumScriptAttrs < VectorVM::MaxOutputRegisters);
	}

	void AddDataSet(FNiagaraDataSet& InDataSet)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSimRegisterSetup);
		check(InDataSet.GetNumVariables() > 0);
		check(StartParticle + NumParticles <= InDataSet.GetNumInstances());
		FDataSetMeta AttribSetMeta(InDataSet.GetSizeBytes(), &InputRegisters[NumInputRegisters], NumInputRegisters);
		DataSetMetaTable.Add(AttribSetMeta);
		DataSets.Add(&InDataSet);
		for (int32 AttrIdx = 0; AttrIdx < Script->Attributes.Num(); ++AttrIdx)
		{
			InDataSet.AppendToRegisterTable(Script->Attributes[AttrIdx], InputRegisters, InputRegisterSizes, NumInputRegisters, OutputRegisters, OutputRegisterSizes, NumOutputRegisters, StartParticle);
		}
	}

	void AddEventDataSet(FNiagaraDataSet& EventSet, int32 EventIndex)
	{
		check(EventSet.GetPrevNumInstances() > 0);
		check(EventSet.GetNumVariables() > 0);
		FDataSetMeta EventSetMeta(EventSet.GetSizeBytes(), &InputRegisters[NumInputRegisters], NumInputRegisters);
		DataSetMetaTable.Add(EventSetMeta);	
		DataSets.Add(&EventSet);
		for (const FNiagaraVariable &Var : EventSet.GetVariables())
		{
			EventSet.AppendToRegisterTable(Var, InputRegisters, InputRegisterSizes, NumInputRegisters, OutputRegisters, OutputRegisterSizes, NumOutputRegisters, EventIndex, true);
		}
	}

	void AddEventDataSets(TArray<FNiagaraDataSet*>& EventDataSets)
	{
		for (FNiagaraDataSet *EventSet : EventDataSets)
		{
			FDataSetMeta EventSetMeta(EventSet->GetSizeBytes(), &InputRegisters[NumInputRegisters], NumInputRegisters);
			DataSetMetaTable.Add(EventSetMeta);
			EventSet->Allocate(NumParticles);
			EventSet->SetNumInstances(NumParticles);
			DataSets.Add(EventSet);
			for (const FNiagaraVariable &Var : EventSet->GetVariables())
			{
				EventSet->AppendToRegisterTable(Var, InputRegisters, InputRegisterSizes, NumInputRegisters, OutputRegisters, OutputRegisterSizes, NumOutputRegisters, StartParticle);
			}
		}
	}

	void InitParameters(FNiagaraParameters& Parameters)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraConstants);
		int32 ParamSize = Script->Parameters.GetTableSize();
		int32 InternalConstSize = Script->InternalParameters.GetTableSize();

		ConstantTable.SetNumUninitialized(ParamSize + InternalConstSize);
		Script->Parameters.AppendToConstantsTable(ConstantTable.GetData(), Parameters);
		Script->InternalParameters.AppendToConstantsTable(ConstantTable.GetData() + ParamSize);
	}

	/** Inits parameters for scripts that require the previous params for interpolation. */
	void InitParameters(FNiagaraParameters& Parameters, FNiagaraParameters& PrevParameters)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraConstants);
		bool bInterpolated = Script->IsInterpolatedSpawnScript();
		int32 ParamSize = Script->Parameters.GetTableSize();
		int32 InternalConstSize = Script->InternalParameters.GetTableSize();

		if (bInterpolated)
		{
			ConstantTable.SetNumUninitialized(ParamSize + ParamSize + InternalConstSize);
			Script->Parameters.AppendToConstantsTable(ConstantTable.GetData(), PrevParameters);
			Script->Parameters.AppendToConstantsTable(ConstantTable.GetData() + ParamSize, Parameters);
			Script->InternalParameters.AppendToConstantsTable(ConstantTable.GetData() + ParamSize + ParamSize);
		}
		else
		{
			ConstantTable.SetNumUninitialized(ParamSize + InternalConstSize);
			Script->Parameters.AppendToConstantsTable(ConstantTable.GetData(), Parameters);
			Script->InternalParameters.AppendToConstantsTable(ConstantTable.GetData() + ParamSize);
		}
	}

	void ResolveDataSetWrites()
	{
		// Tell the datasets we wrote how many instances were actually written.
		// TODO: Currently not doing the primary set but we should do this and implement conditional writes tehre too.
		for (int Idx = 1; Idx < DataSets.Num(); Idx++)
		{
			DataSets[Idx]->SetNumInstances(DataSetMetaTable[Idx].DataSetAccessIndex);
		}
	}

	void Execute()
	{
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
			FunctionTable.GetData(),
			NumParticles
#if STATS
			, StatScopes
#endif
		);
	}
};

//////////////////////////////////////////////////////////////////////////

FNiagaraSimulation::FNiagaraSimulation(FNiagaraEffectInstance* InParentEffectInstance)
: bDataInterfacesEnabled(false)
, bHasValidPropertiesAndScripts(false)
, CPUTimeMS(0.0f)
, CachedBounds(ForceInit)
, SpawnRemainder(0.0f)
, EffectRenderer(nullptr)
, ParentEffectInstance(InParentEffectInstance)
, EmitterAge(SYS_PARAM_EMITTER_AGE)
, DeltaTime(SYS_PARAM_DELTA_TIME)
, InvDeltaTime(SYS_PARAM_INV_DELTA_TIME)
, ExecCount(SYS_PARAM_EXEC_COUNT)
, SpawnRateParam(SYS_PARAM_SPAWNRATE)
, SpawnIntervalParam(SYS_PARAM_SPAWN_INTERVAL)
, InterpSpawnStartDtParam(SYS_PARAM_INTERP_SPAWN_START_DT)

{
	bDumpAfterEvent = false;
}

FNiagaraSimulation::~FNiagaraSimulation()
{
	//UE_LOG(LogNiagara, Warning, TEXT("~Simulator %p"), this);
	ClearRenderer();
	CachedBounds.Init();
}

FString FNiagaraSimulation::GetEmitterSpawnRateInternalVarName()
{
	return "__SpawnRate"; 
}

FString FNiagaraSimulation::GetEmitterEnabledInternalVarName() 
{
	return "__Enabled"; 
}

void FNiagaraSimulation::ClearRenderer()
{
	if (EffectRenderer)
	{
		//UE_LOG(LogNiagara, Warning, TEXT("ClearRenderer %p"), EffectRenderer);
		// This queues up the renderer for deletion on the render thread..
		EffectRenderer->Release();
		EffectRenderer = nullptr;
	}
}

void FNiagaraSimulation::Init(const FNiagaraEmitterHandle& InEmitterHandle, FName InEffectInstanceName)
{
	EmitterHandle = &InEmitterHandle;
	OwnerEffectInstanceName = InEffectInstanceName;
	Data = FNiagaraDataSet(FNiagaraDataSetID(EmitterHandle->GetIdName(), ENiagaraDataSetType::ParticleData));

	UNiagaraEmitterProperties* EmitterProps = EmitterHandle->GetInstance();

	if (EmitterProps != nullptr)
	{
		SpawnRate = EmitterProps->SpawnRate;
	}
	else
	{
		SpawnRate = 0.0f;
	}

}

void FNiagaraSimulation::ResetSimulation()
{
	Data.ResetNumInstances();
	Age = 0;
	Loops = 0;
	CollisionBatch.Reset();
	bHasValidPropertiesAndScripts = true;
	CurrentBurstInstanceIndex = 0;

	SpawnRate = 0.0f;

	const UNiagaraEmitterProperties* PinnedProps = EmitterHandle->GetInstance();

	if (!PinnedProps)
	{
		UE_LOG(LogNiagara, Error, TEXT("Unknown Error creating Niagara Simulation. Properties were null."));
		bHasValidPropertiesAndScripts = false;
		return;
	}

	SpawnRate = PinnedProps->SpawnRate;

	//Check for various failure conditions and bail.
	if (!PinnedProps->UpdateScriptProps.Script || !PinnedProps->SpawnScriptProps.Script)
	{
		//TODO - Arbitrary named scripts. Would need some base functionality for Spawn/Udpate to be called that can be overriden in BPs for emitters with custom scripts.
		UE_LOG(LogNiagara, Error, TEXT("Emitter cannot be enabled because it's doesn't have both an update and spawn script."), *PinnedProps->GetFullName());
		bHasValidPropertiesAndScripts = false;
		return;
	}
	if (PinnedProps->UpdateScriptProps.Script->ByteCode.Num() == 0 || PinnedProps->SpawnScriptProps.Script->ByteCode.Num() == 0)
	{
		UE_LOG(LogNiagara, Error, TEXT("Emitter cannot be enabled because it's spawn or update script was not compiled correctly. %s"), *PinnedProps->GetFullName());
		bHasValidPropertiesAndScripts = false;
		return;
	}

	if (PinnedProps->SpawnScriptProps.Script->DataUsage.bReadsAttriubteData)
	{
		UE_LOG(LogNiagara, Error, TEXT("%s reads attribute data and so cannot be used as a spawn script. The data being read would be invalid."), *PinnedProps->SpawnScriptProps.Script->GetName());
		bHasValidPropertiesAndScripts = false;
		return;
	}
	if (PinnedProps->UpdateScriptProps.Script->Attributes.Num() == 0 || PinnedProps->SpawnScriptProps.Script->Attributes.Num() == 0)
	{
		UE_LOG(LogNiagara, Error, TEXT("This emitter cannot be enabled because it's spawn or update script doesn't have any attriubtes.."));
		bHasValidPropertiesAndScripts = false;
		return;
	}

	ExternalSpawnConstants.Set(PinnedProps->SpawnScriptProps.Script->Parameters);
	ExternalUpdateConstants.Set(PinnedProps->UpdateScriptProps.Script->Parameters);


	if (PinnedProps->EventHandlerScriptProps.Script)
	{
		if (PinnedProps->EventHandlerScriptProps.Script->ByteCode.Num() == 0)
		{
			UE_LOG(LogNiagara, Error, TEXT("%s has an event handler script hat didn't compile correctly."), *GetEmitterHandle().GetName().ToString());
			bHasValidPropertiesAndScripts = false;
			return;
		}

		ExternalEventHandlerConstants.Set(PinnedProps->EventHandlerScriptProps.Script->Parameters);
		ExternalEventHandlerDataInterfaces = PinnedProps->EventHandlerScriptProps.Script->DataInterfaceInfo;
	}
	else
	{
		ExternalEventHandlerConstants.Empty();
		ExternalEventHandlerDataInterfaces.Empty();
	}


}

void FNiagaraSimulation::ReBindDataInterfaces()
{
	// Make sure that our function tables need to be regenerated...
	ExternalSpawnFunctionTable.Empty();
	ExternalUpdateFunctionTable.Empty();
	ExternalEventHandlerFunctionTable.Empty();
}

void FNiagaraSimulation::ReInitDataInterfaces()
{
	ReBindDataInterfaces();

	const UNiagaraEmitterProperties* PinnedProps = EmitterHandle->GetInstance();

	ExternalSpawnDataInterfaces = PinnedProps->SpawnScriptProps.Script->DataInterfaceInfo;
	ExternalUpdateDataInterfaces = PinnedProps->UpdateScriptProps.Script->DataInterfaceInfo;
}

void FNiagaraSimulation::ReInitSimulation()
{
	bIsEnabled = EmitterHandle->GetIsEnabled();
	bDataInterfacesEnabled = false;

	ResetSimulation();

	Data.Reset();
	DataSetMap.Empty();
	BurstInstances.Empty();

	// Make sure that our function tables need to be regenerated...
	ExternalSpawnFunctionTable.Empty();
	ExternalUpdateFunctionTable.Empty();
	ExternalEventHandlerFunctionTable.Empty();

	const UNiagaraEmitterProperties* PinnedProps = EmitterHandle->GetInstance();

	ExternalSpawnDataInterfaces = PinnedProps->SpawnScriptProps.Script->DataInterfaceInfo;
	ExternalUpdateDataInterfaces = PinnedProps->UpdateScriptProps.Script->DataInterfaceInfo;

	//Add the particle data to the data set map.
	//Currently just used for the tick loop but will also allow access directly to the particle data from other emitters.
	DataSetMap.Add(Data.GetID(), &Data);
	//Warn the user if there are any attributes used in the update script that are not initialized in the spawn script.
	//TODO: We need some window in the effect editor and possibly the graph editor for warnings and errors.

	const bool bVerboseAttributeLogging = false;

	if (bVerboseAttributeLogging)
	{
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

	SpawnScriptEventDataSets.Empty();
	for (const FNiagaraEventGeneratorProperties &GeneratorProps : PinnedProps->SpawnScriptProps.EventGenerators)
	{
		FNiagaraDataSet *Set = FNiagaraEventDataSetMgr::CreateEventDataSet(ParentEffectInstance->GetIDName(), EmitterHandle->GetIdName(), GeneratorProps.SetProps.ID.Name);
		Set->Reset();
		Set->AddVariables(GeneratorProps.SetProps.Variables);
		Set->Finalize();
		SpawnScriptEventDataSets.Add(Set);
	}
	
#if STATS
	SpawnStatScopes.Empty();
	if (UNiagaraScript* Script = PinnedProps->SpawnScriptProps.Script)
	{
		for (FNiagaraStatScope& StatScope : Script->StatScopes)
		{
			SpawnStatScopes.Add(FDynamicStats::CreateStatId<FStatGroup_STATGROUP_NiagaraDetailed>(StatScope.FriendlyName.ToString()));
		}
	}
	
	UpdateStatScopes.Empty();
	if (UNiagaraScript* Script = PinnedProps->UpdateScriptProps.Script)
	{
		for (FNiagaraStatScope& StatScope : Script->StatScopes)
		{
			UpdateStatScopes.Add(FDynamicStats::CreateStatId<FStatGroup_STATGROUP_NiagaraDetailed>(StatScope.FriendlyName.ToString()));
		}
	}
	
	EventHandlerStatScopes.Empty();
	if (UNiagaraScript* Script = PinnedProps->EventHandlerScriptProps.Script)
	{
		for (FNiagaraStatScope& StatScope : Script->StatScopes)
		{
			EventHandlerStatScopes.Add(FDynamicStats::CreateStatId<FStatGroup_STATGROUP_NiagaraDetailed>(StatScope.FriendlyName.ToString()));
		}
	}
#endif
}

void FNiagaraSimulation::PostResetSimulation()
{
	if (bHasValidPropertiesAndScripts)
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
  * Also calculates bounds; Kill will be removed from this once we do conditional write
  */
void FNiagaraSimulation::PostProcessParticles()
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraKill);
	int32 OrigNumParticles = Data.GetNumInstances();
	int32 CurNumParticles = OrigNumParticles;

	//TODO: REPLACE AGE/DEATH METHOD WITH A CONDITIONAL WRITE FROM THE SIMULATE.
	FNiagaraVariable AgeVar = FNiagaraVariable(FNiagaraTypeDefinition::GetFloatDef(), TEXT("Age"));
	FNiagaraDataSetIterator<float> AgeItr(Data, AgeVar);
	FNiagaraDataSetIterator<FVector> PosItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	FNiagaraDataSetIterator<FVector2D> SizeItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Size")));
	FNiagaraDataSetIterator<FVector> MeshScaleItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Scale")));

	FVector MaxSize(EForceInit::ForceInitToZero);
	CachedBounds.Init();

	//UE_LOG(LogNiagara, Log, TEXT("KillParticles..."));
	while (AgeItr.IsValid())
	{
		float AgeVal;
		AgeItr.Get(AgeVal);

		if (AgeVal >= 1.0f)
		{
			//UE_LOG(LogNiagara, Log, TEXT("Killed %d"),AgeItr.GetCurrIndex());
			Data.KillInstance(AgeItr.GetCurrIndex());
		}
		else
		{
			// Only increment the iterators if we don't kill a particle because
			// we swap with the last particle and it too may have aged out, so we want
			// to keep looping on the same index as long as it gets swapped out for a 
			// dead particle.

			FVector Position;
			PosItr.Get(Position);

			// Some graphs have a tendency to divide by zero. This ContainsNaN has been added prophylactically
			// to keep us safe during GDC. It should be removed as soon as we feel safe that scripts are appropriately warned.
			if (Position.ContainsNaN() == false)
			{
				CachedBounds += Position;

				// We advance the scale or size depending of if we use either.
				if (MeshScaleItr.IsValid())
				{
					MaxSize = MaxSize.ComponentMax(*MeshScaleItr);
					MeshScaleItr.Advance();
				}
				else if (SizeItr.IsValid())
				{
					MaxSize = MaxSize.ComponentMax(FVector((*SizeItr).GetMax()));
					SizeItr.Advance();
				}

				// Now we advance our main iterators since we've safely handled this particle.
				AgeItr.Advance();
				PosItr.Advance();
			}
			else
			{
				// Kill off an offending particle...
				Data.KillInstance(PosItr.GetCurrIndex());
			}
		}
	}

	CachedBounds = CachedBounds.ExpandBy(MaxSize);

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

	if (!PinnedProps || !bIsEnabled || !bDataInterfacesEnabled || !bHasValidPropertiesAndScripts
		|| TickState == NTS_Suspended || TickState == NTS_Dead)
	{
		return;
	}

	const FNiagaraEventScriptProperties &EventHandlerProps = PinnedProps->EventHandlerScriptProps;

	if (ExternalSpawnFunctionTable.Num() == 0 && PinnedProps->SpawnScriptProps.Script != nullptr && PinnedProps->SpawnScriptProps.Script->DataInterfaceInfo.Num() != 0)
	{
		if (!UpdateFunctionTableInternal(ExternalSpawnDataInterfaces, ExternalSpawnFunctionTable, PinnedProps->SpawnScriptProps.Script))
		{
			UE_LOG(LogNiagara, Warning, TEXT("Failed to sync data interface table for spawn script!"));
			bDataInterfacesEnabled = false;
			return;
		}
	}

	if (ExternalUpdateFunctionTable.Num() == 0 && PinnedProps->UpdateScriptProps.Script != nullptr && PinnedProps->UpdateScriptProps.Script->DataInterfaceInfo.Num() != 0)
	{
		if (!UpdateFunctionTableInternal(ExternalUpdateDataInterfaces, ExternalUpdateFunctionTable, PinnedProps->UpdateScriptProps.Script))
		{
			UE_LOG(LogNiagara, Warning, TEXT("Failed to sync data interface table for update script!"));
			bDataInterfacesEnabled = false;
			return;
		}
	}

	if (ExternalEventHandlerFunctionTable.Num() == 0 && EventHandlerProps.Script != nullptr && EventHandlerProps.Script->DataInterfaceInfo.Num() != 0)
	{
		if (!UpdateFunctionTableInternal(ExternalEventHandlerDataInterfaces, ExternalEventHandlerFunctionTable, EventHandlerProps.Script))
		{
			UE_LOG(LogNiagara, Warning, TEXT("Failed to sync data interface table for event script!"));
			bDataInterfacesEnabled = false;
			return;
		}
	}

	if (Data.GetNumInstances() == 0)
	{
		return;
	}

	check(Data.GetNumVariables() > 0);
	check(PinnedProps->SpawnScriptProps.Script);
	check(PinnedProps->UpdateScriptProps.Script);

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

	for (FNiagaraDataSet* Set : SpawnScriptEventDataSets)
	{
		Set->Tick();
	}
}


void FNiagaraSimulation::Tick(float DeltaSeconds)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraTick);
	SimpleTimer TickTime;

	const UNiagaraEmitterProperties* PinnedProps = EmitterHandle->GetInstance();
	if (!PinnedProps || !bIsEnabled || !bDataInterfacesEnabled || !bHasValidPropertiesAndScripts || TickState == NTS_Suspended || TickState == NTS_Dead)
	{
		return;
	}

	const FNiagaraEventScriptProperties &EventHandlerProps = PinnedProps->EventHandlerScriptProps;

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

		if (PinnedProps->SpawnScriptProps.Script->IsInterpolatedSpawnScript())
		{
			SpawnConstants.Merge(UpdateConstants);
		}

		//Pull effect params.
		FNiagaraParameters& EffectParams = GetParentEffectInstance()->GetEffectParameters();
		SpawnConstants.Merge(EffectParams);
		UpdateConstants.Merge(EffectParams);
		EventHandlerConstants.Merge(EffectParams);

		//Set emitter params.
		//TODO: THIS IS ALL REALLY PANTS.
		//Need to rework so the array to be used for execution is directly updatable here with the values themselves rather than these intermediate FNiagaraVariables.
		DeltaTime.SetValue(DeltaSeconds);
		InvDeltaTime.SetValue(1.0f / DeltaSeconds);
		EmitterAge.SetValue(Age);
		SpawnRateParam.SetValue(SpawnRate);

		//Setup these so it always has an entry in the prev table too regardless of any spawns that actually happen.
		ExecCount.SetValue(0);
		SpawnConstants.SetOrAdd(ExecCount);
		SpawnIntervalParam.SetValue(0.0f);
		SpawnConstants.SetOrAdd(SpawnIntervalParam);
		InterpSpawnStartDtParam.SetValue(0.0f);
		SpawnConstants.SetOrAdd(InterpSpawnStartDtParam);

		SpawnConstants.SetOrAdd(DeltaTime);
		SpawnConstants.SetOrAdd(InvDeltaTime);
		SpawnConstants.SetOrAdd(EmitterAge);
		SpawnConstants.SetOrAdd(SpawnRateParam);

		UpdateConstants.SetOrAdd(DeltaTime);
		UpdateConstants.SetOrAdd(InvDeltaTime);
		UpdateConstants.SetOrAdd(EmitterAge);

		EventHandlerConstants.SetOrAdd(DeltaTime);
		EventHandlerConstants.SetOrAdd(InvDeltaTime);
		EventHandlerConstants.SetOrAdd(EmitterAge);
	}
	
	// Calculate number of new particles from regular spawning 
	int32 OrigNumParticles = Data.GetPrevNumInstances();
	float PrevSpawnRemainder = SpawnRemainder;
	int32 SpawnRateCount = CalcNumToSpawn(DeltaSeconds);

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

	uint32 BurstSpawnTotal = 0;
	TArray<FNiagaraBurstInstance, TInlineAllocator<8>> BurstsToProcess;
	while (CurrentBurstInstanceIndex < BurstInstances.Num() && BurstInstances[CurrentBurstInstanceIndex].Time < Age)
	{
		BurstsToProcess.Add(BurstInstances[CurrentBurstInstanceIndex++]);
		BurstSpawnTotal += BurstsToProcess.Last().NumberToSpawn;
	}

	//Allocate space for prev frames particles and any new one's we're going to spawn.
	Data.Allocate(OrigNumParticles + SpawnRateCount + BurstSpawnTotal + EventSpawnTotal);

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

		FNiagaraScriptExecutionContext Context(PinnedProps->UpdateScriptProps.Script, ExternalUpdateFunctionTable, 0, Data.GetNumInstances() UPDATE_STAT_SCOPES_PARAM);
		Context.AddDataSet(Data);
		Context.AddEventDataSets(UpdateScriptEventDataSets);
		Context.InitParameters(UpdateConstants);
		Context.Execute();
		Context.ResolveDataSetWrites();

		if (GbDumpParticleData)
		{
			UE_LOG(LogNiagara, Log, TEXT("=== Updated %d Particles ==="), OrigNumParticles);
			Data.Dump(true);
		}
	}

	//Init new particles with the spawn script.
	if (TickState==NTS_Running)
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraSpawn);

		if (PrevSpawnConstants.GetTableSize() == 0)
		{
			PrevSpawnConstants = SpawnConstants;
		}

		//Handle main spawn rate spawning
		auto SpawnParticles = [&](int32 Num, FString DumpLabel)
		{
			if (Num > 0)
			{
				int32 OrigNum = Data.GetNumInstances();
				Data.SetNumInstances(OrigNum + Num);
				ExecCount.SetValue(Num);
				SpawnConstants.SetOrAdd(ExecCount);
				FNiagaraScriptExecutionContext Context(PinnedProps->SpawnScriptProps.Script, ExternalSpawnFunctionTable, OrigNum, Num SPAWN_STAT_SCOPES_PARAM);
				Context.AddDataSet(Data);
				Context.AddEventDataSets(SpawnScriptEventDataSets);
				Context.InitParameters(SpawnConstants, PrevSpawnConstants);
				Context.Execute();
				Context.ResolveDataSetWrites();

				if (GbDumpParticleData)
				{
					UE_LOG(LogNiagara, Log, TEXT("=== %s ==="), *DumpLabel);
					UE_LOG(LogNiagara, Log, TEXT("=== Spawned %d Particles ==="), Num);
					Data.Dump(true);
				}
			}
		};

		//Spawn particles from spawn rate.
		{
			float SpawnInterval = 1.0f / SpawnRate;
			SpawnIntervalParam.SetValue(SpawnInterval);
			InterpSpawnStartDtParam.SetValue((1.0f - PrevSpawnRemainder) * SpawnInterval);
			SpawnConstants.SetOrAdd(SpawnIntervalParam);
			SpawnConstants.SetOrAdd(InterpSpawnStartDtParam);

			SpawnParticles(SpawnRateCount, TEXT("Spawn Rate"));
		}
		
		//Spawn particles coming from bursts.
		for(FNiagaraBurstInstance& Burst : BurstsToProcess)
		{
			//Setup for instantaneous bursts at a particular time in the frame.
			SpawnIntervalParam.SetValue(0.0f);
			InterpSpawnStartDtParam.SetValue(DeltaSeconds - (Age - Burst.Time));
			SpawnConstants.SetOrAdd(SpawnIntervalParam);
			SpawnConstants.SetOrAdd(InterpSpawnStartDtParam);

			SpawnParticles(Burst.NumberToSpawn, FString::Printf(TEXT("Burst at "),Burst.Time));
		}

		//Spawn particles coming from events.
		for (int32 Num : EventSpawnCounts)
		{
			//Event spawns are instantaneous at the middle of the frame?
			SpawnIntervalParam.SetValue(0.0f);
			InterpSpawnStartDtParam.SetValue(DeltaSeconds * 0.5f);
			SpawnConstants.SetOrAdd(SpawnIntervalParam);
			SpawnConstants.SetOrAdd(InterpSpawnStartDtParam);

			SpawnParticles(Num, TEXT("Event Spawn"));
		}
	}

	PrevSpawnConstants = SpawnConstants;

	// handle event based spawning
	if (bPerformEventSpawning && EventSet && EventSpawnCounts.Num())
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraEventHandle);
		uint32 EventSpawnStart = OrigNumParticles + SpawnRateCount + BurstSpawnTotal;
		for (int32 i = 0; i < EventSpawnCounts.Num(); i++)
		{
			int32 EventNumToSpawn = EventSpawnCounts[i];

			ExecCount.SetValue(EventNumToSpawn);
			EventHandlerConstants.SetOrAdd(ExecCount);

			FNiagaraScriptExecutionContext Context(PinnedProps->EventHandlerScriptProps.Script, ExternalEventHandlerFunctionTable, EventSpawnStart, EventNumToSpawn EVENT_STAT_SCOPES_PARAM);
			Context.AddDataSet(Data);
			Context.AddEventDataSet(*EventSet, i);
			Context.InitParameters(EventHandlerConstants);
			Context.Execute();

			EventSpawnStart += EventNumToSpawn;
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

				FNiagaraScriptExecutionContext Context(PinnedProps->EventHandlerScriptProps.Script, ExternalEventHandlerFunctionTable, 0, Data.GetPrevNumInstances() EVENT_STAT_SCOPES_PARAM);
				Context.AddDataSet(Data);
				Context.AddEventDataSet(*EventSet, i);
				Context.InitParameters(EventHandlerConstants);
				Context.Execute();
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
				FNiagaraScriptExecutionContext Context(PinnedProps->EventHandlerScriptProps.Script, ExternalEventHandlerFunctionTable, Index, 1 EVENT_STAT_SCOPES_PARAM);
				Context.AddDataSet(Data);
				Context.AddEventDataSet(*EventSet, i);
				Context.InitParameters(EventHandlerConstants);
				Context.Execute();
			}
		}
	}


	// kick off collision tests from this emitter
	if (PinnedProps->CollisionMode == ENiagaraCollisionMode::SceneGeometry)
	{
		CollisionBatch.KickoffNewBatch(this, DeltaSeconds);
	}

	PostProcessParticles();

	//Store off the previous spawn constants.
	PrevSpawnConstants = SpawnConstants;

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
		NumEventsToProcess = EventSet->GetPrevNumInstances();
		if(EventHandlerProps.MaxEventsPerFrame > 0)
		{
			NumEventsToProcess = FMath::Min<int32>(EventSet->GetPrevNumInstances(), EventHandlerProps.MaxEventsPerFrame);
		}

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

	float FloatNumToSpawn = SpawnRemainder + (DeltaSeconds * SpawnRate);
	int32 NumToSpawn = FMath::FloorToInt(FloatNumToSpawn);
	SpawnRemainder = FloatNumToSpawn - NumToSpawn;

	return NumToSpawn;
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
void FNiagaraSimulation::UpdateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel, TArray<NiagaraEffectRenderer*>& ToBeRemovedList)
{
	const UNiagaraEmitterProperties* EmitterProperties = EmitterHandle->GetInstance();
	if (bIsEnabled && bDataInterfacesEnabled && bHasValidPropertiesAndScripts && 
	    EmitterProperties != nullptr && EmitterProperties->RendererProperties != nullptr)
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
			ToBeRemovedList.Add(EffectRenderer);
		}

		EffectRenderer = EmitterProperties->RendererProperties->CreateEffectRenderer(FeatureLevel);
		EffectRenderer->SetMaterial(Material, FeatureLevel);
		EffectRenderer->SetLocalSpace(EmitterProperties->bLocalSpace);

		//UE_LOG(LogNiagara, Warning, TEXT("CreateRenderer %p"), EffectRenderer);
		CheckAttributesForRenderer();
	}
	else
	{
		if (EffectRenderer)
		{
			ToBeRemovedList.Add(EffectRenderer);
			EffectRenderer = nullptr;
		}
	}
}

bool FNiagaraSimulation::UpdateFunctionTableInternal(TArray<FNiagaraScriptDataInterfaceInfo>& DataInterfaces, TArray<FVMExternalFunction>& FunctionTable, UNiagaraScript* Script)
{
	FunctionTable.Empty();

	// We must make sure that the data interfaces match up between the original script values and our overrides...
	if (Script->DataInterfaceInfo.Num() != DataInterfaces.Num())
	{
		return false;
	}
	
	bool bSuccessfullyMapped = true;
	for (FVMExternalFunctionBindingInfo& BindingInfo : Script->CalledVMExternalFunctions)
	{
		for (int32 i = 0; i < Script->DataInterfaceInfo.Num(); i++)
		{
			FNiagaraScriptDataInterfaceInfo& ScriptInfo = Script->DataInterfaceInfo[i];
			FNiagaraScriptDataInterfaceInfo& ExternalInfo = DataInterfaces[i];

			if (ScriptInfo.Id == BindingInfo.OwnerId)
			{
				FVMExternalFunction Func = ExternalInfo.DataInterface->GetVMExternalFunction(BindingInfo);
				if (Func.IsBound())
				{
					FunctionTable.Add(Func);
				}
				else
				{
					UE_LOG(LogNiagara, Error, TEXT("Could not GetVMExternalFunction '%s'.. emitter will not run!"), *BindingInfo.Name.ToString())
						bSuccessfullyMapped = false;
				}
			}
		}
	}

	if (!bSuccessfullyMapped)
	{
		FunctionTable.Empty();
		return false;
	}
	
	return true;
}
