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
#include "NiagaraCommon.h"
#include "NiagaraSimulation.h"
#include "NiagaraDataInterface.h"
#include "NiagaraDataInterfaceStaticMesh.h"

DECLARE_CYCLE_STAT(TEXT("Gen Verts"),STAT_NiagaraGenerateVertices,STATGROUP_Niagara);

DEFINE_LOG_CATEGORY(LogNiagara);

FNiagaraSceneProxy::FNiagaraSceneProxy(const UNiagaraComponent* InComponent)
		: FPrimitiveSceneProxy(InComponent)
		, bRenderingEnabled(true)
{
	// In this case only, update the effect renderers on the game thread.
	check(IsInGameThread());
	TArray<TSharedRef<FNiagaraSimulation> > Sims = InComponent->GetEffectInstance()->GetEmitters();
	TArray<NiagaraEffectRenderer*> RenderersFromSims;
	for (TSharedRef<FNiagaraSimulation> Sim : Sims)
	{
		RenderersFromSims.Add(Sim->GetEffectRenderer());
	}
	UpdateEffectRenderers(RenderersFromSims);
	//UE_LOG(LogNiagara, Warning, TEXT("FNiagaraSceneProxy %p"), this);

	bAlwaysHasVelocity = true;
}

void FNiagaraSceneProxy::AddEffectRenderer(NiagaraEffectRenderer* Renderer)
{
	EffectRenderers.Add(Renderer); 
	//UE_LOG(LogNiagara, Warning, TEXT("FNiagaraSceneProxy::AddEffectRenderer %p"), Renderer);
}

void FNiagaraSceneProxy::UpdateEffectRenderers(TArray<NiagaraEffectRenderer*>& InRenderers)
{
	EffectRenderers.Empty();
	for (NiagaraEffectRenderer* EmitterRenderer : InRenderers)
	{
		AddEffectRenderer(EmitterRenderer);
	}
}

