// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraSystemInstance.h"
#include "NiagaraConstants.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterface.h"
#include "NiagaraStats.h"
#include "Async/ParallelFor.h"
#include "NiagaraParameterCollection.h"
#include "NiagaraWorldManager.h"
#include "NiagaraComponent.h"
#include "NiagaraRenderer.h"
#include "GameFramework/PlayerController.h"

DECLARE_CYCLE_STAT(TEXT("Parallel Tick"), STAT_NiagaraParallelTick, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("System Reset (GT)"), STAT_NiagaraSystemReset, STATGROUP_Niagara);
DECLARE_CYCLE_STAT(TEXT("System Reinit (GT)"), STAT_NiagaraSystemReinit, STATGROUP_Niagara);

 
/** Safety time to allow for the LastRenderTime coming back from the RT. */
static float GLastRenderTimeSafetyBias = 0.1f;
static FAutoConsoleVariableRef CVarLastRenderTimeSafetyBias(
	TEXT("fx.LastRenderTimeSafetyBias"),
	GLastRenderTimeSafetyBias,
	TEXT("The time to bias the LastRenderTime value to allow for the delay from it being written by the RT."),
	ECVF_Default
);

FNiagaraSystemInstance::FNiagaraSystemInstance(UNiagaraComponent* InComponent)
	: SystemInstanceIndex(INDEX_NONE)
	, Component(InComponent)
	, Age(0.0f)
	, ID(FGuid::NewGuid())
	, IDName(*ID.ToString())
	, InstanceParameters(Component)
	, bSolo(false)
	, bForceSolo(false)
	, bError(true)
	, bDataInterfacesNeedInit(true)
	, bHasTickingEmitters(true)
	, ExecutionState(ENiagaraExecutionState::Complete)
{
	SystemBounds.Init();
}

void FNiagaraSystemInstance::Init(TSharedRef<FNiagaraSystemSimulation, ESPMode::ThreadSafe> InSystemSimulation, bool bInForceSolo)
{
	SystemSimulation = InSystemSimulation;

	bError = false;
	bForceSolo = bInForceSolo;
	ExecutionState = ENiagaraExecutionState::Inactive;

	//InstanceParameters = GetSystem()->GetInstanceParameters();
	InitEmitters();
	Reset(EResetMode::ReInit);

#if WITH_EDITORONLY_DATA
	InstanceParameters.Name = *FString::Printf(TEXT("SystemInstance %p"), this);
#endif
	OnInitializedDelegate.Broadcast();
}

void FNiagaraSystemInstance::SetExecutionState(ENiagaraExecutionState InState)
{
	if (ExecutionState != InState)
	{
		const UEnum* EnumPtr = FNiagaraTypeDefinition::GetExecutionStateEnum();
		UE_LOG(LogNiagara, Log, TEXT("System \"%s\" change state: %s to %s"), *GetSystem()->GetName(), *EnumPtr->GetNameStringByValue((int64)ExecutionState),
			*EnumPtr->GetNameStringByValue((int64)InState));

	}
	ExecutionState = InState;
}

void FNiagaraSystemInstance::Activate(bool bReset /* = false */)
{
	if (GetSystem() && GetSystem()->IsValid() && IsReadyToRun())
	{
		if (IsComplete() || bReset)
		{
			Reset(EResetMode::ResetAll);
		}
		else
		{
			Reset(EResetMode::ResetSystem);
		}

		ExecutionState = ENiagaraExecutionState::Active;
	}
	else
	{
		ExecutionState = ENiagaraExecutionState::Disabled;
	}
}

void FNiagaraSystemInstance::Deactivate(bool bImmediate)
{
	if (IsComplete())
	{
		return;
	}

	if (bImmediate)
	{
		Complete();
	}
	else
	{
		ExecutionState = ENiagaraExecutionState::Inactive;
	}
}

void FNiagaraSystemInstance::Complete()
{
	TSharedPtr<FNiagaraSystemSimulation, ESPMode::ThreadSafe> SystemSim = GetSystemSimulation();
	SystemSim->RemoveInstance(this);

	ExecutionState = ENiagaraExecutionState::Complete;

	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
	{
		Simulation->HandleCompletion(true);
	}

	if (Component)
	{
		Component->OnSystemComplete();
	}

	OnCompleteDelegate.Broadcast(this);
}

