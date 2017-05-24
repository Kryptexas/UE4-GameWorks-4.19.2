// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectInstance.h"
#include "NiagaraConstants.h"
#include "NiagaraCommon.h"
#include "NiagaraDataInterface.h"
#include "NiagaraStats.h"
#include "Async/ParallelFor.h"

DECLARE_CYCLE_STAT(TEXT("Parallel Tick"), STAT_NiagaraParallelTick, STATGROUP_Niagara);


/** Whether to allow emitters to tick in parallel */
static int32 GbNiagaraParallelEmitterTick = 1;
static FAutoConsoleVariableRef CVarParallelEmitterTick(
	TEXT("niagara.ParallelEmitterTick"),
	GbNiagaraParallelEmitterTick,
	TEXT("Whether to tick individual emitters in an effect in parallel"),
	ECVF_Default
	);
 


FNiagaraEffectInstance::FNiagaraEffectInstance(UNiagaraComponent* InComponent)
	: Component(InComponent)
	, Effect(nullptr)
	, AgeUpdateMode(EAgeUpdateMode::TickDeltaTime)
	, Age(0.0f)
	, DesiredAge(0.0f)
	, SeekDelta(1 / 30.0f)
	, bSuppressSpawning(false)
	, bResetPending(false)
	, bReInitPending(false)
	, EffectPositionParam(SYS_PARAM_EFFECT_POSITION)
	, EffectVelocityParam(SYS_PARAM_EFFECT_VELOCITY)
	, EffectAgeParam(SYS_PARAM_EFFECT_AGE)
	, EffectXAxisParam(SYS_PARAM_EFFECT_X_AXIS)
	, EffectYAxisParam(SYS_PARAM_EFFECT_Y_AXIS)
	, EffectZAxisParam(SYS_PARAM_EFFECT_Z_AXIS)
	, EffectLocalToWorldParam(SYS_PARAM_EFFECT_LOCAL_TO_WORLD)
	, EffectWorldToLocalParam(SYS_PARAM_EFFECT_WORLD_TO_LOCAL)
	, EffectLocalToWorldTranspParam(SYS_PARAM_EFFECT_LOCAL_TO_WORLD_TRANSPOSED)
	, EffectWorldToLocalTranspParam(SYS_PARAM_EFFECT_WORLD_TO_LOCAL_TRANSPOSED)
	, ID(FGuid::NewGuid())
	, IDName(*ID.ToString())
	, bDataInterfacesNeedInit(true)
{
	EffectBounds.Init();
}

void FNiagaraEffectInstance::Init(UNiagaraEffect* InEffect, bool bForceReset)
{
	Effect = InEffect;
	InitEmitters();
	Reset(bForceReset ? EResetMode::ImmediateReInit : EResetMode::DeferredReInit);
	OnInitializedDelegate.Broadcast();
}

FNiagaraEffectInstance::~FNiagaraEffectInstance()
{
	ClearPerInstanceDataInterfaceCache();

	//UE_LOG(LogNiagara, Warning, TEXT("~FNiagaraEffectInstance %p"), this);

	//FlushRenderingCommands();

	// Clear out the effect renderer from the proxy.
	TArray<NiagaraEffectRenderer*> NewRenderers;
	UpdateProxy(NewRenderers);

	// Clear out the effect renderer from the simulation.
	for (TSharedRef<FNiagaraSimulation> Simulation : Emitters)
	{
		Simulation->ClearRenderer();
	}

#if WITH_EDITOR
	OnDestroyedDelegate.Broadcast();
#endif
}


void FNiagaraEffectInstance::Reset(FNiagaraEffectInstance::EResetMode Mode)
{
	if (Mode == EResetMode::DeferredReset)
	{
		bResetPending = true;
	}
	else if (Mode == EResetMode::ImmediateReset)
	{
		ResetInternal();
	}
	else if (Mode == EResetMode::DeferredReInit)
	{
		bReInitPending = true;
	}
	else if (Mode == EResetMode::ImmediateReInit)
	{
		ReInitInternal();
	}
}


void FNiagaraEffectInstance::InvalidateComponentBindings()
{
	// This must happen after the update of the parameter bindings on the instance or else we will have cached the wrong variables.
	UpdateParameterBindingInstances();
	UpdateDataInterfaceBindingInstances();
	bDataInterfacesNeedInit = true;
}


void FNiagaraEffectInstance::ResetInternal()
{
	Age = 0;
	if (Effect.IsValid() == false || Component.IsValid() == false)
	{
		return;
	}

	for (TSharedRef<FNiagaraSimulation> Simulation : Emitters)
	{
		Simulation->ResetSimulation();
	}

	ExternalInstanceParameters.Set(Effect->GetEffectScript()->Parameters);
	UpdateParameterBindingInstances();

	bSuppressSpawning = false;
	bResetPending = false;
}

