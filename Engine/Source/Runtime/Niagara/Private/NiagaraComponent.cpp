// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraComponent.h"
#include "VectorVM.h"
#include "NiagaraEffectRenderer.h"
#include "NiagaraEffect.h"
#include "NiagaraEffectInstance.h"
#include "NiagaraSimulation.h"
#include "MeshBatch.h"
#include "SceneUtils.h"
#include "ComponentReregisterContext.h"
#include "NiagaraConstants.h"
#include "NiagaraStats.h"

DECLARE_CYCLE_STAT(TEXT("Gen Verts"),STAT_NiagaraGenerateVertices,STATGROUP_Niagara);

DEFINE_LOG_CATEGORY(LogNiagara);

FNiagaraSceneProxy::FNiagaraSceneProxy(const UNiagaraComponent* InComponent)
		:	FPrimitiveSceneProxy(InComponent)
{
	UpdateEffectRenderers(InComponent->GetEffectInstance().Get());
}

void FNiagaraSceneProxy::UpdateEffectRenderers(FNiagaraEffectInstance* InEffect)
{
	EffectRenderers.Empty();
	if (InEffect)
	{
		for (TSharedPtr<FNiagaraSimulation>Emitter : InEffect->GetEmitters())
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
	for (NiagaraEffectRenderer* Renderer : EffectRenderers)
	{
		if (Renderer)
		{
			Renderer->SetDynamicData_RenderThread(NewDynamicData);
		}
	}
	return;
}


void FNiagaraSceneProxy::ReleaseRenderThreadResources()
{
	for (NiagaraEffectRenderer* Renderer : EffectRenderers)
	{
		if (Renderer)
		{
			Renderer->ReleaseRenderThreadResources();
		}
	}
	return;
}

// FPrimitiveSceneProxy interface.
void FNiagaraSceneProxy::CreateRenderThreadResources()
{
	for (NiagaraEffectRenderer* Renderer : EffectRenderers)
	{
		if (Renderer)
		{
			Renderer->CreateRenderThreadResources();
		}
	}
	return;
}

void FNiagaraSceneProxy::OnTransformChanged()
{
	//WorldSpacePrimitiveUniformBuffer.ReleaseResource();
}

FPrimitiveViewRelevance FNiagaraSceneProxy::GetViewRelevance(const FSceneView* View) const
{
	FPrimitiveViewRelevance Relevance;
	Relevance.bDynamicRelevance = true;

	for (NiagaraEffectRenderer* Renderer : EffectRenderers)
	{
		if (Renderer)
		{
			Relevance |= Renderer->GetViewRelevance(View, this);
		}
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
	for (NiagaraEffectRenderer* Renderer : EffectRenderers)
	{
		if (Renderer)
		{
			DynamicDataSize += Renderer->GetDynamicDataSize();
		}
	}
	return FPrimitiveSceneProxy::GetAllocatedSize() + DynamicDataSize;
}


void FNiagaraSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
	for (NiagaraEffectRenderer* Renderer : EffectRenderers)
	{
		if (Renderer)
		{
			Renderer->GetDynamicMeshElements(Views, ViewFamily, VisibilityMap, Collector, this);
		}
	}
}

//////////////////////////////////////////////////////////////////////////

UNiagaraComponent::UNiagaraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
}


void UNiagaraComponent::TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (EffectInstance.Get())
	{ 
		EffectInstance->Tick(DeltaSeconds);
	}

	UpdateComponentToWorld();
	MarkRenderDynamicDataDirty();
}

const UObject* UNiagaraComponent::AdditionalStatObject() const
{
	return Asset;
}

void UNiagaraComponent::OnRegister()
{
	Super::OnRegister();

	if (EffectInstance.IsValid() == false)
	{
		EffectInstance = MakeShareable(new FNiagaraEffectInstance(this));
	}
	
	EffectInstance->Init(Asset);
	VectorVM::Init();
}


void UNiagaraComponent::SendRenderDynamicData_Concurrent()
{
	if (EffectInstance.IsValid() && SceneProxy)
	{
		FNiagaraSceneProxy* NiagaraProxy = static_cast<FNiagaraSceneProxy*>(SceneProxy);
		for (int32 i = 0; i < EffectInstance->GetEmitters().Num(); i++)
		{
			TSharedPtr<FNiagaraSimulation> Emitter = EffectInstance->GetEmitters()[i];
			NiagaraEffectRenderer* Renderer = Emitter->GetEffectRenderer();
			if (Renderer)
			{
				if (Emitter->IsEnabled())
				{
					FNiagaraDynamicDataBase* DynamicData = Renderer->GenerateVertexData(NiagaraProxy, Emitter->GetData());

					ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
						FSendNiagaraDynamicData,
						NiagaraEffectRenderer*, EffectRenderer, Emitter->GetEffectRenderer(),
						FNiagaraDynamicDataBase*, DynamicData, DynamicData,
						{
						EffectRenderer->SetDynamicData_RenderThread(DynamicData);
					});
				}
				else
				{
					ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
						FSendNiagaraDynamicData,
						NiagaraEffectRenderer*, EffectRenderer, Emitter->GetEffectRenderer(),
						{
							EffectRenderer->SetDynamicData_RenderThread(nullptr);
						});
				}
			}
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
	if (EffectInstance.IsValid())
	{
		FBoxSphereBounds BSBounds(EffectInstance->GetEffectBounds());
		//BSBounds.Origin = LocalToWorld.GetLocation();
		return BSBounds;
	}
	return FBoxSphereBounds();
}