void FNiagaraSystemInstance::Reset(FNiagaraSystemInstance::EResetMode Mode)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraSystemReset);

	TSharedPtr<FNiagaraSystemSimulation, ESPMode::ThreadSafe> SystemSim = GetSystemSimulation();
	if (!SystemSim.IsValid())
	{
		return;
	}

	Component->LastRenderTime = Component->GetWorld()->GetTimeSeconds();

	SystemSim->RemoveInstance(this);
	bPendingSpawn = true;

	if (bError)
	{
		Mode = EResetMode::ReInit;
	}
		
	if (Mode == EResetMode::ResetSystem)
	{
		ResetInternal(false);
	}
	else if (Mode == EResetMode::ResetAll)
	{
		ResetInternal(true);
	}
	else if (Mode == EResetMode::ReInit)
	{
		DestroyDataInterfaceInstanceData();
		ReInitInternal();
	}
	
	if (!bError)
	{
		if (bSolo)
		{
			SystemSim->ResetSolo(this);
		}
		else
		{
			SystemSim->AddInstance(this);
		}
	}
}

void FNiagaraSystemInstance::ResetInternal(bool bResetSimulations)
{
	Age = 0;
	UNiagaraSystem* System = GetSystem();
	if (System == nullptr || Component == nullptr || bError)
	{
		return;
	}

#if WITH_EDITOR
	if (Component->GetWorld() != nullptr && Component->GetWorld()->WorldType == EWorldType::Editor)
	{
		Component->SynchronizeWithSourceSystem();
		Component->GetOverrideParameters().Tick();
	}
#endif

	bool bAllReadyToRun = IsReadyToRun();

	if (!bAllReadyToRun)
	{
		return;
	}

	if (!System->IsValid())
	{
		bError = true;		
		UE_LOG(LogNiagara, Warning, TEXT("Failed to activate Niagara System due to invalid asset!"));
		return;
	}

	if (bResetSimulations)
	{
		for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
		{
			Simulation->ResetSimulation();
		}
	}

#if WITH_EDITOR
	//UE_LOG(LogNiagara, Log, TEXT("OnResetInternal %p"), this);
	OnResetDelegate.Broadcast();
#endif
}

UNiagaraParameterCollectionInstance* FNiagaraSystemInstance::GetParameterCollectionInstance(UNiagaraParameterCollection* Collection)
{
	return SystemSimulation->GetParameterCollectionInstance(Collection);
}

bool FNiagaraSystemInstance::IsReadyToRun() const
{
	bool bAllReadyToRun = true;

	UNiagaraSystem* System = GetSystem();

	if (!System || !System->IsReadyToRun())
	{
		return false;
	}

	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
	{
		if (!Simulation->IsReadyToRun())
		{
			bAllReadyToRun = false;
		}
	}
	return bAllReadyToRun;
}

void FNiagaraSystemInstance::ReInitInternal()
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraSystemReinit);
	bool bPrevError = bError;
	Age = 0;
	UNiagaraSystem* System = GetSystem();
	if (System == nullptr || Component == nullptr)
	{
		return;
	}
	bError = false;

#if WITH_EDITOR
	if (Component->GetWorld() != nullptr && Component->GetWorld()->WorldType == EWorldType::Editor)
	{
		Component->SynchronizeWithSourceSystem();
	}