void FNiagaraEffectInstance::ReInitInternal()
{
	Age = 0;
	if (Effect.IsValid() == false || Component.IsValid() == false)
	{
		return;
	}

	ExternalInstanceParameters.Set(Effect->GetEffectScript()->Parameters);
	ExternalInstanceDataInterfaces = Effect->GetEffectScript()->DataInterfaceInfo;
	
	for (TSharedRef<FNiagaraSimulation> Simulation : Emitters)
	{
		Simulation->ReInitSimulation();
	}

	for (TSharedRef<FNiagaraSimulation> Simulation : Emitters)
	{
		Simulation->PostResetSimulation();
	}

	UpdateParameterBindingInstances();
	UpdateDataInterfaceBindingInstances();
	InitDataInterfaces(); // We can't wait until tick b/c the data interfaces need to be around when we update render modules.
	FlushRenderingCommands();

	// This gets a little tricky, but we want to delete any renderers that are no longer in use on the rendering thread, but
	// first (to be safe), we want to update the proxy to point to the new renderer objects.

	// Step 1: Recreate the renderers on the simulations, we keep the old and new renderers.
	TArray<NiagaraEffectRenderer*> NewRenderers;
	TArray<NiagaraEffectRenderer*> OldRenderers;

	UpdateRenderModules(Component->GetWorld()->FeatureLevel, NewRenderers, OldRenderers);

	// Step 2: Update the proxy with the new renderers that were created.
	UpdateProxy(NewRenderers);

	// Step 3: Queue up the old renderers for deletion on the render thread.
	for (NiagaraEffectRenderer* Renderer : OldRenderers)
	{
		if (Renderer != nullptr)
		{
			Renderer->Release();
		}
	}

	bSuppressSpawning = false;
	bReInitPending = false;

#if WITH_EDITOR
	//UE_LOG(LogNiagara, Log, TEXT("OnResetInternal %p"), this);
	OnResetDelegate.Broadcast();
#endif

}

FNiagaraVariable* FindParameterById(FNiagaraParameters& Parameters, FGuid ParameterId)
{
	for (FNiagaraVariable& Parameter : Parameters.Parameters)
	{
		if (Parameter.GetId() == ParameterId)
		{
			return &Parameter;
		}
	}
	return nullptr;
}

FNiagaraScriptDataInterfaceInfo* FindDataInterfaceById(TArray<FNiagaraScriptDataInterfaceInfo>& DataInterfaces, FGuid ParameterId)
{
	for (FNiagaraScriptDataInterfaceInfo& DataInterface : DataInterfaces)
	{
		if (DataInterface.Id == ParameterId)
		{
			return &DataInterface;
		}
	}
	return nullptr;
}

TSharedPtr<FNiagaraSimulation> FindSimulationByEmitterId(TArray<TSharedRef<FNiagaraSimulation>>& Simulations, FGuid EmitterId)
{
	for (TSharedRef<FNiagaraSimulation> Simulation : Simulations)
	{
		if (Simulation->GetEmitterHandle().GetId() == EmitterId)
		{
			return Simulation;
		}
	}
	return TSharedPtr<FNiagaraSimulation>();
}

void FNiagaraEffectInstance::UpdateParameterBindingInstances()
{
	if (Effect.IsValid() == false || Component.IsValid() == false)
	{
		return;
	}
	// TODO: Logging on invalid bindings.
	ParameterBindingInstances.Empty(); 
	bDataInterfacesNeedInit = true;

	for (const FNiagaraParameterBinding& ParameterBinding : Effect->GetParameterBindings())
	{
		FNiagaraVariable* EffectInstanceParameter = FindParameterById(ExternalInstanceParameters, ParameterBinding.GetSourceParameterId());
		if (EffectInstanceParameter != nullptr)
		{
			// Check to see if there is a local override.
			FNiagaraVariable* LocalComponentOverride = Component->EffectParameterLocalOverrides.FindByPredicate([&](const FNiagaraVariable& Var)
			{
				return Var.GetId() == EffectInstanceParameter->GetId();
			});

			// If it exists and the parameter matches the source parameter, then use the local instead!
			if (LocalComponentOverride != nullptr)
			{
				if (LocalComponentOverride->GetType() == EffectInstanceParameter->GetType())
				{
					EffectInstanceParameter = LocalComponentOverride;
				}
				else
				{
					UE_LOG(LogNiagara, Warning, TEXT("Component Niagara variable override '%s' does not match types with source! Using source instead."), *EffectInstanceParameter->GetName().ToString());
				}
			}

			TSharedPtr<FNiagaraSimulation> BindingSimulation = FindSimulationByEmitterId(Emitters, ParameterBinding.GetDestinationEmitterId());
			if (BindingSimulation.IsValid())
			{
				FNiagaraVariable* SimulationParameter = FindParameterById(BindingSimulation->GetExternalSpawnConstants(), ParameterBinding.GetDestinationParameterId());
				if (SimulationParameter == nullptr)
				{
					SimulationParameter = FindParameterById(BindingSimulation->GetExternalUpdateConstants(), ParameterBinding.GetDestinationParameterId());
				}
				if (SimulationParameter == nullptr)
				{
					SimulationParameter = FindParameterById(BindingSimulation->GetExternalEventConstants(), ParameterBinding.GetDestinationParameterId());
				}

				if (SimulationParameter != nullptr && EffectInstanceParameter->GetType() == SimulationParameter->GetType())
				{
					ParameterBindingInstances.Add(FNiagaraParameterBindingInstance(*EffectInstanceParameter, *SimulationParameter));
				}
			}
		}
	}
}

