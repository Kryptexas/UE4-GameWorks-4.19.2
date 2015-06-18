// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#include "NiagaraPrivate.h"
#include "NiagaraEffect.h"



UNiagaraEffect::UNiagaraEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}



UNiagaraEmitterProperties *UNiagaraEffect::AddEmitterProperties(UNiagaraEmitterProperties *Props)
{
	if (Props == nullptr)
	{
		Props = NewObject<UNiagaraEmitterProperties>(this);
	}
	EmitterProps.Add(Props);
	return Props;
}

void UNiagaraEffect::DeleteEmitterProperties(UNiagaraEmitterProperties *Props)
{
	EmitterProps.Remove(Props);
}


void UNiagaraEffect::CreateEffectRendererProps(TSharedPtr<FNiagaraSimulation> Sim)
{
	UClass *RendererProps = Sim->GetEffectRenderer()->GetPropertiesClass();
	if (RendererProps)
	{
		Sim->GetProperties()->RendererProperties = NewObject<UNiagaraEffectRendererProperties>(this, RendererProps);
	}
	else
	{
		Sim->GetProperties()->RendererProperties = nullptr;
	}
}

void UNiagaraEffect::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	if (Ar.IsLoading() && Ar.UE4Ver() < VER_UE4_NIAGARA_DATA_OBJECT_DEV_UI_FIX)
	{
		int32 NumEmitters = EmitterPropsSerialized_DEPRECATED.Num();
		for (int32 i = 0; i < NumEmitters; ++i)
		{
			UNiagaraEmitterProperties* NewProps = NewObject<UNiagaraEmitterProperties>(this);
			NewProps->InitFromOldStruct(EmitterPropsSerialized_DEPRECATED[i]);
			AddEmitterProperties(NewProps);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

TSharedPtr<FNiagaraSimulation> FNiagaraEffectInstance::AddEmitter(UNiagaraEmitterProperties *Properties)
{
	FNiagaraSimulation *SimPtr = new FNiagaraSimulation(Properties, Effect);
	TSharedPtr<FNiagaraSimulation> Sim = MakeShareable(SimPtr);
	Sim->SetRenderModuleType(Properties->RenderModuleType, Component->GetWorld()->FeatureLevel);
	Emitters.Add(Sim);
	FNiagaraSceneProxy *SceneProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	SceneProxy->UpdateEffectRenderers(this);

	return Sim;
}

void FNiagaraEffectInstance::DeleteEmitter(TSharedPtr<FNiagaraSimulation> Emitter)
{
	Emitters.Remove(Emitter);
	FNiagaraSceneProxy *SceneProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	SceneProxy->UpdateEffectRenderers(this);
}

void FNiagaraEffectInstance::Tick(float DeltaSeconds)
{
	Constants.SetOrAdd(FName(TEXT("EffectGrid")), VolumeGrid);

	// pass the constants down to the emitter
	// TODO: should probably just pass a pointer to the table
	EffectBounds.Init();

	for (TSharedPtr<FNiagaraSimulation>&it : Emitters)
	{
		UNiagaraEmitterProperties *Props = it->GetProperties().Get();
		check(Props);

		int Duration = Props->EndTime - Props->StartTime;
		int LoopedStartTime = Props->StartTime + Duration*it->GetLoopCount();
		int LoopedEndTime = Props->EndTime + Duration*it->GetLoopCount();

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

		if (it->GetTickState() != NTS_Dead && it->GetTickState() != NTS_Suspended)
		{
			//TODO - Handle constants better. Like waaaay better.
			it->SetConstants(Constants);
			it->GetConstants().Merge(it->GetProperties()->SpawnScriptProps.ExternalConstants);
			it->GetConstants().Merge(it->GetProperties()->UpdateScriptProps.ExternalConstants);
			it->Tick(DeltaSeconds);
		}

		EffectBounds += it->GetEffectRenderer()->GetBounds();
	}

	Age += DeltaSeconds;
}

void FNiagaraEffectInstance::RenderModuleupdate()
{
	FNiagaraSceneProxy *NiagaraProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	if (NiagaraProxy)
	{
		NiagaraProxy->UpdateEffectRenderers(this);
	}
}


void FNiagaraEffectInstance::InitEmitters(UNiagaraEffect *InAsset)
{
	check(InAsset);
	for (int i = 0; i < InAsset->GetNumEmitters(); i++)
	{
		UNiagaraEmitterProperties *Props = InAsset->GetEmitterProperties(i);
		FNiagaraSimulation *Sim = new FNiagaraSimulation(Props, InAsset);
		Emitters.Add(MakeShareable(Sim));
	}
}