#endif

	bool bAllReadyToRun = IsReadyToRun();

	if (!bAllReadyToRun)
	{
		return;
	}
	
	if (!System->IsValid())
	{
		bError = true;
		UE_LOG(LogNiagara, Warning, TEXT("Failed to activate Niagara System due to invalid asset!"));
		return;
	}

	//When re initializing, throw away old emitters and init new ones.
	Emitters.Reset();
	InitEmitters();
	
	InstanceParameters.Empty();
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_POSITION);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_SCALE);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_VELOCITY);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_X_AXIS);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_Y_AXIS);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_Z_AXIS);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_LOCAL_TO_WORLD);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_WORLD_TO_LOCAL);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_LOCAL_TO_WORLD_TRANSPOSED);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_WORLD_TO_LOCAL_TRANSPOSED);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_LOCAL_TO_WORLD_NO_SCALE);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_WORLD_TO_LOCAL_NO_SCALE);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_DELTA_TIME);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_INV_DELTA_TIME);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_TIME_SINCE_RENDERED);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_MIN_DIST_TO_CAMERA);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_SYSTEM_NUM_EMITTERS);
	InstanceParameters.AddParameter(SYS_PARAM_ENGINE_SYSTEM_NUM_EMITTERS_ALIVE);

	OwnerPositionParam.Init(InstanceParameters, SYS_PARAM_ENGINE_POSITION);
	OwnerScaleParam.Init(InstanceParameters, SYS_PARAM_ENGINE_SCALE);
	OwnerVelocityParam.Init(InstanceParameters, SYS_PARAM_ENGINE_VELOCITY);
	OwnerXAxisParam.Init(InstanceParameters, SYS_PARAM_ENGINE_X_AXIS);
	OwnerYAxisParam.Init(InstanceParameters, SYS_PARAM_ENGINE_Y_AXIS);
	OwnerZAxisParam.Init(InstanceParameters, SYS_PARAM_ENGINE_Z_AXIS);

	OwnerTransformParam.Init(InstanceParameters, SYS_PARAM_ENGINE_LOCAL_TO_WORLD);
	OwnerInverseParam.Init(InstanceParameters, SYS_PARAM_ENGINE_WORLD_TO_LOCAL);
	OwnerTransposeParam.Init(InstanceParameters, SYS_PARAM_ENGINE_LOCAL_TO_WORLD_TRANSPOSED);
	OwnerInverseTransposeParam.Init(InstanceParameters, SYS_PARAM_ENGINE_WORLD_TO_LOCAL_TRANSPOSED);
	OwnerTransformNoScaleParam.Init(InstanceParameters, SYS_PARAM_ENGINE_LOCAL_TO_WORLD_NO_SCALE);
	OwnerInverseNoScaleParam.Init(InstanceParameters, SYS_PARAM_ENGINE_WORLD_TO_LOCAL_NO_SCALE);

	OwnerDeltaSecondsParam.Init(InstanceParameters, SYS_PARAM_ENGINE_DELTA_TIME);
	OwnerInverseDeltaSecondsParam.Init(InstanceParameters, SYS_PARAM_ENGINE_INV_DELTA_TIME);
	OwnerMinDistanceToCameraParam.Init(InstanceParameters, SYS_PARAM_ENGINE_MIN_DIST_TO_CAMERA);
	SystemNumEmittersParam.Init(InstanceParameters, SYS_PARAM_ENGINE_SYSTEM_NUM_EMITTERS);
	SystemNumEmittersAliveParam.Init(InstanceParameters, SYS_PARAM_ENGINE_SYSTEM_NUM_EMITTERS_ALIVE);

	SystemTimeSinceRenderedParam.Init(InstanceParameters, SYS_PARAM_ENGINE_TIME_SINCE_RENDERED);

	ParameterNumParticleBindings.SetNum(Emitters.Num());
	for (int32 i = 0; i < Emitters.Num(); i++)
	{
		TSharedRef<FNiagaraEmitterInstance> Simulation = Emitters[i];
		FString EmitterName = Simulation->GetEmitterHandle().GetInstance()->GetUniqueEmitterName();
		FNiagaraVariable Var = SYS_PARAM_ENGINE_EMITTER_NUM_PARTICLES;
		FString ParamName = Var.GetName().ToString().Replace(TEXT("Emitter"), *EmitterName);
		Var.SetName(*ParamName);
		InstanceParameters.AddParameter(Var);
		ParameterNumParticleBindings[i].Init(InstanceParameters, Var);
	}

	const bool bOnlyAdd = false;
	System->GetExposedParameters().CopyParametersTo(InstanceParameters, bOnlyAdd);

	TickInstanceParameters(0.01f);

	//Determine if we can update normally or have to update solo.
	bSolo = bForceSolo;
	//If our scripts have any interfaces that require instance data.
	UNiagaraScript* SystemSpawn = GetSystem()->GetSystemSpawnScript();
	for (int32 i = 0; !bSolo && i < SystemSpawn->DataInterfaceInfo.Num(); ++i)
	{
		FNiagaraScriptDataInterfaceInfo& Info = SystemSpawn->DataInterfaceInfo[i];
		if ( (Info.DataInterface && Info.DataInterface->PerInstanceDataSize() > 0) ||
			Info.Name.ToString().StartsWith("User."))//Temp hack to force solo on any systems with system scrips needing user (aka per instance) interfaces.
		{
			bSolo = true;
			break;
		}
		
	}	
	UNiagaraScript* SystemUpdate = GetSystem()->GetSystemUpdateScript();
	for (int32 i = 0; !bSolo && i < SystemUpdate->DataInterfaceInfo.Num(); ++i)
	{
		FNiagaraScriptDataInterfaceInfo& Info = SystemUpdate->DataInterfaceInfo[i];
		if ( (Info.DataInterface && Info.DataInterface->PerInstanceDataSize() > 0) ||
			Info.Name.ToString().StartsWith("User."))//Temp hack to force solo on any systems with system scrips needing user (aka per instance) interfaces.
		{
			bSolo = true;
			break;
		}
	}

	BindParameters();

	InitDataInterfaces(); // We can't wait until tick b/c the data interfaces need to be around when we update render modules.
	//FlushRenderingCommands();//Ah! This causes massive hitch on activation of new fx. 

	// This gets a little tricky, but we want to delete any renderers that are no longer in use on the rendering thread, but
	// first (to be safe), we want to update the proxy to point to the new renderer objects.

	// Step 1: Recreate the renderers on the simulations, we keep the old and new renderers.
	TArray<NiagaraRenderer*> NewRenderers;
	TArray<NiagaraRenderer*> OldRenderers;

	UpdateRenderModules(Component->GetWorld()->FeatureLevel, NewRenderers, OldRenderers);

	// Step 2: Update the proxy with the new renderers that were created.
	UpdateProxy(NewRenderers);
	Component->MarkRenderStateDirty();

	// Step 3: Queue up the old renderers for deletion on the render thread.
	for (NiagaraRenderer* Renderer : OldRenderers)
	{
		if (Renderer != nullptr)
		{
			Renderer->Release();
		}
	}