// Helper to look through a simulation's data interfaces for a given id.
bool FindMatchingDataInterfaceInSimulation(const FGuid& TargetParamId, TSharedPtr<FNiagaraSimulation>& BindingSimulation, FNiagaraScriptDataInterfaceInfo*& SimulationDataInterface, ENiagaraScriptUsage& ScriptUsage)
{
	SimulationDataInterface = nullptr;
	ScriptUsage = ENiagaraScriptUsage::SpawnScript;

	if (BindingSimulation.IsValid())
	{
		SimulationDataInterface = FindDataInterfaceById(BindingSimulation->GetExternalSpawnDataInterfaceInfo(), TargetParamId);
		ScriptUsage = ENiagaraScriptUsage::SpawnScript;
		if (SimulationDataInterface == nullptr)
		{
			SimulationDataInterface = FindDataInterfaceById(BindingSimulation->GetExternalUpdateDataInterfaceInfo(), TargetParamId);
			ScriptUsage = ENiagaraScriptUsage::UpdateScript;
		}
		if (SimulationDataInterface == nullptr)
		{
			SimulationDataInterface = FindDataInterfaceById(BindingSimulation->GetExternalEventDataInterfaceInfo(), TargetParamId);
			ScriptUsage = ENiagaraScriptUsage::Function;
		}
	}

	return SimulationDataInterface != nullptr;
}

void FNiagaraEffectInstance::ClearPerInstanceDataInterfaceCache()
{
	for(const TSharedPtr<FNiagaraScriptDataInterfaceInfo>& Info: RequiredPerInstanceDataInterfaces)
	{
		//UE_LOG(LogNiagara, Log, TEXT("Clearing %s"), *Info->DataInterface->GetPathName());
		// DataInterface is a UObject. The Component is holding onto a reference to it. If we don't need it anymore, clear it out on the component.
		// Technically it isn't required to clear out the pointer here, but it emphasizes that the gc is going to manage it.
		if (Info->DataInterface != nullptr)
		{
			if (Component.IsValid())
			{
				Component->InstanceDataInterfaces.Remove(Info->DataInterface);
			}
			Info->DataInterface = nullptr;
		}
	}
	// However, since Info is a struct, it isn't manageable unless it is owned in a UProperty or other UObject. That means that we have
	// to delete it ourselves if we don't want it to leak (deletion via clearing out unique ptrs).
	RequiredPerInstanceDataInterfaces.Empty();
}