FNiagaraSceneProxy::~FNiagaraSceneProxy()
{
	//UE_LOG(LogNiagara, Warning, TEXT("~FNiagaraSceneProxy %p"), this);
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

	if (bRenderingEnabled == false)
	{
		return Relevance;
	}
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

bool FNiagaraSceneProxy::GetRenderingEnabled() const
{
	return bRenderingEnabled;
}

void FNiagaraSceneProxy::SetRenderingEnabled(bool bInRenderingEnabled)
{
	bRenderingEnabled = bInRenderingEnabled;
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



void FNiagaraSceneProxy::GatherSimpleLights(const FSceneViewFamily& ViewFamily, FSimpleLightArray& OutParticleLights) const
{
	NiagaraEffectRendererLights *LightRenderer = nullptr;
	FNiagaraDynamicDataLights *DynamicData = nullptr;
	for (int32 Idx = 0; Idx < EffectRenderers.Num(); Idx++)
	{
		NiagaraEffectRenderer *Renderer = EffectRenderers[Idx];
		if (Renderer && Renderer->GetPropertiesClass() == UNiagaraLightRendererProperties::StaticClass())
		{
			LightRenderer = static_cast<NiagaraEffectRendererLights*>(Renderer);
			DynamicData = static_cast<FNiagaraDynamicDataLights*>(Renderer->GetDynamicData());
			break;
		}
	}


	if (DynamicData)
	{
		int32 LightCount = DynamicData->LightArray.Num();
		
		OutParticleLights.InstanceData.Reserve(LightCount);
		OutParticleLights.PerViewData.Reserve(LightCount);

		for (NiagaraEffectRendererLights::SimpleLightData &LightData : DynamicData->LightArray)
		{
			// When not using camera-offset, output one position for all views to share. 
			OutParticleLights.PerViewData.Add(LightData.PerViewEntry);

			// Add an entry for the light instance.
			OutParticleLights.InstanceData.Add(LightData.LightEntry);
		}
	}

}


//////////////////////////////////////////////////////////////////////////

UNiagaraComponent::UNiagaraComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_DuringPhysics;
	bTickInEditor = true;
	bAutoActivate = true;
	bRenderingEnabled = true;
}


void UNiagaraComponent::TickComponent(float DeltaSeconds, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (bIsActive && EffectInstance.Get())
	{ 
		EffectInstance->Tick(DeltaSeconds);
	}

	if (SceneProxy != nullptr)
	{
		FNiagaraSceneProxy* NiagaraProxy = static_cast<FNiagaraSceneProxy*>(SceneProxy);
		NiagaraProxy->SetRenderingEnabled(bRenderingEnabled);
	}

	UpdateComponentToWorld();
	MarkRenderDynamicDataDirty();
}

const UObject* UNiagaraComponent::AdditionalStatObject() const
{
	return Asset;
}

void UNiagaraComponent::ResetEffect()
{
	if (IsRegistered() && EffectInstance.IsValid())
	{
		EffectInstance->Reset(FNiagaraEffectInstance::EResetMode::ImmediateReset);
		MarkRenderDynamicDataDirty();
	}
}

void UNiagaraComponent::ReinitializeEffect()
{
	if (IsRegistered() && EffectInstance.IsValid())
	{
		EffectInstance->Init(Asset);
	}
}

bool UNiagaraComponent::GetRenderingEnabled() const
{
	return bRenderingEnabled;
}

void UNiagaraComponent::SetRenderingEnabled(bool bInRenderingEnabled)
{
	bRenderingEnabled = bInRenderingEnabled;
}

void UNiagaraComponent::OnRegister()
{
	Super::OnRegister();

	if (EffectInstance.IsValid() == false)
	{
		EffectInstance = MakeShareable(new FNiagaraEffectInstance(this));
	}
	
	EffectInstance->Init(Asset, true);
	VectorVM::Init();
}

void UNiagaraComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	Super::OnComponentDestroyed(bDestroyingHierarchy);
	//UE_LOG(LogNiagara, Log, TEXT("OnComponentDestroyed %p %p"), this, EffectInstance.Get());
	EffectInstance = nullptr;
}

void UNiagaraComponent::OnUnregister()
{
	Super::OnUnregister();
	EffectInstance = nullptr;
}

void UNiagaraComponent::SendRenderDynamicData_Concurrent()
{
	if (EffectInstance.IsValid() && SceneProxy)
	{
		EffectInstance->GetEffectBounds().Init();
		
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
		EffectInstance->GetEffectBounds().Init();
		for (int32 i = 0; i < EffectInstance->GetEmitters().Num(); i++)
		{
			FNiagaraSimulation &Sim = *(EffectInstance->GetEmitters()[i]);
			EffectInstance->GetEffectBounds() += Sim.GetCachedBounds();
		}
		FBoxSphereBounds BSBounds(EffectInstance->GetEffectBounds());

		return BSBounds;
	}
	return FBoxSphereBounds();
}

FPrimitiveSceneProxy* UNiagaraComponent::CreateSceneProxy()
{
	// The constructor will set up the effect renderers from the component.
	FNiagaraSceneProxy* Proxy = new FNiagaraSceneProxy(this);
	return Proxy;
}

void UNiagaraComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (!EffectInstance.IsValid())
	{
		return;
	}

	for (TSharedRef<FNiagaraSimulation> Sim : EffectInstance->GetEmitters())	
	{
#if WITH_EDITORONLY_DATA
		OutMaterials.Add(Sim->GetEmitterHandle().GetSource()->Material);
#endif
		if (UNiagaraEmitterProperties* Props = Sim->GetEmitterHandle().GetInstance())
		{	
			//TODO: Remove this! Material and all rendering related data should live in the renderer props
			OutMaterials.Add(Props->Material);

			if (UNiagaraEffectRendererProperties* Renderer = Props->RendererProperties)
			{
				//Add a get materials func to the render props.
				Renderer->GetUsedMaterials(OutMaterials);
			}
		}
	}
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

