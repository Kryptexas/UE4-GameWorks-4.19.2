// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/NiagaraEffect.h"



UNiagaraEffect::UNiagaraEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}



TSharedPtr<FNiagaraSimulation> UNiagaraEffect::AddEmitter()
{
	TSharedPtr<FNiagaraSimulation> Sim = MakeShareable(new FNiagaraSimulation());

	Sim->SetRenderModuleType(RMT_Sprites, Component->GetWorld()->FeatureLevel);
	Emitters.Add(Sim);

	FNiagaraSceneProxy *SceneProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	SceneProxy->UpdateEffectRenderers(this);

	FNiagaraEmitterProperties Props;
	EmitterProps.Add(Props);

	return Sim;
}
