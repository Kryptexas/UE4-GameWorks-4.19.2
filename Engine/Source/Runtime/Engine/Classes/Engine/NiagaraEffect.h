// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraSimulation.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraEffect.generated.h"

UCLASS(MinimalAPI)
class UNiagaraEffect : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	ENGINE_API TSharedPtr<FNiagaraSimulation> AddEmitter();


	void Init(UNiagaraComponent *InComponent)
	{
		Component = InComponent;
	}



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

	/** Local constant table. */
	FNiagaraConstantMap Constants;

	//UPROPERTY()
	TArray< TSharedPtr<FNiagaraSimulation> > Emitters;
	UNiagaraComponent *GetComponent() { return Component; }

	virtual void PostLoad() override; 

private:
	UNiagaraComponent *Component;

	UPROPERTY()
	TArray<FNiagaraEmitterProperties>EmitterProps;
};