void FNiagaraEffectInstance::UpdateDataInterfaceBindingInstances()
{
	if (Effect.IsValid() == false)
	{
		return;
	}

	// If the component is invalid, we won't be able to create valid data interfaces below and have them
	// outer'ed properly...
	if (Component.IsValid() == false)
	{
		return;
	}
	
	//UE_LOG(LogNiagara, Warning, TEXT("UpdateDataInterfaceBindingInstances()"));

	ClearPerInstanceDataInterfaceCache();

	//If we clear out the data interface cache above, we need to reset the simulations so that they no longer are potentially holding on to bogus interfaces...
	for (TSharedRef<FNiagaraSimulation>&it : Emitters)
	{
		it->ReInitDataInterfaces();
	}

	//CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

	bDataInterfacesNeedInit = true;

	// This is a traffic routing method. We have multiple simulations that have been instantiated, each with their own needs for data interfaces.
	// They can come from the following locations, from least priority to most:
	// 1) The data interface was specified on the emitter and not overridden by any of the other items on this list. This is usually resolved during compilation and 
	//    needs no additional routing.
	// 2) The data interface may have been set up in the effect and wired to one or more emitters. Each emitter needs to get that master data interface in its simulation instance.
	// 3) The data interface may need to be unique per effect instance due to locally cached data. This can apply to either emitter or effect-level data interfaces.
	//    They are created as needed within this function.
	// 4) The Niagara component may have a local override set, which trumps everything else.

	// TODO: Logging on invalid bindings.
	DataInterfaceBindingInstances.Empty();

	// The effect's binding array is created to handle case 2 above and is part of the "compilation" of the effect. This loop will create the binding
	// instance routing the data to each emitter.
	// (Covers cases 4, partially 3, and 2)
	for (const FNiagaraParameterBinding& DataInterfaceBinding : Effect->GetDataInterfaceBindings())
	{
		FNiagaraScriptDataInterfaceInfo* EffectInstanceDataInterface = FindDataInterfaceById(ExternalInstanceDataInterfaces, DataInterfaceBinding.GetSourceParameterId());
		if (EffectInstanceDataInterface != nullptr)
		{
			// Check to see if there is a local override (case 4 above).
			FNiagaraScriptDataInterfaceInfo* LocalComponentOverride = Component->EffectDataInterfaceLocalOverrides.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
			{
				return Var.Id == EffectInstanceDataInterface->Id;
			});
			
			// If the data interface requires per-instance data but was not locally overridden, create a per-instance required override instead... (The effect version of 3 above)
			if (LocalComponentOverride == nullptr && EffectInstanceDataInterface->DataInterface != nullptr && EffectInstanceDataInterface->DataInterface->RequiresPerInstanceData())
			{
				int32 InsertIdx = RequiredPerInstanceDataInterfaces.Add(TSharedPtr<FNiagaraScriptDataInterfaceInfo>());
				RequiredPerInstanceDataInterfaces[InsertIdx] = MakeShared<FNiagaraScriptDataInterfaceInfo>();
				EffectInstanceDataInterface->CopyTo(RequiredPerInstanceDataInterfaces[InsertIdx].Get(), Component.Get());
				LocalComponentOverride = RequiredPerInstanceDataInterfaces[InsertIdx].Get();
				if (RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface != nullptr)
				{
					Component->InstanceDataInterfaces.AddUnique(RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface);
				}
				//UE_LOG(LogNiagara, Log, TEXT("Created %s"), *LocalComponentOverride->DataInterface->GetPathName());
				

				// In the event that the values of the effect change during runtime, we may still need to synch up our internal copy with the 
				// master copy. We create a binding instance to do this for us each frame. It will occur prior to any subsequent bindings off of our internal copy to 
				// simulations, so the data will flow in the proper graph ordering. Note the final boolean parameter tells the binding to be a 
				// member-level copy and not a raw data interface pointer swap.
				ensure(EffectInstanceDataInterface->DataInterface->IsValidLowLevel());
				ensure(RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface->IsValidLowLevel());
				DataInterfaceBindingInstances.Add(FNiagaraDataInterfaceBindingInstance(*EffectInstanceDataInterface, *RequiredPerInstanceDataInterfaces[InsertIdx], ENiagaraScriptUsage::EffectScript, false));
			}

			// If it exists and the DataInterface matches the source DataInterface, then use the  instead!
			if (LocalComponentOverride != nullptr)
			{
				if (LocalComponentOverride->DataInterface->GetClass() == EffectInstanceDataInterface->DataInterface->GetClass())
				{
					EffectInstanceDataInterface = LocalComponentOverride;
				}
#if WITH_EDITORONLY_DATA
				else
				{
					UE_LOG(LogNiagara, Warning, TEXT("Component Niagara data interface override '%s' does not match types with source! Using source instead."), *EffectInstanceDataInterface->Name.ToString());
				}
#endif
			}

			// Create the binding instance for this pair to continuously update the simulation.
			TSharedPtr<FNiagaraSimulation> BindingSimulation = FindSimulationByEmitterId(Emitters, DataInterfaceBinding.GetDestinationEmitterId());
			FNiagaraScriptDataInterfaceInfo* SimulationDataInterface = nullptr;
			ENiagaraScriptUsage ScriptUsage = ENiagaraScriptUsage::SpawnScript;
			if (FindMatchingDataInterfaceInSimulation(DataInterfaceBinding.GetDestinationParameterId(), BindingSimulation, SimulationDataInterface, ScriptUsage))
			{
				if (SimulationDataInterface != nullptr && SimulationDataInterface->DataInterface != nullptr && 
					EffectInstanceDataInterface->DataInterface->GetClass() == SimulationDataInterface->DataInterface->GetClass())
				{
					ensure(EffectInstanceDataInterface->DataInterface->IsValidLowLevel());
					ensure(SimulationDataInterface->DataInterface->IsValidLowLevel());
					DataInterfaceBindingInstances.Add(FNiagaraDataInterfaceBindingInstance(*EffectInstanceDataInterface, *SimulationDataInterface, ScriptUsage));
				}
			}
		}
	}

	// The above code covers all effect level data interfaces and guarantees an override if required by the data interface. However, each emitter may have its own
	// unique data interface per script type that could potentially require a local override.
	// (This is the other half of case 3)
	for (int32 i = 0; i < Effect->GetNumEmitters(); i++)
	{
		const FNiagaraEmitterHandle& Handle = Effect->GetEmitterHandle(i);
		UNiagaraEmitterProperties* EmitterProps = Handle.GetInstance();
		if (EmitterProps != nullptr)
		{
			// Iterate over the scripts on this emitter...
			const int32 MaxNumSupportedScripts = 3;
			for (int32 ScriptIdx = 0; ScriptIdx < MaxNumSupportedScripts; ScriptIdx++)
			{	
				// TODO: there has to be a cleaner way to iterate the scripts on an effect. Maybe a flat array accessor or visitor pattern with lambda on the emitter?
				UNiagaraScript* Script = nullptr;
				switch (ScriptIdx)
				{
				case 0:
					Script = EmitterProps->SpawnScriptProps.Script;
					break;
				case 1:
					Script = EmitterProps->UpdateScriptProps.Script;
					break;
				case 2:
					Script = EmitterProps->EventHandlerScriptProps.Script;
					break;
				}

				if (Script == nullptr)
				{
					continue;
				}

				// Check out each data interface on the script to see if it needs to have a local override...
				for (FNiagaraScriptDataInterfaceInfo& Info : Script->DataInterfaceInfo)
				{
					if (Info.DataInterface != nullptr && Info.DataInterface->RequiresPerInstanceData())
					{
						// First check to see if the code above already placed an override, if it did then we're good. If not, time to insert one.
						if (nullptr == DataInterfaceBindingInstances.FindByPredicate([&Info](const FNiagaraDataInterfaceBindingInstance& InBindingInst) {
							return InBindingInst.GetDetinationId() == Info.Id; }))
						{
							// Create the local override...
							int32 InsertIdx = RequiredPerInstanceDataInterfaces.Add(TSharedPtr<FNiagaraScriptDataInterfaceInfo>());
							RequiredPerInstanceDataInterfaces[InsertIdx] = MakeShared<FNiagaraScriptDataInterfaceInfo>();
							Info.CopyTo(RequiredPerInstanceDataInterfaces[InsertIdx].Get(), Component.Get());
							if (RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface != nullptr)
							{
								Component->InstanceDataInterfaces.AddUnique(RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface);
							}
							//UE_LOG(LogNiagara, Log, TEXT("Created %s"), *RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface->GetPathName());
							
							// Now generate the binding that will set the data...
							TSharedPtr<FNiagaraSimulation> BindingSimulation = FindSimulationByEmitterId(Emitters, Handle.GetId());
							FNiagaraScriptDataInterfaceInfo* SimulationDataInterface = nullptr;
							ENiagaraScriptUsage ScriptUsage = ENiagaraScriptUsage::SpawnScript;
							if (FindMatchingDataInterfaceInSimulation(Info.Id, BindingSimulation, SimulationDataInterface, ScriptUsage))
							{
								if (SimulationDataInterface != nullptr && SimulationDataInterface->DataInterface != nullptr && 
									RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface->GetClass() == SimulationDataInterface->DataInterface->GetClass())
								{
									ensure(Info.DataInterface->IsValidLowLevel());
									ensure(RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface->IsValidLowLevel());

									// In the event that the values of the emitter change during runtime, we may still need to synch up our internal copy with the 
									// master copy. We create a binding instance to do this for us each frame. It will occur prior to any subsequent bindings off of our internal copy to 
									// simulations, so the data will flow in the proper graph ordering. Note the final boolean parameter tells the binding to be a 
									// member-level copy and not a raw data interface pointer swap.
									DataInterfaceBindingInstances.Add(FNiagaraDataInterfaceBindingInstance(Info,
										*RequiredPerInstanceDataInterfaces[InsertIdx], ScriptUsage, false));

									ensure(RequiredPerInstanceDataInterfaces[InsertIdx]->DataInterface->IsValidLowLevel());
									ensure(SimulationDataInterface->DataInterface->IsValidLowLevel());

									// Now do the binding from our internal copy to the actual simulation instance.
									DataInterfaceBindingInstances.Add(FNiagaraDataInterfaceBindingInstance(*RequiredPerInstanceDataInterfaces[InsertIdx], 
										*SimulationDataInterface, ScriptUsage));
								}
							}
						}
					}
				}
			}
		}
	}

	// just to be safe, in case we initialize data interfaces again before a tick, we will go ahead and tick the bindings  we just created.
	for (FNiagaraDataInterfaceBindingInstance& DataInterfaceBindingInstance : DataInterfaceBindingInstances)
	{
		if (DataInterfaceBindingInstance.Tick())
		{
			for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
			{
				Sim->DirtyExternalEventDataInterfaceInfo();
				Sim->DirtyExternalSpawnDataInterfaceInfo();
				Sim->DirtyExternalUpdateDataInterfaceInfo();
				bDataInterfacesNeedInit = true;
			}
		}
	}
}

