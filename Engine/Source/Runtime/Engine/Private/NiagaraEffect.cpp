// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Engine/NiagaraEffect.h"



UNiagaraEffect::UNiagaraEffect(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}



FNiagaraEmitterProperties &UNiagaraEffect::AddEmitterProperties()
{
	FNiagaraEmitterProperties Props;
	EmitterProps.Add(Props);
	return EmitterProps.Top();
}




void UNiagaraEffect::PostLoad()
{
	Super::PostLoad();
}





void FNiagaraEffectInstance::AddEmitter(FNiagaraEmitterProperties *Properties)
{
	TSharedPtr<FNiagaraSimulation> Sim = MakeShareable(new FNiagaraSimulation(Properties));

	Sim->SetRenderModuleType(RMT_Sprites, Component->GetWorld()->FeatureLevel);
	Emitters.Add(Sim);
	FNiagaraSceneProxy *SceneProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
	SceneProxy->UpdateEffectRenderers(this);

}