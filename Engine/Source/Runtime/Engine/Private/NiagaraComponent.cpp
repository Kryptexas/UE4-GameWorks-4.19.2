// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "Components/NiagaraComponent.h"
#include "Engine/NiagaraScript.h"
#include "VectorVM.h"
#include "ParticleHelper.h"
#include "Particles/ParticleResources.h"
#include "Engine/NiagaraEffectRenderer.h"
#include "Engine/NiagaraEffect.h"
#include "Engine/NiagaraSimulation.h"
#include "MeshBatch.h"
#include "SceneUtils.h"
#include "ComponentReregisterContext.h"

DECLARE_CYCLE_STAT(TEXT("Gen Verts"),STAT_NiagaraGenerateVertices,STATGROUP_Niagara);
DECLARE_DWORD_COUNTER_STAT(TEXT("NumParticles"),STAT_NiagaraNumParticles,STATGROUP_Niagara);



FNiagaraSceneProxy::FNiagaraSceneProxy(const UNiagaraComponent* InComponent)
		:	FPrimitiveSceneProxy(InComponent)
{
	UpdateEffectRenderers(InComponent->Effect);
}

void FNiagaraSceneProxy::UpdateEffectRenderers(UNiagaraEffect *InEffect)
{
	EffectRenderers.Empty();
	if (InEffect)
	{
		for (TSharedPtr<FNiagaraSimulation>Emitter : InEffect->Emitters)
		{
			AddEffectRenderer(Emitter->GetEffectRenderer());
		}
	}
}

FNiagaraSceneProxy::~FNiagaraSceneProxy()
{
	ReleaseRenderThreadResources();
}

/** Called on render thread to assign new dynamic data */
void FNiagaraSceneProxy::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		Renderer->SetDynamicData_RenderThread(NewDynamicData);
	}
	return;
}


void FNiagaraSceneProxy::ReleaseRenderThreadResources()
{
	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		Renderer->ReleaseRenderThreadResources();
	}
	return;
}

// FPrimitiveSceneProxy interface.
void FNiagaraSceneProxy::CreateRenderThreadResources()
{
	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		Renderer->CreateRenderThreadResources();
	}
	return;
}

void FNiagaraSceneProxy::OnActorPositionChanged()
{
	//WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void FNiagaraSceneProxy::OnTransformChanged()
{
	//WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

void FNiagaraSceneProxy::PreRenderView(const FSceneViewFamily* ViewFamily, const uint32 VisibilityMap, int32 FrameNumber)
{
	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		Renderer->PreRenderView(ViewFamily, VisibilityMap, FrameNumber, this);
	}
	return;
}
		  
void FNiagaraSceneProxy::DrawDynamicElements(FPrimitiveDrawInterface* PDI, const FSceneView* View) 
{
	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		Renderer->DrawDynamicElements(PDI, View, this);
	}
	return;
}

FPrimitiveViewRelevance FNiagaraSceneProxy::GetViewRelevance(const FSceneView* View)
{
	FPrimitiveViewRelevance Relevance;
	Relevance.bDynamicRelevance = true;

	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		Relevance |= EffectRenderers[0]->GetViewRelevance(View, this);
	}
	return Relevance;
}


uint32 FNiagaraSceneProxy::GetMemoryFootprint() const
{ 
	return (sizeof(*this) + GetAllocatedSize()); 
}

uint32 FNiagaraSceneProxy::GetAllocatedSize() const
{ 
	uint32 DynamicDataSize = 0;
	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		DynamicDataSize += Renderer->GetDynamicDataSize();
	}
	return FPrimitiveSceneProxy::GetAllocatedSize() + DynamicDataSize;
}


void FNiagaraSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	for (NiagaraEffectRenderer *Renderer : EffectRenderers)
	{
		Renderer->GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector, this);
	}
}




namespace ENiagaraVectorAttr
{
	enum Type
	{
		Position,
		Velocity,
		Color,
		Rotation,
		RelativeTime,
		MaxVectorAttribs
	};
}



//////////////////////////////////////////////////////////////////////////

UNiagaraComponent::UNiagaraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
}