void FNiagaraEffectInstance::InitDataInterfaces()
{
	// If either the effect or the component is invalid, it is possible that our cached data interfaces
	// are now bogus and could point to invalid memory. Only the UNiagaraComponent or UNiagaraEffect
	// can hold onto GC references to the DataInterfaces.
	if (Effect.IsValid() == false)
	{
		return;
	}

	if (Component.IsValid() == false)
	{
		return;
	}
	
	PreSimulationTickInterfaces.Empty();
	bDataInterfacesNeedInit = false;

	TArray<UNiagaraDataInterface*> InitializedDataInterfaces;

	// Go through all the assigned data interfaces in this effect and 
	// initialize them once. If they need continued per-tick maintenance, add them to that list.
	// If they fail to initialize, disable the emitter that causes problems.
	for (TSharedRef<FNiagaraSimulation>&it : Emitters)
	{
		bool bOk = true;
		it->ReBindDataInterfaces();

		for (FNiagaraScriptDataInterfaceInfo& NDI : it->GetExternalEventDataInterfaceInfo())
		{
			if (InitializedDataInterfaces.Contains(NDI.DataInterface))
			{
				continue;
			}

			// TODO We should remove the IsValidLowLevel call as soon as we are past GDC...
			// It was added prophylactically to protect against JIRA UE-42400.
			if (NDI.DataInterface != nullptr && NDI.DataInterface->IsValidLowLevel() && NDI.DataInterface->PrepareForSimulation(this))
			{
				InitializedDataInterfaces.Add(NDI.DataInterface);
				if (NDI.DataInterface->HasPreSimulationTick())
				{
					PreSimulationTickInterfaces.AddUnique(NDI.DataInterface);
				}
			}
			else
			{
				bOk = false;
			}
		}
		for (FNiagaraScriptDataInterfaceInfo& NDI : it->GetExternalSpawnDataInterfaceInfo())
		{
			if(InitializedDataInterfaces.Contains(NDI.DataInterface))
			{
				continue;
			}

			// TODO We should remove the IsValidLowLevel call as soon as we are past GDC...
			// It was added prophylactically to protect against JIRA UE-42400.
			if (NDI.DataInterface != nullptr && NDI.DataInterface->IsValidLowLevel() && NDI.DataInterface->PrepareForSimulation(this))
			{
				InitializedDataInterfaces.Add(NDI.DataInterface);
				if (NDI.DataInterface->HasPreSimulationTick())
				{
					PreSimulationTickInterfaces.AddUnique(NDI.DataInterface);
				}
			}
			else
			{
				bOk = false;
			}
		}
		for (FNiagaraScriptDataInterfaceInfo& NDI : it->GetExternalUpdateDataInterfaceInfo())
		{
			if(InitializedDataInterfaces.Contains(NDI.DataInterface))
			{
				continue;
			}

			// TODO We should remove the IsValidLowLevel call as soon as we are past GDC...
			// It was added prophylactically to protect against JIRA UE-42400.
			if (NDI.DataInterface != nullptr && NDI.DataInterface->IsValidLowLevel() && NDI.DataInterface->PrepareForSimulation(this))
			{
				InitializedDataInterfaces.Add(NDI.DataInterface);
				if (NDI.DataInterface->HasPreSimulationTick())
				{
					PreSimulationTickInterfaces.AddUnique(NDI.DataInterface);
				}
			}
			else
			{
				bOk = false;
			}
		}

		if (!bOk)
		{
			UE_LOG(LogNiagara, Error, TEXT("Error initializing data interfaces."));
			it->SetDataInterfacesProperlySetup(false);
		}
		else
		{
			it->SetDataInterfacesProperlySetup(true);
		}
	}
}