const FNiagaraVariable* UNiagaraComponent::GetEffectParameter(FName VarName) 
{
	UNiagaraEffect* Effect = GetAsset();
	if (Effect == nullptr)
	{
		return nullptr;
	}
	UNiagaraScript* Script = Effect->GetEffectScript();
	if (Script == nullptr)
	{
		return nullptr;
	}

	// Grab the values already set on the effect to make sure that types match and that we are using ID's for most of the process.
	const FNiagaraParameters& Params = Script->Parameters;
	const FNiagaraVariable* SrcVariable = Params.Parameters.FindByPredicate([&](const FNiagaraVariable& Param) { return Param.GetName() == VarName; });
	return SrcVariable;
}

const FNiagaraVariable* UNiagaraComponent::GetEffectParameter(FGuid VarId)
{
	UNiagaraEffect* Effect = GetAsset();
	if (Effect == nullptr)
	{
		return nullptr;
	}
	UNiagaraScript* Script = Effect->GetEffectScript();
	if (Script == nullptr)
	{
		return nullptr;
	}
	// Grab the values already set on the effect to make sure that types match and that we are using ID's for most of the process.
	const FNiagaraParameters& Params = Script->Parameters;
	const FNiagaraVariable* SrcVariable = Params.Parameters.FindByPredicate([&](const FNiagaraVariable& Param) { return Param.GetId() == VarId; });
	return SrcVariable;
}

FNiagaraVariable* UNiagaraComponent::GetLocalOverrideParameter(FGuid Guid)
{
	FNiagaraVariable* ExistingVar = EffectParameterLocalOverrides.FindByPredicate([&](const FNiagaraVariable& Var)
	{
		return Var.GetId() == Guid;
	});
	return ExistingVar;
}

FNiagaraVariable* UNiagaraComponent::CopyOnWriteParameter(FGuid ParameterId)
{
	const FNiagaraVariable* SrcVariable = GetEffectParameter(ParameterId);
	if (SrcVariable == nullptr)
	{
		if (Asset != nullptr)
		{
			UE_LOG(LogNiagara, Warning, TEXT("ID %s NiagaraVariable doesn't exist on effect '%s'"), *ParameterId.ToString(), *Asset->GetPathName());
		}
		else
		{
			UE_LOG(LogNiagara, Warning, TEXT("ID %s NiagaraVariable doesn't exist, no Asset set."), *ParameterId.ToString());
		}
		return nullptr;
	}

	// Now see if we already have an override local to this component...
	FNiagaraVariable* ExistingVar = GetLocalOverrideParameter(ParameterId);

	// Add if the existing override variable doesn't exist, make one and force invalidation to happen so it 
	// propagates through to the effects.
	if (ExistingVar == nullptr)
	{
		int32 AddIndex = EffectParameterLocalOverrides.Add(*SrcVariable);
		ExistingVar = &EffectParameterLocalOverrides[AddIndex];

		TSharedRef<FNiagaraEffectInstance> Instance = GetEffectInstance();
		Instance->InvalidateComponentBindings();
	}
	return ExistingVar;
}