#if WITH_EDITOR
	//UE_LOG(LogNiagara, Log, TEXT("OnResetInternal %p"), this);
	OnResetDelegate.Broadcast();
#endif

}

FNiagaraSystemInstance::~FNiagaraSystemInstance()
{
	//UE_LOG(LogNiagara, Warning, TEXT("~FNiagaraSystemInstance %p"), this);

	//FlushRenderingCommands();

	if (SystemInstanceIndex != INDEX_NONE)
	{
		TSharedPtr<FNiagaraSystemSimulation, ESPMode::ThreadSafe> SystemSim = GetSystemSimulation();
		SystemSim->RemoveInstance(this);
	}

	DestroyDataInterfaceInstanceData();

	// Clear out the System renderer from the proxy.
	TArray<NiagaraRenderer*> NewRenderers;
	UpdateProxy(NewRenderers);

	// Clear out the System renderer from the simulation.
	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
	{
		Simulation->ClearRenderer();
	}

	UnbindParameters();

	// Clear out the emitters.
	Emitters.Empty(0);

// #if WITH_EDITOR
// 	OnDestroyedDelegate.Broadcast();
// #endif
}

//Unsure on usage of this atm. Possibly useful in future.
// void FNiagaraSystemInstance::RebindParameterCollection(UNiagaraParameterCollectionInstance* OldInstance, UNiagaraParameterCollectionInstance* NewInstance)
// {
// 	OldInstance->GetParameterStore().Unbind(&InstanceParameters);
// 	NewInstance->GetParameterStore().Bind(&InstanceParameters);
// 
// 	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
// 	{
// 		Simulation->RebindParameterCollection(OldInstance, NewInstance);
// 	}
// 
// 	//Have to re init the instance data for data interfaces.
// 	//This is actually lots more work than absolutely needed in some cases so we can improve it a fair bit.
// 	InitDataInterfaces();
// }

void FNiagaraSystemInstance::BindParameters()
{
	Component->GetOverrideParameters().Bind(&InstanceParameters);

	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
	{
		Simulation->BindParameters();
	}
}

void FNiagaraSystemInstance::UnbindParameters()
{
	Component->GetOverrideParameters().Unbind(&InstanceParameters);

	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
	{
		Simulation->UnbindParameters();
	}
}

FNiagaraWorldManager* FNiagaraSystemInstance::GetWorldManager()const
{
	return Component ? FNiagaraWorldManager::Get(Component->GetWorld()) : nullptr; 
}