void FNiagaraEffectInstance::TickDataInterfaces(float DeltaSeconds)
{
	// These are decided by InitDataInterfaces.
	bool bReInitDataInterfaces = false;
	for (TWeakObjectPtr<UNiagaraDataInterface>& NDI : PreSimulationTickInterfaces)
	{
		if (NDI.IsValid())
		{
			bReInitDataInterfaces |= NDI->PreSimulationTick(this, DeltaSeconds);
		}
	}
	
	//This should ideally really only happen at edit time.
	if (bReInitDataInterfaces)
	{
		InitDataInterfaces();	
	}
}

void FNiagaraEffectInstance::InitEmitters()
{
	// Just in case this ends up being called more than in Init, we need to 
	// clear out the update proxy of any renderers that will be destroyed when Emitters.Empty occurs..
	TArray<NiagaraEffectRenderer*> NewRenderers;
	UpdateProxy(NewRenderers);

	Emitters.Empty();
	if (Effect != nullptr)
	{
		for (const FNiagaraEmitterHandle& EmitterHandle : Effect->GetEmitterHandles())
		{
			TSharedRef<FNiagaraSimulation> Sim = MakeShareable(new FNiagaraSimulation(this));
			Sim->Init(EmitterHandle, IDName);
			Emitters.Add(Sim);
		}
		bDataInterfacesNeedInit = true;
	}

}

void FNiagaraEffectInstance::UpdateRenderModules(ERHIFeatureLevel::Type InFeatureLevel, TArray<NiagaraEffectRenderer*>& OutNewRenderers, TArray<NiagaraEffectRenderer*>& OutOldRenderers)
{
	for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
	{
		Sim->UpdateEffectRenderer(InFeatureLevel, OutOldRenderers);
		OutNewRenderers.Add(Sim->GetEffectRenderer());
	}
}

void FNiagaraEffectInstance::UpdateProxy(TArray<NiagaraEffectRenderer*>& InRenderers)
{
	if (Component.IsValid())
	{
		FNiagaraSceneProxy *NiagaraProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
		if (NiagaraProxy)
		{
			if (Component != nullptr && Component->GetWorld() != nullptr)
			{
				// Tell the scene proxy on the render thread to update its effect renderers.
				ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
					FChangeNiagaraRenderModule,
					FNiagaraSceneProxy*, InProxy, NiagaraProxy,
					TArray<NiagaraEffectRenderer*>, InRendererArray, InRenderers,
					{
						InProxy->UpdateEffectRenderers(InRendererArray);
					}
				);
			}
		}
	}
}

FNiagaraEffectInstance::EAgeUpdateMode FNiagaraEffectInstance::GetAgeUpdateMode() const
{
	return AgeUpdateMode;
}

void FNiagaraEffectInstance::SetAgeUpdateMode(EAgeUpdateMode InAgeUpdateMode)
{
	AgeUpdateMode = InAgeUpdateMode;
}

float FNiagaraEffectInstance::GetDesiredAge() const
{
	return DesiredAge;
}

void FNiagaraEffectInstance::SetDesiredAge(float InDesiredAge)
{
	DesiredAge = InDesiredAge;
}

float FNiagaraEffectInstance::GetSeekDelta() const
{
	return SeekDelta;
}

void FNiagaraEffectInstance::SetSeekDelta(float InSeekDelta)
{ 
	SeekDelta = InSeekDelta;
}

bool FNiagaraEffectInstance::GetSuppressSpawning() const
{
	return bSuppressSpawning;
}

void FNiagaraEffectInstance::SetSuppressSpawning(bool bInSuppressSpawning)
{
	bSuppressSpawning = bInSuppressSpawning;
}