FNiagaraVariable* UNiagaraComponent::CopyOnWriteParameter(FName VarName, FNiagaraTypeDefinition RequiredType)
{
	const FNiagaraVariable* SrcVariable = GetEffectParameter(VarName);
	if (SrcVariable == nullptr)
	{
		if (Asset != nullptr)
		{
			UE_LOG(LogNiagara, Warning, TEXT("%s NiagaraVariable doesn't exist on effect '%s'"), *VarName.ToString(), *Asset->GetPathName());
		}
		else
		{
			UE_LOG(LogNiagara, Warning, TEXT("%s NiagaraVariable doesn't exist, no Asset set."), *VarName.ToString());
		}
		return nullptr;
	}

	// Make sure this is the expected type.
	if (SrcVariable->GetType() != RequiredType)
	{
		UE_LOG(LogNiagara, Warning, TEXT("%s NiagaraVariable exists, but type is %s, not %s."), *VarName.ToString(), *SrcVariable->GetType().GetName(), *RequiredType.GetName());
		return nullptr;
	}

	// Now see if we already have an override local to this component...
	FGuid ParameterId = SrcVariable->GetId();
	FNiagaraVariable* ExistingVar = GetLocalOverrideParameter(ParameterId);

	// Add if the existing override variable doesn't exist, make one and force invalidation to happen so it 
	// propagates through to the effects.
	if (ExistingVar == nullptr)
	{
		int32 AddIndex = EffectParameterLocalOverrides.Add(*SrcVariable);
		ExistingVar = &EffectParameterLocalOverrides[AddIndex];

		TSharedRef<FNiagaraEffectInstance> Instance = GetEffectInstance();
		Instance->InvalidateComponentBindings();
	}
	return ExistingVar;
}

void UNiagaraComponent::ClearLocalOverrideParameter(FGuid ParameterId)
{
	int32 RemoveCount = EffectParameterLocalOverrides.RemoveAll([&](const FNiagaraVariable& Var)
	{
		return Var.GetId() == ParameterId;
	});

	if (RemoveCount != 0)
	{
		//UE_LOG(LogNiagara, Warning, TEXT("ClearLocalOverrideParameter"));
		TSharedRef<FNiagaraEffectInstance> Instance = GetEffectInstance();
		Instance->InvalidateComponentBindings();
	}
}

void UNiagaraComponent::SetNiagaraVariableVec4(const FString& InVariableName, const FVector4& InValue)
{
	FName VarName = FName(*InVariableName);

	FNiagaraVariable* ExistingVar = CopyOnWriteParameter(VarName, FNiagaraTypeDefinition::GetVec4Def());

	if (ExistingVar != nullptr)
	{
		ExistingVar->SetValue(InValue);
	}
}


void UNiagaraComponent::SetNiagaraVariableVec3(const FString& InVariableName, FVector InValue)
{
	FName VarName = FName(*InVariableName);
	
	FNiagaraVariable* ExistingVar = CopyOnWriteParameter(VarName, FNiagaraTypeDefinition::GetVec3Def());

	if (ExistingVar != nullptr)
	{
		ExistingVar->SetValue(InValue);
	}
}

void UNiagaraComponent::SetNiagaraVariableVec2(const FString& InVariableName, FVector2D InValue)
{
	FName VarName = FName(*InVariableName);

	FNiagaraVariable* ExistingVar = CopyOnWriteParameter(VarName, FNiagaraTypeDefinition::GetVec2Def());

	if (ExistingVar != nullptr)
	{
		ExistingVar->SetValue(InValue);
	}
}

void UNiagaraComponent::SetNiagaraVariableFloat(const FString& InVariableName, float InValue)
{
	FName VarName = FName(*InVariableName);

	FNiagaraVariable* ExistingVar = CopyOnWriteParameter(VarName, FNiagaraTypeDefinition::GetFloatDef());

	if (ExistingVar != nullptr)
	{
		ExistingVar->SetValue(InValue);
	}
}

void UNiagaraComponent::SetNiagaraVariableBool(const FString& InVariableName, bool InValue)
{
	FName VarName = FName(*InVariableName);

	FNiagaraVariable* ExistingVar = CopyOnWriteParameter(VarName, FNiagaraTypeDefinition::GetBoolDef());

	if (ExistingVar != nullptr)
	{
		int32 IntValue = InValue ? INDEX_NONE : 0;
		ExistingVar->SetValue(IntValue);
	}
}

void UNiagaraComponent::SetNiagaraEmitterSpawnRate(const FString& InEmitterName, float InValue)
{
	FName EmitterName = FName(*InEmitterName);
	if (EffectInstance.IsValid())
	{
		for (TSharedRef<FNiagaraSimulation>& Sim : EffectInstance->GetEmitters())
		{
			if (Sim->GetEmitterHandle().GetName() == EmitterName)
			{
				Sim->SetSpawnRate(InValue);
				return;
			}
		}
	}

	UE_LOG(LogNiagara, Warning, TEXT("Emitter '%s' does not exist."), *InEmitterName);
}