FPrimitiveSceneProxy* UNiagaraComponent::CreateSceneProxy()
{
	FNiagaraSceneProxy* Proxy = new FNiagaraSceneProxy(this);
	Proxy->UpdateEffectRenderers(EffectInstance.Get());
	return Proxy;
}

TSharedPtr<FNiagaraEffectInstance> UNiagaraComponent::GetEffectInstance() const
{
	return EffectInstance;
}

TSharedRef<FNiagaraEffectInstance> UNiagaraComponent::GetEffectInstance()
{
	if (EffectInstance.IsValid() == false)
	{
		EffectInstance = MakeShareable(new FNiagaraEffectInstance(this));
	}
	return EffectInstance.ToSharedRef();
}

#if WITH_EDITOR
void UNiagaraComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	FComponentReregisterContext ReregisterContext(this);
}


bool UNiagaraComponent::SynchronizeWithSourceEffect()
{
	if (Asset == nullptr)
	{
		return false; 
	}

	UNiagaraScript* Script = Asset->GetEffectScript();
	if (Script == nullptr)
	{
		return false;
	}

	bool bEditsMade = false;
	TArray<int32> VarIndicesToRemove;

	// Check over all of our overrides to see if they still match up with the source effect.
	for (int32 i = 0; i < EffectParameterLocalOverrides.Num(); i++)
	{
		FNiagaraVariable* Variable = Script->Parameters.FindParameter(EffectParameterLocalOverrides[i].GetId());
		// If the variable still exists, keep digging... otherwise remove it.
		if (Variable)
		{
			if (Variable->GetType() != EffectParameterLocalOverrides[i].GetType())
			{
				UE_LOG(LogNiagara, Warning, TEXT("Variable '%s' types changed.. possibly losing override value."), *Variable->GetName().ToString());
				VarIndicesToRemove.Add(i);
			}
			else if (Variable->GetName() != EffectParameterLocalOverrides[i].GetName())
			{
				if (bEditsMade == false)
				{
					bEditsMade = true;
					Modify();
				}
				EffectParameterLocalOverrides[i].SetName(Variable->GetName());
			}
		}
		else
		{
			VarIndicesToRemove.Add(i);
		}
	}

	// Handle the variables that we want to remove.
	if (VarIndicesToRemove.Num() != 0)
	{
		if (bEditsMade == false)
		{
			bEditsMade = true;
			Modify();
		}

		for (int32 removeIdx = VarIndicesToRemove.Num() - 1; removeIdx >= 0; removeIdx--)
		{
			UE_LOG(LogNiagara, Warning, TEXT("Removing local override '%s'."), *EffectParameterLocalOverrides[VarIndicesToRemove[removeIdx]].GetName().ToString());

			EffectParameterLocalOverrides.RemoveAt(VarIndicesToRemove[removeIdx]);
		}
	}

	return bEditsMade;
}

#endif // WITH_EDITOR



void UNiagaraComponent::SetAsset(UNiagaraEffect* InAsset)
{
	Asset = InAsset;

	if (IsRegistered())
	{
		EffectInstance->Init(Asset);
	}
}


const TArray<FNiagaraVariable>& UNiagaraComponent::GetSystemConstants()
{
	static TArray<FNiagaraVariable> SystemParameters;
	if (SystemParameters.Num() == 0)
	{
		SystemParameters.Add(SYS_PARAM_DELTA_TIME);
		SystemParameters.Add(SYS_PARAM_EMITTER_AGE);
		SystemParameters.Add(SYS_PARAM_EFFECT_POSITION);
		SystemParameters.Add(SYS_PARAM_EFFECT_X_AXIS);
		SystemParameters.Add(SYS_PARAM_EFFECT_Y_AXIS);
		SystemParameters.Add(SYS_PARAM_EFFECT_Z_AXIS);
		SystemParameters.Add(SYS_PARAM_EFFECT_LOCAL_TO_WORLD);
		SystemParameters.Add(SYS_PARAM_EFFECT_WORLD_TO_LOCAL);
		SystemParameters.Add(SYS_PARAM_EFFECT_LOCAL_TO_WORLD_TRANSPOSED);
		SystemParameters.Add(SYS_PARAM_EFFECT_WORLD_TO_LOCAL_TRANSPOSED);
		SystemParameters.Add(SYS_PARAM_EXEC_COUNT);

		//TODO: Should be exposed to spawn script only? What about modules?
		//Think our idea of "system parameters" needs improving.
		SystemParameters.Add(SYS_PARAM_SPAWNRATE);
		SystemParameters.Add(SYS_PARAM_SPAWNRATE_INVERSE);
		SystemParameters.Add(SYS_PARAM_SPAWNRATE_REMAINDER);
	}
	return SystemParameters;
}

const FNiagaraVariable* UNiagaraComponent::FindSystemConstant(const FNiagaraVariable& InVar)
{
	const TArray<FNiagaraVariable>& SystemParameters = GetSystemConstants();
	const FNiagaraVariable* FoundSystemVar = SystemParameters.FindByPredicate([&](const FNiagaraVariable& Var)
	{
		return Var.GetName() == InVar.GetName();
	});
	return FoundSystemVar;
}
/*FNiagaraDataSetID FNiagaraDataSetID::DeathEvent(TEXT("Death"),ENiagaraDataSetType::Event);
FNiagaraDataSetID FNiagaraDataSetID::SpawnEvent(TEXT("Spawn"), ENiagaraDataSetType::Event);
FNiagaraDataSetID FNiagaraDataSetID::CollisionEvent(TEXT("Collision"), ENiagaraDataSetType::Event);*/

