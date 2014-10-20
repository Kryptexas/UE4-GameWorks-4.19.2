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
		//ConstantTable.Empty();
		/*
		// make room for constants
		while (ConstantTable.Num() < 10)
		{
			ConstantTable.Add(FVector4(0.0f, 0.0f, 0.0f, 0.0f));
		}
		*/
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
			/*
			for (int32 i = 0; i < ConstantTable.Num(); i++)
			{
				it->SetConstant(i, ConstantTable[i]);
			}
			*/
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
	//TArray<FVector4> ConstantTable;
	FNiagaraConstantMap Constants;

	//UPROPERTY()
	TArray< TSharedPtr<FNiagaraSimulation> > Emitters;

	UNiagaraComponent *GetComponent() { return Component; }
private:
	UNiagaraComponent *Component;
	TArray<FNiagaraEmitterProperties>EmitterProps;
};
