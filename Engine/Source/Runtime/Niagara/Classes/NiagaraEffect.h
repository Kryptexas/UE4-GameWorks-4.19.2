// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraSimulation.h"
#include "Engine/NiagaraEffectRenderer.h"
#include "List.h"
#include "Runtime/VectorVM/Public/VectorVMDataObject.h"
#include "NiagaraEffect.generated.h"


enum NiagaraTickState
{
	ERunning,		// normally running
	ESuspended,		// stop simulating and spawning, still render
	EDieing,		// stop spawning, still simulate and render
	EDead			// no live particles, no new spawning
};

UCLASS()
class NIAGARA_API UNiagaraEffect : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	FNiagaraEmitterProperties* AddEmitterProperties();

	void Init()
	{
	}


	FNiagaraEmitterProperties *GetEmitterProperties(int Idx)
	{
		check(Idx < EmitterProps.Num());
		return EmitterProps[Idx];
	};

	int GetNumEmitters()
	{
		return EmitterProps.Num();
	}

	void CreateEffectRendererProps(TSharedPtr<FNiagaraSimulation> Sim);

	virtual void PostLoad() override; 
	virtual void PreSave() override;

private:
	// serialized array of emitter properties
	UPROPERTY()
	TArray<FNiagaraEmitterProperties>EmitterPropsSerialized;

	TArray<FNiagaraEmitterProperties*> EmitterProps;
};



class FNiagaraEffectInstance
{
public:
	explicit FNiagaraEffectInstance(UNiagaraEffect *InAsset, UNiagaraComponent *InComponent)
		: Effect(InAsset)
	{
		Component = InComponent;
		for (int32 i = 0; i < InAsset->GetNumEmitters(); i++)
		{
			FNiagaraSimulation *Sim = new FNiagaraSimulation(InAsset->GetEmitterProperties(i), InAsset, Component->GetWorld()->FeatureLevel);
			Emitters.Add(MakeShareable(Sim));
		}
		InitRenderModules(Component->GetWorld()->FeatureLevel);
		VolumeGrid = NewObject<UNiagaraSparseVolumeDataObject>();
	}

	explicit FNiagaraEffectInstance(UNiagaraEffect *InAsset)
		: Component(nullptr)
		, Effect(InAsset)
	{
		InitEmitters(InAsset);
		VolumeGrid = NewObject<UNiagaraSparseVolumeDataObject>();
	}

	NIAGARA_API void InitEmitters(UNiagaraEffect *InAsset);
	void InitRenderModules(ERHIFeatureLevel::Type InFeatureLevel)
	{
		for (TSharedPtr<FNiagaraSimulation> Sim : Emitters)
		{
			if(Sim->GetProperties()->RendererProperties == nullptr)
			{
				Component->GetAsset()->CreateEffectRendererProps(Sim);
			}
			Sim->SetRenderModuleType(Sim->GetProperties()->RenderModuleType, InFeatureLevel);
		}
	}

	void Init(UNiagaraComponent *InComponent)
	{
		if (InComponent->GetAsset())
		{
			Emitters.Empty();
			InitEmitters(InComponent->GetAsset());
		}
		Component = InComponent;
		InitRenderModules(Component->GetWorld()->FeatureLevel);
		RenderModuleupdate();

		Age = 0.0f;
	}

	NIAGARA_API TSharedPtr<FNiagaraSimulation> AddEmitter(FNiagaraEmitterProperties *Properties);

	void SetConstant(FNiagaraVariableInfo ID, const float Value)
	{
		Constants.SetOrAdd(ID, Value);
	}

	void SetConstant(FNiagaraVariableInfo ID, const FVector4& Value)
	{
		Constants.SetOrAdd(ID, Value);
	}

	void SetConstant(FNiagaraVariableInfo ID, const FMatrix& Value)
	{
		Constants.SetOrAdd(ID, Value);
	}

	void SetConstant(FName ConstantName, UNiagaraDataObject *Value)
	{
		Constants.SetOrAdd(ConstantName, Value);
	}



	void Tick(float DeltaSeconds)
	{
		Constants.SetOrAdd(FName(TEXT("EffectGrid")), VolumeGrid); 

		// pass the constants down to the emitter
		// TODO: should probably just pass a pointer to the table
		EffectBounds.Init();

		for (TSharedPtr<FNiagaraSimulation>&it : Emitters)
		{
			FNiagaraEmitterProperties *Props = it->GetProperties();

			int Duration = Props->EndTime - Props->StartTime;
			int LoopedStartTime = Props->StartTime + Duration*it->GetLoopCount();
			int LoopedEndTime = Props->EndTime + Duration*it->GetLoopCount();

			// manage emitter lifetime
			//
			if (	(Props->StartTime == 0.0f && Props->EndTime == 0.0f)
					|| (LoopedStartTime<Age && LoopedEndTime>Age)
				)
			{
				it->SetTickState(NTS_Running);
			}
			else
			{
				// if we're past end time, manage looping; we reset the emitters age constant
				// if it has one
				if (Props->NumLoops > 1 && it->GetLoopCount()<Props->NumLoops)
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
				it->SetConstants(Constants);
				it->GetConstants().Merge(it->GetProperties()->ExternalConstants);
				it->Tick(DeltaSeconds);
			}

			EffectBounds += it->GetEffectRenderer()->GetBounds();
		}

		Age += DeltaSeconds;
	}

	NIAGARA_API void RenderModuleupdate();

	FNiagaraSimulation *GetEmitter(uint32 idx)
	{
		return Emitters[idx].Get();
	}


	UNiagaraComponent *GetComponent() { return Component; }

	TArray< TSharedPtr<FNiagaraSimulation> > &GetEmitters()	{ return Emitters; }

	FBox GetEffectBounds()	{ return EffectBounds;  }
private:
	UNiagaraComponent *Component;
	UNiagaraEffect *Effect;
	FBox EffectBounds;
	float Age;

	/** Local constant table. */
	FNiagaraConstantMap Constants;

	TArray< TSharedPtr<FNiagaraSimulation> > Emitters;

	UNiagaraSparseVolumeDataObject *VolumeGrid;
};