void FNiagaraSystemInstance::InitDataInterfaces()
{
	// If either the System or the component is invalid, it is possible that our cached data interfaces
	// are now bogus and could point to invalid memory. Only the UNiagaraComponent or UNiagaraSystem
	// can hold onto GC references to the DataInterfaces.
	if (GetSystem() == nullptr || bError)
	{
		return;
	}

	if (Component == nullptr)
	{
		return;
	}

	Component->GetOverrideParameters().Tick();
	
	bDataInterfacesNeedInit = false;

	DestroyDataInterfaceInstanceData();

	//Now the interfaces in the simulations are all correct, we can build the per instance data table.
	int32 InstanceDataSize = 0;
	DataInterfaceInstanceDataOffsets.Empty();
	auto CalcInstDataSize = [&](const TArray<UNiagaraDataInterface*>& Interfaces)
	{
		for (UNiagaraDataInterface* Interface : Interfaces)
		{
			if (!Interface)
			{
				continue;
			}

			if (int32 Size = Interface->PerInstanceDataSize())
			{
				int32* ExistingInstanceDataOffset = DataInterfaceInstanceDataOffsets.Find(Interface);
				if (!ExistingInstanceDataOffset)//Don't add instance data for interfaces we've seen before.
				{
					DataInterfaceInstanceDataOffsets.Add(Interface) = InstanceDataSize;
					InstanceDataSize += Size;
				}
			}
		}
	};

	CalcInstDataSize(InstanceParameters.GetDataInterfaces());//This probably should be a proper exec context. 
	if (bSolo)
	{
		TSharedPtr<FNiagaraSystemSimulation, ESPMode::ThreadSafe> SystemSim = GetSystemSimulation();
		
		CalcInstDataSize(SystemSim->GetSoloDataInterfacesSpawn());
		CalcInstDataSize(SystemSim->GetSoloDataInterfacesUpdate());
	}

	//Iterate over interfaces to get size for table and clear their interface bindings.
	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
	{
		FNiagaraEmitterInstance& Sim = Simulation.Get();
		CalcInstDataSize(Sim.GetSpawnExecutionContext().GetDataInterfaces());
		CalcInstDataSize(Sim.GetUpdateExecutionContext().GetDataInterfaces());
		for (int32 i = 0; i < Sim.GetEventExecutionContexts().Num(); i++)
		{
			CalcInstDataSize(Sim.GetEventExecutionContexts()[i].GetDataInterfaces());
		}

		//Also force a rebind while we're here.
		Sim.DirtyDataInterfaces();
	}

	DataInterfaceInstanceData.SetNumUninitialized(InstanceDataSize);

	bool bOk = true;
	for (TPair<TWeakObjectPtr<UNiagaraDataInterface>, int32>& Pair : DataInterfaceInstanceDataOffsets)
	{
		if (UNiagaraDataInterface* Interface = Pair.Key.Get())
		{
			//Ideally when we make the batching changes, we can keep the instance data in big single type blocks that can all be updated together with a single virtual call.
			bOk &= Pair.Key->InitPerInstanceData(&DataInterfaceInstanceData[Pair.Value], this);
		}
		else
		{
			UE_LOG(LogNiagara, Error, TEXT("A data interface currently in use by an System has been destroyed."));
			bOk = false;
		}
	}

	if (!bOk)
	{
		UE_LOG(LogNiagara, Error, TEXT("Error initializing data interfaces."));
		bError = true;
		Component->MarkRenderStateDirty();
	}
}

void FNiagaraSystemInstance::TickDataInterfaces(float DeltaSeconds, bool bPostSimulate)
{
	if (!GetSystem() || !Component || bError)
	{
		return;
	}

	bool bReInitDataInterfaces = false;
	for (TPair<TWeakObjectPtr<UNiagaraDataInterface>, int32>& Pair : DataInterfaceInstanceDataOffsets)
	{
		if(UNiagaraDataInterface* Interface = Pair.Key.Get())
		{
			//Ideally when we make the batching changes, we can keep the instance data in big single type blocks that can all be updated together with a single virtual call.
			if (bPostSimulate)
			{
				bReInitDataInterfaces |= Interface->PerInstanceTickPostSimulate(&DataInterfaceInstanceData[Pair.Value], this, DeltaSeconds);
			}
			else
			{
				bReInitDataInterfaces |= Interface->PerInstanceTick(&DataInterfaceInstanceData[Pair.Value], this, DeltaSeconds);
			}
		}
	}
	
	//This should ideally really only happen at edit time.
	if (bReInitDataInterfaces)
	{
		InitDataInterfaces();	
	}
}

