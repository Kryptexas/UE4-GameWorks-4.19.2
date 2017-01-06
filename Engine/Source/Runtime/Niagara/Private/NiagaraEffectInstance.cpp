// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectInstance.h"
#include "NiagaraConstants.h"


FNiagaraEffectInstance::FNiagaraEffectInstance(UNiagaraComponent* InComponent)
	: Component(InComponent)
	, Effect(nullptr)
	, AgeUpdateMode(EAgeUpdateMode::TickDeltaTime)
	, Age(0.0f)
	, DesiredAge(0.0f)
	, SeekDelta(1 / 30.0f)
	, bResetPending(false)
	, EffectPositionParam(SYS_PARAM_EFFECT_POSITION)
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
{
}

void FNiagaraEffectInstance::Init(UNiagaraEffect* InEffect)
{
	Effect = InEffect;
	InitEmitters();
	RequestResetSimulation();
	OnInitializedDelegate.Broadcast();
}

void FNiagaraEffectInstance::RequestResetSimulation()
{
	bResetPending = true;
}

void FNiagaraEffectInstance::InvalidateComponentBindings()
{
	// This must happen after the update of the parameter bindings on the instance or else we will have cached the wrong variables.
	UpdateParameterBindingInstances();
}


void FNiagaraEffectInstance::ResetInternal()
{
	Age = 0;

	ExternalInstanceParameters.Set(Effect->GetEffectScript()->Parameters);


	for (TSharedRef<FNiagaraSimulation> Simulation : Emitters)
	{
		Simulation->ResetSimulation();
	}

	for (TSharedRef<FNiagaraSimulation> Simulation : Emitters)
	{
		Simulation->PostResetSimulation();
	}

	UpdateParameterBindingInstances();
	
	FlushRenderingCommands();

	if (Component != nullptr && Component->GetWorld() != nullptr)
	{
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			FChangeNiagaraRenderModule,
			FNiagaraEffectInstance*, InEffect, this,
			UNiagaraComponent*, InComponent, Component,
			{
				InEffect->UpdateRenderModules(InComponent->GetWorld()->FeatureLevel);
				InEffect->UpdateProxy();
			}
		);
	}

	bResetPending = false;

#if WITH_EDITOR
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
	// TODO: Logging on invalid bindings.
	ParameterBindingInstances.Empty();
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

				if (SimulationParameter != nullptr && EffectInstanceParameter->GetType() == SimulationParameter->GetType())
				{
					ParameterBindingInstances.Add(FNiagaraParameterBindingInstance(*EffectInstanceParameter, *SimulationParameter));
				}
			}
		}
	}

}


void FNiagaraEffectInstance::InitEmitters()
{
	Emitters.Empty();
	if (Effect != nullptr)
	{
		for (const FNiagaraEmitterHandle& EmitterHandle : Effect->GetEmitterHandles())
		{
			TSharedRef<FNiagaraSimulation> Sim = MakeShareable(new FNiagaraSimulation(this));
			Sim->Init(EmitterHandle, IDName);
			Emitters.Add(Sim);
		}
	}
}

void FNiagaraEffectInstance::UpdateRenderModules(ERHIFeatureLevel::Type InFeatureLevel)
{
	for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
	{
		Sim->UpdateEffectRenderer(InFeatureLevel);
	}
}

void FNiagaraEffectInstance::UpdateProxy()
{
	FNiagaraSceneProxy *NiagaraProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	if (NiagaraProxy)
	{
		NiagaraProxy->UpdateEffectRenderers(this);
	}
}

void FNiagaraEffectInstance::SetAgeUpdateMode(EAgeUpdateMode InAgeUpdateMode)
{
	AgeUpdateMode = InAgeUpdateMode;
}

void FNiagaraEffectInstance::SetDesiredAge(float InDesiredAge)
{
	DesiredAge = InDesiredAge;
}

void FNiagaraEffectInstance::SetSeekDelta(float InSeekDelta)
{ 
	SeekDelta = InSeekDelta;
}

void FNiagaraEffectInstance::Tick(float DeltaSeconds)
{
	if (AgeUpdateMode == EAgeUpdateMode::TickDeltaTime)
	{
		if (bResetPending)
		{
			ResetInternal();
		}
		TickInternal(DeltaSeconds);
	}
	else
	{
		if (bResetPending || DesiredAge < Age)
		{
			ResetInternal();
		}

		if (DesiredAge == Age)
		{
			TickInternal(0);
		}
		else
		{
			while (Age + SeekDelta < DesiredAge)
			{
				TickInternal(SeekDelta);
			}
			TickInternal(DesiredAge - Age);
		}
	}
}

void FNiagaraEffectInstance::TickInternal(float DeltaSeconds)
{
	// pass the constants down to the emitter
	// TODO: should probably just pass a pointer to the table
	EffectBounds.Init();

	for (TPair<FNiagaraDataSetID, FNiagaraDataSet>& EventSetPair : ExternalEvents)
	{
		EventSetPair.Value.Tick();
	}

	//Set effect params. Would be nice to just have a single struct we could fill and push into the param set.
	FTransform ComponentTrans = Component->GetComponentTransform();
	EffectPositionParam.SetValue(ComponentTrans.GetLocation());
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
	InstanceParameters.SetOrAdd(EffectXAxisParam);
	InstanceParameters.SetOrAdd(EffectYAxisParam);
	InstanceParameters.SetOrAdd(EffectZAxisParam);
	InstanceParameters.SetOrAdd(EffectAgeParam);
	InstanceParameters.SetOrAdd(EffectLocalToWorldParam);
	InstanceParameters.SetOrAdd(EffectWorldToLocalParam);
	InstanceParameters.SetOrAdd(EffectLocalToWorldTranspParam);
	InstanceParameters.SetOrAdd(EffectWorldToLocalTranspParam);

	for (FNiagaraParameterBindingInstance& ParameterBindingInstance : ParameterBindingInstances)
	{
		ParameterBindingInstance.Tick();
	}

	for (TSharedRef<FNiagaraSimulation>&it : Emitters)
	{
		const UNiagaraEmitterProperties *Props = it->GetEmitterHandle().GetInstance();
		check(Props);

		float Duration = Props->EndTime - Props->StartTime;
		float LoopedStartTime = Props->StartTime + Duration*it->GetLoopCount();
		float LoopedEndTime = Props->EndTime + Duration*it->GetLoopCount();

		// manage emitter lifetime
		//
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

		//TODO - Handle constants better. Like waaaay better.


		it->PreTick();
	}

	for (TSharedRef<FNiagaraSimulation>&it : Emitters)
	{
		if (it->GetTickState() != NTS_Dead && it->GetTickState() != NTS_Suspended)
		{
			it->Tick(DeltaSeconds);
		}

		if (it->GetEffectRenderer() != nullptr)
		{
			EffectBounds += it->GetEffectRenderer()->GetBounds();
		}
	}

	Age += DeltaSeconds;
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
#endif