const FNiagaraScriptDataInterfaceInfo* UNiagaraComponent::GetEffectDataInterface(FGuid Id) 
{
	if (UNiagaraEffect* Effect = GetAsset())
	{
		const TArray<FNiagaraScriptDataInterfaceInfo>& Params = Effect->GetEffectScript()->DataInterfaceInfo;
		const FNiagaraScriptDataInterfaceInfo* SrcVariable = Params.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
		{
			return Var.Id == Id;
		});

		return SrcVariable;
	}

	return nullptr;
}

const FNiagaraScriptDataInterfaceInfo* UNiagaraComponent::GetEffectDataInterface(FName InName)
{
	if (UNiagaraEffect* Effect = GetAsset())
	{
		const TArray<FNiagaraScriptDataInterfaceInfo>& Params = Effect->GetEffectScript()->DataInterfaceInfo;
		const FNiagaraScriptDataInterfaceInfo* SrcVariable = Params.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
		{
			return Var.Name == InName;
		});

		return SrcVariable;
	}

	return nullptr;
}

FNiagaraScriptDataInterfaceInfo* UNiagaraComponent::GetLocalOverrideDataInterface(FGuid Id)
{
	const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetEffectDataInterface(Id);
	if (SrcVariable == nullptr)
	{
		return nullptr;
	}

	return EffectDataInterfaceLocalOverrides.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
	{
		return Var.Id == SrcVariable->Id;
	});
}

FNiagaraScriptDataInterfaceInfo* UNiagaraComponent::CopyOnWriteDataInterface(FName VarName, UClass* RequiredClass)
{
	const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetEffectDataInterface(VarName);
	if (SrcVariable == nullptr)
	{
		UE_LOG(LogNiagara, Warning, TEXT("%s FNiagaraScriptDataInterfaceInfo doesn't exist."), *VarName.ToString());
		return nullptr;
	}

	// Make sure this is the expected type.
	if (SrcVariable->DataInterface == nullptr)
	{
		UE_LOG(LogNiagara, Warning, TEXT("%s FNiagaraScriptDataInterfaceInfo exists, but datainterface is nullptr."), *VarName.ToString());
		return nullptr;
	}

	if (!SrcVariable->DataInterface->GetClass()->IsChildOf(RequiredClass))
	{
		UE_LOG(LogNiagara, Warning, TEXT("%s FNiagaraScriptDataInterfaceInfo exists, but class is %s, not %s."), *VarName.ToString(), *SrcVariable->DataInterface->GetClass()->GetName(), *RequiredClass->GetName());
		return nullptr;
	}

	bool bInvalidate = false;
	FNiagaraScriptDataInterfaceInfo* ExistingVar = GetLocalOverrideDataInterface(SrcVariable->Id);
	if (ExistingVar == nullptr)
	{
		int32 AddIndex = EffectDataInterfaceLocalOverrides.AddDefaulted();
		ExistingVar = &EffectDataInterfaceLocalOverrides[AddIndex];
		SrcVariable->CopyTo(ExistingVar, this);
		bInvalidate = true;
	}

	if (ExistingVar->DataInterface->GetClass() != SrcVariable->DataInterface->GetClass())
	{
		SrcVariable->CopyTo(ExistingVar, this);
	}

	if (ExistingVar->Name != SrcVariable->Name)
	{
		ExistingVar->Name = SrcVariable->Name;
	}

	if (bInvalidate)
	{
		TSharedRef<FNiagaraEffectInstance> Instance = GetEffectInstance();
		Instance->InvalidateComponentBindings();
	}
	return ExistingVar;
}

