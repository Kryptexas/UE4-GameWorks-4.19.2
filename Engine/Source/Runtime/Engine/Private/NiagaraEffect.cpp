// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/NiagaraEffect.h"



UNiagaraEffect::UNiagaraEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}



TSharedPtr<FNiagaraSimulation> UNiagaraEffect::AddEmitter()
{
	FNiagaraEmitterProperties Props;
	EmitterProps.Add(Props);

	TSharedPtr<FNiagaraSimulation> Sim = MakeShareable(new FNiagaraSimulation(&EmitterProps.Top()));

	Sim->SetRenderModuleType(RMT_Sprites, Component->GetWorld()->FeatureLevel);
	Emitters.Add(Sim);

	FNiagaraSceneProxy *SceneProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	SceneProxy->UpdateEffectRenderers(this);

	return Sim;
}




void UNiagaraEffect::PostLoad()
{
	Super::PostLoad();

	Emitters.Empty();

	for (FNiagaraEmitterProperties &Props : EmitterProps)
	{
		FNiagaraSimulation *Sim = new FNiagaraSimulation(&Props);
		Emitters.Add(MakeShareable(Sim));
	}
}