void FNiagaraSystemInstance::TickInstanceParameters(float DeltaSeconds)
{
	//TODO: Create helper binding objects to avoid the search in set parameter value.
	//Set System params.
	FTransform ComponentTrans = Component->GetComponentTransform();
	FVector OldPos = OwnerPositionParam.GetValue();// ComponentTrans.GetLocation();
	FVector CurrPos = ComponentTrans.GetLocation();
	OwnerPositionParam.SetValue(CurrPos);
	OwnerScaleParam.SetValue(ComponentTrans.GetScale3D());
	OwnerVelocityParam.SetValue((CurrPos - OldPos) / DeltaSeconds);
	OwnerXAxisParam.SetValue(ComponentTrans.GetRotation().GetAxisX());
	OwnerYAxisParam.SetValue(ComponentTrans.GetRotation().GetAxisY());
	OwnerZAxisParam.SetValue(ComponentTrans.GetRotation().GetAxisZ());

	FMatrix Transform = ComponentTrans.ToMatrixWithScale();
	FMatrix Inverse = Transform.Inverse();
	FMatrix Transpose = Transform.GetTransposed();
	FMatrix InverseTranspose = Inverse.GetTransposed();
	OwnerTransformParam.SetValue(Transform);
	OwnerInverseParam.SetValue(Inverse);
	OwnerTransposeParam.SetValue(Transpose);
	OwnerInverseTransposeParam.SetValue(InverseTranspose);

	FMatrix TransformNoScale = ComponentTrans.ToMatrixNoScale();
	FMatrix InverseNoScale = TransformNoScale.Inverse();
	OwnerTransformNoScaleParam.SetValue(TransformNoScale);
	OwnerInverseNoScaleParam.SetValue(InverseNoScale);

	OwnerDeltaSecondsParam.SetValue(DeltaSeconds);
	OwnerInverseDeltaSecondsParam.SetValue(1.0f / DeltaSeconds);

	//Calculate the min distance to a camera.
	UWorld* World = Component->GetWorld();
	if (World != NULL)
	{
		TArray<FVector, TInlineAllocator<8> > PlayerViewLocations;
		if (World->GetPlayerControllerIterator())
		{
			for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				APlayerController* PlayerController = Iterator->Get();
				if (PlayerController->IsLocalPlayerController())
				{
					FVector* POVLoc = new(PlayerViewLocations) FVector;
					FRotator POVRotation;
					PlayerController->GetPlayerViewPoint(*POVLoc, POVRotation);
				}
			}
		}
		else
		{
			PlayerViewLocations.Append(World->ViewLocationsRenderedLastFrame);
		}

		float LODDistanceSqr = (PlayerViewLocations.Num() ? FMath::Square(WORLD_MAX) : 0.0f);
		for (const FVector& ViewLocation : PlayerViewLocations)
		{
			const float DistanceToEffectSqr = FVector(ViewLocation - CurrPos).SizeSquared();
			if (DistanceToEffectSqr < LODDistanceSqr)
			{
				LODDistanceSqr = DistanceToEffectSqr;
			}
		}
		OwnerMinDistanceToCameraParam.SetValue(FMath::Sqrt(LODDistanceSqr));
	}

	int32 NumAlive = 0;
	for (int32 i = 0; i < Emitters.Num(); i++)
	{
		int32 NumParticles = Emitters[i]->GetNumParticles();
		if (!Emitters[i]->IsComplete())
		{
			NumAlive++;
		}
		ParameterNumParticleBindings[i].SetValue(NumParticles);
	}
	SystemNumEmittersParam.SetValue(Emitters.Num());
	SystemNumEmittersAliveParam.SetValue(NumAlive);

	check(World);
	float SafeTimeSinceRendererd = FMath::Max(0.0f, World->GetTimeSeconds() - Component->LastRenderTime - GLastRenderTimeSafetyBias);
	SystemTimeSinceRenderedParam.SetValue(SafeTimeSinceRendererd);

	
	Component->GetOverrideParameters().Tick();
	InstanceParameters.Tick();
	InstanceParameters.MarkParametersDirty();
}

#if WITH_EDITORONLY_DATA

bool FNiagaraSystemInstance::UsesEmitter(const UNiagaraEmitter* Emitter)const
{
	if (GetSystem())
	{
		for (FNiagaraEmitterHandle EmitterHandle : GetSystem()->GetEmitterHandles())
		{
			if (Emitter == EmitterHandle.GetSource() || Emitter == EmitterHandle.GetInstance())
			{
				return true;
			}
		}
	}
	return false;
}

bool FNiagaraSystemInstance::UsesScript(const UNiagaraScript* Script)const
{
	if (GetSystem())
	{
		for (FNiagaraEmitterHandle EmitterHandle : GetSystem()->GetEmitterHandles())
		{
			if ((EmitterHandle.GetSource() && EmitterHandle.GetSource()->UsesScript(Script)) || (EmitterHandle.GetInstance() && EmitterHandle.GetInstance()->UsesScript(Script)))
			{
				return true;
			}
		}
	}
	return false;
}

// bool FNiagaraSystemInstance::UsesDataInterface(UNiagaraDataInterface* Interface)
// {
// 
// }