FNiagaraScriptDataInterfaceInfo* UNiagaraComponent::CopyOnWriteDataInterface(FGuid Id)
{
	bool bInvalidate = false;
	const FNiagaraScriptDataInterfaceInfo* SrcVariable = GetEffectDataInterface(Id);
	FNiagaraScriptDataInterfaceInfo* ExistingVar = GetLocalOverrideDataInterface(Id);
	if (ExistingVar == nullptr && SrcVariable != nullptr)
	{
		int32 AddIndex = EffectDataInterfaceLocalOverrides.AddDefaulted();
		ExistingVar = &EffectDataInterfaceLocalOverrides[AddIndex];
		SrcVariable->CopyTo(ExistingVar, this);
		bInvalidate = true;
	}

	if (ExistingVar != nullptr && ExistingVar->DataInterface->GetClass() != SrcVariable->DataInterface->GetClass())
	{
		SrcVariable->CopyTo(ExistingVar, this);
	}

	if (ExistingVar != nullptr && ExistingVar->Name != SrcVariable->Name)
	{
		ExistingVar->Name = SrcVariable->Name;
	}

	if (bInvalidate)
	{
		TSharedRef<FNiagaraEffectInstance> Instance = GetEffectInstance();
		Instance->InvalidateComponentBindings();
	}
	return ExistingVar;
}

void UNiagaraComponent::SetNiagaraStaticMeshDataInterfaceActor(const FString& InVariableName, AActor* InSource)
{
	FNiagaraScriptDataInterfaceInfo* Variable = CopyOnWriteDataInterface(FName(*InVariableName), UNiagaraDataInterfaceStaticMesh::StaticClass());
	if (Variable != nullptr)
	{
		UNiagaraDataInterfaceStaticMesh* MeshDataInterface = Cast<UNiagaraDataInterfaceStaticMesh>(Variable->DataInterface);
		if (MeshDataInterface != nullptr)
		{
			MeshDataInterface->Source = InSource;
			MeshDataInterface->PrepareForSimulation(&GetEffectInstance().Get());
			//ReinitializeEffect();
		}
	}
}


#if WITH_EDITOR
void UNiagaraComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ReinitializeEffect();
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
	{
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
	}

	{
		TArray<int32> VarIndicesToRemove;

		// Check over all of our overrides to see if they still match up with the source effect.
		for (int32 i = 0; i < EffectDataInterfaceLocalOverrides.Num(); i++)
		{
			FNiagaraScriptDataInterfaceInfo* Variable = Script->DataInterfaceInfo.FindByPredicate([&](const FNiagaraScriptDataInterfaceInfo& Var)
			{
				return Var.Id == EffectDataInterfaceLocalOverrides[i].Id;
			}); 
			// If the variable still exists, keep digging... otherwise remove it.
			if (Variable)
			{
				if (Variable->DataInterface->GetClass() != EffectDataInterfaceLocalOverrides[i].DataInterface->GetClass())
				{
					UE_LOG(LogNiagara, Warning, TEXT("Variable '%s' types changed.. losing override value."), *Variable->Name.ToString());
					VarIndicesToRemove.Add(i);
				}
				else if (Variable->Name != EffectDataInterfaceLocalOverrides[i].Name)
				{
					if (bEditsMade == false)
					{
						bEditsMade = true;
						Modify();
					}
					EffectDataInterfaceLocalOverrides[i].Name = Variable->Name;
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
				UE_LOG(LogNiagara, Warning, TEXT("Removing local override '%s'."), *EffectDataInterfaceLocalOverrides[VarIndicesToRemove[removeIdx]].Name.ToString());

				EffectDataInterfaceLocalOverrides.RemoveAt(VarIndicesToRemove[removeIdx]);
			}
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
		SystemParameters.Add(SYS_PARAM_EFFECT_VELOCITY);
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
		SystemParameters.Add(SYS_PARAM_SPAWN_INTERVAL);
		SystemParameters.Add(SYS_PARAM_INTERP_SPAWN_START_DT);
		SystemParameters.Add(SYS_PARAM_INV_DELTA_TIME);
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