void FNiagaraEffectInstance::Tick(float DeltaSeconds)
{
	if (AgeUpdateMode == EAgeUpdateMode::TickDeltaTime)
	{
		if (bReInitPending)
		{
			ReInitInternal();
		}
		else if (bResetPending)
		{
			ResetInternal();
		}

		TickInternal(DeltaSeconds);
	}
	else
	{
		if (bReInitPending)
		{
			ReInitInternal();
		}
		else if (bResetPending || DesiredAge < Age)
		{
			ResetInternal();
		}

		if (DesiredAge == Age)
		{
			TickInternal(0);
		}
		else
		{
			// Don't seek unless we're more than 2 seek deltas off.
			bool bSeek = Age + (SeekDelta * 2) < DesiredAge;

			// TODO: This maximum should be configurable, and we should not draw the effect until we are caught up.
			static const float MaxSimTime = 1;
			float CurrentSimTime = 0;
			if (bSeek)
			{
				while (Age + SeekDelta < DesiredAge && CurrentSimTime < MaxSimTime)
				{
					TickInternal(SeekDelta);
					CurrentSimTime += SeekDelta;
				}
			}

			if (CurrentSimTime < MaxSimTime)
			{
				TickInternal(DesiredAge - Age);
			}
		}
	}
}

void FNiagaraEffectInstance::ReinitializeDataInterfaces()
{
	// Update next tick
	bDataInterfacesNeedInit = true;
}

void FNiagaraEffectInstance::TickInternal(float DeltaSeconds)
{
	if (Effect.IsValid() == false || Component.IsValid() == false || DeltaSeconds < SMALL_NUMBER)
	{
		return;
	}

	// pass the constants down to the emitter
	// TODO: should probably just pass a pointer to the table
	for (TPair<FNiagaraDataSetID, FNiagaraDataSet>& EventSetPair : ExternalEvents)
	{
		EventSetPair.Value.Tick();
	}

	//Set effect params. Would be nice to just have a single struct we could fill and push into the param set.
	FTransform ComponentTrans = Component->GetComponentTransform();
	FVector OldPos = ComponentTrans.GetLocation();
	if (EffectPositionParam.IsDataAllocated())
	{
		OldPos = *EffectPositionParam.GetValue<FVector>();
	}
	EffectPositionParam.SetValue(ComponentTrans.GetLocation());
	EffectVelocityParam.SetValue( (ComponentTrans.GetLocation() - OldPos) / DeltaSeconds);
	EffectXAxisParam.SetValue(ComponentTrans.GetRotation().GetAxisX());
	EffectYAxisParam.SetValue(ComponentTrans.GetRotation().GetAxisY());
	EffectZAxisParam.SetValue(ComponentTrans.GetRotation().GetAxisZ());
	EffectAgeParam.SetValue(Age);
	FMatrix Transform = ComponentTrans.ToMatrixWithScale();
	FMatrix Inverse = Transform.Inverse();
	FMatrix Transpose = Transform.GetTransposed();
	FMatrix InverseTranspose = Inverse.GetTransposed();

	EffectLocalToWorldParam.SetValue(Transform);
	EffectWorldToLocalParam.SetValue(Inverse);
	EffectLocalToWorldTranspParam.SetValue(Transpose);
	EffectWorldToLocalTranspParam.SetValue(InverseTranspose);

	InstanceParameters.SetOrAdd(EffectPositionParam);
	InstanceParameters.SetOrAdd(EffectVelocityParam);
	InstanceParameters.SetOrAdd(EffectXAxisParam);
	InstanceParameters.SetOrAdd(EffectYAxisParam);
	InstanceParameters.SetOrAdd(EffectZAxisParam);
	InstanceParameters.SetOrAdd(EffectAgeParam);
	InstanceParameters.SetOrAdd(EffectLocalToWorldParam);
	InstanceParameters.SetOrAdd(EffectWorldToLocalParam);
	InstanceParameters.SetOrAdd(EffectLocalToWorldTranspParam);
	InstanceParameters.SetOrAdd(EffectWorldToLocalTranspParam);

	TickEmitterInternalBindings();

	for (FNiagaraParameterBindingInstance& ParameterBindingInstance : ParameterBindingInstances)
	{
		ParameterBindingInstance.Tick();
	}
	
	for (FNiagaraDataInterfaceBindingInstance& DataInterfaceBindingInstance : DataInterfaceBindingInstances)
	{
		if (DataInterfaceBindingInstance.Tick())
		{
			for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
			{
				Sim->DirtyExternalEventDataInterfaceInfo();
				Sim->DirtyExternalSpawnDataInterfaceInfo();
				Sim->DirtyExternalUpdateDataInterfaceInfo();
				bDataInterfacesNeedInit = true;
			}
		}
	}

	if (bDataInterfacesNeedInit)
	{
		InitDataInterfaces();
	}

	TickDataInterfaces(DeltaSeconds);

	// need to run this serially, as PreTick will initialize data interfaces, which can't be done in parallel
	for (TSharedRef<FNiagaraSimulation>&it : Emitters)
	{
		const UNiagaraEmitterProperties *Props = it->GetEmitterHandle().GetInstance();
		check(Props);

		float Duration = Props->EndTime - Props->StartTime;
		float LoopedStartTime = Props->StartTime + Duration*it->GetLoopCount();
		float LoopedEndTime = Props->EndTime + Duration*it->GetLoopCount();

		// manage emitter lifetime
		//
		if (bSuppressSpawning)
		{
			if (it->GetTickState() != NTS_Dead && it->GetTickState() != NTS_Suspended)
			{
				it->SetTickState(NTS_Dieing);
			}
		}
		else
		{
			if ((Props->StartTime == 0.0f && Props->EndTime == 0.0f)
				|| (LoopedStartTime<Age && LoopedEndTime>Age)
				)
			{
				it->SetTickState(NTS_Running);
			}
			else
			{
				// if we're past end time, manage looping; we reset the emitters age constant
				// if it has one
				if (Props->NumLoops > 1 && it->GetLoopCount() < Props->NumLoops)
				{
					it->LoopRestart();
				}
				else
				{
					it->SetTickState(NTS_Dieing);
				}
			}
		}

		//TODO - Handle constants better. Like waaaay better.
		it->PreTick();
	}

	// now tick all emitters in parallel
	{
		SCOPE_CYCLE_COUNTER(STAT_NiagaraParallelTick);

		if (GbNiagaraParallelEmitterTick)
		{
			ParallelFor(Emitters.Num(),
				[&](int32 EmitterIdx)
			{
				TSharedRef<FNiagaraSimulation>&it = Emitters[EmitterIdx];
				if (it->GetTickState() != NTS_Dead && it->GetTickState() != NTS_Suspended)
				{
					it->Tick(DeltaSeconds);
				}
			});
		}
		else
		{
			for (int32 EmitterIdx = 0; EmitterIdx < Emitters.Num(); EmitterIdx++)
			{
				TSharedRef<FNiagaraSimulation>&it = Emitters[EmitterIdx];
				if (it->GetTickState() != NTS_Dead && it->GetTickState() != NTS_Suspended)
				{
					it->Tick(DeltaSeconds);
				}
			}
		}
	}

	Age += DeltaSeconds;
}