bool FNiagaraSystemInstance::UsesCollection(const UNiagaraParameterCollection* Collection)const
{
	if (UNiagaraSystem* System = GetSystem())
	{
		if (System->UsesCollection(Collection))
		{
			return true;
		}
	}
	return false;
}

#endif

void FNiagaraSystemInstance::InitEmitters()
{
	if (Component)
	{
		Component->MarkRenderStateDirty();
	}

	// Just in case this ends up being called more than in Init, we need to 
	// clear out the update proxy of any renderers that will be destroyed when Emitters.Empty occurs..
	TArray<NiagaraRenderer*> NewRenderers;
	UpdateProxy(NewRenderers);

	// Clear out the System renderer from the simulation.
	for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
	{
		Simulation->ClearRenderer();
	}

	Emitters.Empty();
	if (GetSystem() != nullptr)
	{
		const TArray<FNiagaraEmitterHandle>& EmitterHandles = GetSystem()->GetEmitterHandles();
		for (int32 EmitterIdx=0; EmitterIdx < GetSystem()->GetEmitterHandles().Num(); ++EmitterIdx)
		{
			const FNiagaraEmitterHandle& EmitterHandle = EmitterHandles[EmitterIdx];
			TSharedRef<FNiagaraEmitterInstance> Sim = MakeShareable(new FNiagaraEmitterInstance(this));
			Sim->Init(EmitterIdx, IDName);
			Emitters.Add(Sim);
		}

		for (TSharedRef<FNiagaraEmitterInstance> Simulation : Emitters)
		{
			Simulation->PostInitSimulation();
		}

		bDataInterfacesNeedInit = true;
	}
}

void FNiagaraSystemInstance::UpdateRenderModules(ERHIFeatureLevel::Type InFeatureLevel, TArray<NiagaraRenderer*>& OutNewRenderers, TArray<NiagaraRenderer*>& OutOldRenderers)
{
	for (TSharedPtr<FNiagaraEmitterInstance> Sim : Emitters)
	{
		Sim->UpdateEmitterRenderer(InFeatureLevel, OutNewRenderers, OutOldRenderers);
	}
}

void FNiagaraSystemInstance::UpdateProxy(TArray<NiagaraRenderer*>& InRenderers)
{
	if (!GetSystem() || !Component)
	{
		return;
	}

	FNiagaraSceneProxy *NiagaraProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	if (NiagaraProxy)
	{
		if (Component != nullptr && Component->GetWorld() != nullptr)
		{
			// Tell the scene proxy on the render thread to update its System renderers.
			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FChangeNiagaraRenderModule,
				FNiagaraSceneProxy*, InProxy, NiagaraProxy,
				TArray<NiagaraRenderer*>, InRendererArray, InRenderers,
				{
					InProxy->UpdateEmitterRenderers(InRendererArray);
				}
			);
		}
	}
}

void FNiagaraSystemInstance::ComponentTick(float DeltaSeconds)
{
	if (bError)
	{
		return;
	}

	TSharedPtr<FNiagaraSystemSimulation, ESPMode::ThreadSafe> Sim = GetSystemSimulation();
	check(Sim.IsValid());
	check(IsInGameThread());
	check(bSolo);
	check(Component);

	PreSimulateTick(DeltaSeconds);

	TSharedPtr<FNiagaraSystemSimulation, ESPMode::ThreadSafe> SystemSim = GetSystemSimulation();
	SystemSim->TickSolo(this);

	if(!HandleCompletion())
	{
		PostSimulateTick(DeltaSeconds);
	}
}

bool FNiagaraSystemInstance::HandleCompletion()
{
	bool bEmittersCompleteOrDisabled = true;
	bHasTickingEmitters = false;
	for (TSharedRef<FNiagaraEmitterInstance>&it : Emitters)
	{
		FNiagaraEmitterInstance& Inst = *it;
		bEmittersCompleteOrDisabled &= Inst.HandleCompletion();
		bHasTickingEmitters |= Inst.ShouldTick();
	}

	if (IsComplete() || bEmittersCompleteOrDisabled)
	{
		Complete();
		return true;
	}
	return false;
}

void FNiagaraSystemInstance::PreSimulateTick(float DeltaSeconds)
{
	TickInstanceParameters(DeltaSeconds);
}

