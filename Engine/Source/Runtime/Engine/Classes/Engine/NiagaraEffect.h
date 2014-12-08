// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraSimulation.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraEffect.generated.h"

UCLASS(MinimalAPI)
class UNiagaraEffect : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	ENGINE_API FNiagaraEmitterProperties &AddEmitterProperties();

	void Init(UNiagaraComponent *InComponent)
	{
		Component = InComponent;
	}


	TArray<FNiagaraEmitterProperties> &GetEmitterProperties() {
		return EmitterProps;
	};

	virtual void PostLoad() override; 

private:
	UNiagaraComponent *Component;

	UPROPERTY()
	TArray<FNiagaraEmitterProperties>EmitterProps;
};






class FNiagaraEffectInstance
{
public:
	explicit FNiagaraEffectInstance(UNiagaraEffect *InAsset, UNiagaraComponent *InComponent)
	{
		Component = InComponent;
		for (int32 i = 0; i < InAsset->GetEmitterProperties().Num(); i++)
		{
			FNiagaraSimulation *Sim = new FNiagaraSimulation(&InAsset->GetEmitterProperties()[i], Component->GetWorld()->FeatureLevel);
			Emitters.Add(MakeShareable(Sim));
		}
		InitRenderModules(Component->GetWorld()->FeatureLevel);
	}

	explicit FNiagaraEffectInstance(UNiagaraEffect *InAsset)
		: Component(nullptr)
	{
		for (FNiagaraEmitterProperties &Props : InAsset->GetEmitterProperties())
		{
			FNiagaraSimulation *Sim = new FNiagaraSimulation(&Props);
			Emitters.Add(MakeShareable(Sim));
		}
	}


	void InitRenderModules(ERHIFeatureLevel::Type InFeatureLevel)
	{
		for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
		{
			Sim->SetRenderModuleType(Sim->GetProperties()->RenderModuleType, InFeatureLevel);
		}
	}

	void Init(UNiagaraComponent *InComponent)
	{
		Component = InComponent;
		//InitRenderModules(Component->GetWorld()->FeatureLevel);
	}

	ENGINE_API void AddEmitter(FNiagaraEmitterProperties *Properties);


	void SetConstant(FName ConstantName, const float Value)
	{
		Constants.SetOrAdd(ConstantName, Value);
	}

	void SetConstant(FName ConstantName, const FVector4& Value)
	{
		Constants.SetOrAdd(ConstantName, Value);
	}

	void SetConstant(FName ConstantName, const FMatrix& Value)
	{
		Constants.SetOrAdd(ConstantName, Value);
	}


	void Tick(float DeltaSeconds)
	{
		// pass the constants down to the emitter
		// TODO: should probably just pass a pointer to the table
		for (TSharedPtr<FNiagaraSimulation>&it : Emitters)
		{
			it->SetConstants(Constants);
			it->GetConstants().Merge(it->GetProperties()->ExternalConstants);
			it->Tick(DeltaSeconds);
		}
	}

	void RenderModuleupdate()
	{
		FNiagaraSceneProxy *NiagaraProxy = static_cast<FNiagaraSceneProxy*>(Component->SceneProxy);
		NiagaraProxy->UpdateEffectRenderers(this);
	}

	FNiagaraSimulation *GetEmitter(uint32 idx)
	{
		return Emitters[idx].Get();
	}


	UNiagaraComponent *GetComponent() { return Component; }

	TArray< TSharedPtr<FNiagaraSimulation> > &GetEmitters()	{ return Emitters; }

private:
	UNiagaraComponent *Component;

	/** Local constant table. */
	FNiagaraConstantMap Constants;

	TArray< TSharedPtr<FNiagaraSimulation> > Emitters;


};