void FNiagaraEffectInstance::TickEmitterInternalBindings()
{
	if (Effect.IsValid() == false || Component.IsValid() == false)
	{
		return;
	}

	const TArray<FNiagaraEmitterInternalVariableBinding>& Bindings = Effect->GetEmitterInternalVariableBindings();
	for (const FNiagaraEmitterInternalVariableBinding& Binding : Bindings)
	{
		FNiagaraVariable* Var = Effect->GetEffectScript()->Parameters.FindParameter(Binding.SourceParameterId);
		if (FNiagaraVariable* LocalVar = Component->GetLocalOverrideParameter(Binding.SourceParameterId))
		{
			Var = LocalVar;
		}

		TSharedPtr<FNiagaraSimulation> Simulation;
		for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
		{
			if (Sim->GetEmitterHandle().GetId() == Binding.DestinationEmitterId)
			{
				Simulation = Sim;
				break;
			}
		}

		if (Simulation.IsValid() && Var != nullptr)
		{
			if (Binding.DestinationEmitterVariableName.Equals(FNiagaraSimulation::GetEmitterSpawnRateInternalVarName()))
			{
				float FloatValue = *(Var->GetValue<float>());
				Simulation->SetSpawnRate(FloatValue);
			}
			else if (Binding.DestinationEmitterVariableName.Equals(FNiagaraSimulation::GetEmitterEnabledInternalVarName()))
			{
				bool BoolValue = *((int32*)Var->GetData()) != 0;
				Simulation->SetEnabled(BoolValue);
			}
			else
			{
				// Unknown type
				check(false);
			}
		}
	}
}

FNiagaraSimulation* FNiagaraEffectInstance::GetSimulationForHandle(const FNiagaraEmitterHandle& EmitterHandle)
{
	for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
	{
		if(Sim->GetEmitterHandle().GetId() == EmitterHandle.GetId())
		{
			return Sim.Get();//We really need to sort out the ownership of objects in naigara. Am I free to pass a raw ptr here?
		}
	}
	return nullptr;
}

FNiagaraDataSet* FNiagaraEffectInstance::GetDataSet(FNiagaraDataSetID SetID, FName EmitterName)
{
	if (EmitterName == NAME_None)
	{
		if (FNiagaraDataSet* ExternalSet = ExternalEvents.Find(SetID))
		{
			return ExternalSet;
		}
	}
	for (TSharedPtr<FNiagaraSimulation> Emitter : Emitters)
	{
		check(Emitter.IsValid());
		if (Emitter->IsEnabled())
		{
			if (Emitter->GetEmitterHandle().GetIdName() == EmitterName)
			{
				return Emitter->GetDataSet(SetID);
			}
		}
	}

	return NULL;
}

FNiagaraEffectInstance::FOnInitialized& FNiagaraEffectInstance::OnInitialized()
{
	return OnInitializedDelegate;
}

#if WITH_EDITOR
FNiagaraEffectInstance::FOnReset& FNiagaraEffectInstance::OnReset()
{
	return OnResetDelegate;
}

FNiagaraEffectInstance::FOnDestroyed& FNiagaraEffectInstance::OnDestroyed()
{
	return OnDestroyedDelegate;
}
#endif