void FNiagaraSystemInstance::PostSimulateTick(float DeltaSeconds)
{
	if (IsComplete() || !bHasTickingEmitters || GetSystem() == nullptr || Component == nullptr || DeltaSeconds < SMALL_NUMBER || bError)
	{
		return;
	}

	// pass the constants down to the emitter
	// TODO: should probably just pass a pointer to the table
	for (TPair<FNiagaraDataSetID, FNiagaraDataSet>& EventSetPair : ExternalEvents)
	{
		EventSetPair.Value.Tick();
	}

	if (bDataInterfacesNeedInit)
	{
		InitDataInterfaces();
		// We may have entered into an error state as a result of InitDataInterfaces.
		if (bError)
		{
			return;
		}
	}

	//TODO: Now we're batching in the world manager we can store these in big blocks across systems and update all instance data in one virtual call?
	TickDataInterfaces(DeltaSeconds, false);

	// We may have entered into an error state as a result of TickDataInterfaces.
	if (bError)
	{
		return;
	}

	// need to run this serially, as PreTick will initialize data interfaces, which can't be done in parallel
	for (int32 EmitterIdx = 0; EmitterIdx < Emitters.Num(); EmitterIdx++)
	{
		FNiagaraEmitterInstance& Inst = Emitters[EmitterIdx].Get();		
		const UNiagaraEmitter *Props = Inst.GetEmitterHandle().GetInstance();
		check(Props);
		Inst.PreTick();
	}

	// now tick all emitters
	for (int32 EmitterIdx = 0; EmitterIdx < Emitters.Num(); EmitterIdx++)
	{
		FNiagaraEmitterInstance& Inst = Emitters[EmitterIdx].Get();
		Inst.Tick(DeltaSeconds);
	}

	TickDataInterfaces(DeltaSeconds, true);

	Age += DeltaSeconds;
}

#if WITH_EDITORONLY_DATA
bool FNiagaraSystemInstance::GetIsolateEnabled() const
{
	UNiagaraSystem* System = GetSystem();
	if (System)
	{
		return System->GetIsolateEnabled();
	}
	return false;
}
#endif

void FNiagaraSystemInstance::ReinitializeDataInterfaces()
{
	// Update next tick
	bDataInterfacesNeedInit = true;
}

void FNiagaraSystemInstance::DestroyDataInterfaceInstanceData()
{
	for (TPair<TWeakObjectPtr<UNiagaraDataInterface>, int32>& Pair : DataInterfaceInstanceDataOffsets)
	{
		if (UNiagaraDataInterface* Interface = Pair.Key.Get())
		{
			Interface->DestroyPerInstanceData(&DataInterfaceInstanceData[Pair.Value], this);
		}
	}
	DataInterfaceInstanceDataOffsets.Empty();
	DataInterfaceInstanceData.Empty();
}

TSharedPtr<FNiagaraEmitterInstance> FNiagaraSystemInstance::GetSimulationForHandle(const FNiagaraEmitterHandle& EmitterHandle)
{
	for (TSharedPtr<FNiagaraEmitterInstance> Sim : Emitters)
	{
		if(Sim->GetEmitterHandle().GetId() == EmitterHandle.GetId())
		{
			return Sim;
		}
	}
	return nullptr;
}

UNiagaraSystem* FNiagaraSystemInstance::GetSystem()const
{
	return GetSystemSimulation()->GetSystem(); 
}

FNiagaraDataSet* FNiagaraSystemInstance::GetDataSet(FNiagaraDataSetID SetID, FName EmitterName)
{
	if (EmitterName == NAME_None)
	{
		if (FNiagaraDataSet* ExternalSet = ExternalEvents.Find(SetID))
		{
			return ExternalSet;
		}
	}
	for (TSharedPtr<FNiagaraEmitterInstance> Emitter : Emitters)
	{
		check(Emitter.IsValid());
		if (!Emitter->IsComplete())
		{
			if (Emitter->GetEmitterHandle().GetIdName() == EmitterName)
			{
				return Emitter->GetDataSet(SetID);
			}
		}
	}

	return NULL;
}

FNiagaraSystemInstance::FOnInitialized& FNiagaraSystemInstance::OnInitialized()
{
	return OnInitializedDelegate;
}

FNiagaraSystemInstance::FOnComplete& FNiagaraSystemInstance::OnComplete()
{
	return OnCompleteDelegate;
}

#if WITH_EDITOR
FNiagaraSystemInstance::FOnReset& FNiagaraSystemInstance::OnReset()
{
	return OnResetDelegate;
}

FNiagaraSystemInstance::FOnDestroyed& FNiagaraSystemInstance::OnDestroyed()
{
	return OnDestroyedDelegate;
}
#endif