void UNiagaraComponent::TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
//	EmitterAge += DeltaSeconds;

	if (Effect)
	{

		//Todo, open this up to the UI and setting via code and BPs.
		static FName Const_Zero(TEXT("ZERO"));
		Effect->SetConstant(Const_Zero, FVector4(0.0f, 0.0f, 0.0f, 0.0f));	// zero constant
		static FName Const_DeltaTime(TEXT("Delta Time"));
		Effect->SetConstant(Const_DeltaTime, FVector4(DeltaSeconds, DeltaSeconds, DeltaSeconds, DeltaSeconds));
		static FName Const_EmitterPos(TEXT("Emitter Position"));
		Effect->SetConstant(Const_EmitterPos, FVector4(ComponentToWorld.GetTranslation()));
		//static FName Const_Age(TEXT("Emitter Age"));
		//Effect->SetConstant(Const_Age, FVector4(EmitterAge, EmitterAge, EmitterAge, EmitterAge));
		static FName Const_EmitterX(TEXT("Emitter X Axis"));
		Effect->SetConstant(Const_EmitterX, FVector4(ComponentToWorld.GetUnitAxis(EAxis::X)));
		static FName Const_EmitterY(TEXT("Emitter Y Axis"));
		Effect->SetConstant(Const_EmitterY, FVector4(ComponentToWorld.GetUnitAxis(EAxis::Y)));
		static FName Const_EmitterZ(TEXT("Emitter Z Axis"));
		Effect->SetConstant(Const_EmitterZ, FVector4(ComponentToWorld.GetUnitAxis(EAxis::Z)));
		static FName Const_EmitterTransform(TEXT("Emitter Transform"));
		Effect->SetConstant(Const_EmitterTransform, ComponentToWorld.ToMatrixWithScale());
		Effect->Tick(DeltaSeconds);
	}

	UpdateComponentToWorld();
	MarkRenderDynamicDataDirty();
}

void UNiagaraComponent::OnRegister()
{
	Super::OnRegister();
	if (Effect)
	{
		Effect->Init(this);
	}
	VectorVM::Init();
}


void UNiagaraComponent::OnUnregister()
{
	Super::OnUnregister();
}

void UNiagaraComponent::SendRenderDynamicData_Concurrent()
{
	if (Effect && SceneProxy)
	{
		FNiagaraSceneProxy *NiagaraProxy = static_cast<FNiagaraSceneProxy*>(SceneProxy);
		for (int32 i = 0; i < Effect->Emitters.Num(); i++)
		{
			FNiagaraSimulation *Emitter = Effect->GetEmitter(i);
			FNiagaraDynamicDataBase* DynamicData = Emitter->GetEffectRenderer()->GenerateVertexData(Effect->Emitters[i]->GetData());

			ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
				FSendNiagaraDynamicData,
				NiagaraEffectRenderer*, EffectRenderer, Emitter->GetEffectRenderer(),
				FNiagaraDynamicDataBase*, DynamicData, DynamicData,
				{
				EffectRenderer->SetDynamicData_RenderThread(DynamicData);
			});
		}
	}

}

int32 UNiagaraComponent::GetNumMaterials() const
{
	return 0;
}


FBoxSphereBounds UNiagaraComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBox SimBounds(ForceInit);

	/*
	if (Effect)
	{
		for (FNiagaraSimulation Sim : Effect->Emitters)
		{
			SimBounds += Sim->GetBounds();
		}
	}
	*/
	{
		SimBounds.Min = FVector(-HALF_WORLD_MAX,-HALF_WORLD_MAX,-HALF_WORLD_MAX);
		SimBounds.Max = FVector(+HALF_WORLD_MAX,+HALF_WORLD_MAX,+HALF_WORLD_MAX);
	}
	return FBoxSphereBounds(SimBounds);
}

FPrimitiveSceneProxy* UNiagaraComponent::CreateSceneProxy()
{
	FNiagaraSceneProxy *Proxy = new FNiagaraSceneProxy(this);
	return Proxy;
}

#if WITH_EDITOR
void UNiagaraComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FComponentReregisterContext ReregisterContext(this);
}
#endif // WITH_EDITOR
