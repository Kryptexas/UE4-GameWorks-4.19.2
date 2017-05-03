// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NiagaraEffectRenderer.h"
#include "Particles/ParticleResources.h"
#include "ParticleBeamTrailVertexFactory.h"
#include "NiagaraDataSet.h"
#include "NiagaraStats.h"

DECLARE_CYCLE_STAT(TEXT("Generate Particle Lights"), STAT_NiagaraGenLights, STATGROUP_Niagara);


/** Enable/disable parallelized effect renderers */
int32 GbNiagaraParallelEffectRenderers = 1;
static FAutoConsoleVariableRef CVarParallelEffectRenderers(
	TEXT("niagara.ParallelEffectRenderers"),
	GbNiagaraParallelEffectRenderers,
	TEXT("Whether to run Niagara effect renderers in parallel"),
	ECVF_Default
	);




NiagaraEffectRenderer::~NiagaraEffectRenderer() 
{
}

void NiagaraEffectRenderer::Release()
{
	check(IsInGameThread());
	ENQUEUE_UNIQUE_RENDER_COMMAND_ONEPARAMETER(
		NiagaraEffectRendererDeletion,
		NiagaraEffectRenderer*, Renderer, this,
		{
			delete Renderer;
		}
	);
}

	


NiagaraEffectRendererLights::NiagaraEffectRendererLights(ERHIFeatureLevel::Type FeatureLevel, UNiagaraEffectRendererProperties *InProps) :
	NiagaraEffectRenderer()
{
	Properties = Cast<UNiagaraLightRendererProperties>(InProps);
}


void NiagaraEffectRendererLights::ReleaseRenderThreadResources()
{
}

void NiagaraEffectRendererLights::CreateRenderThreadResources()
{
}



/** Update render data buffer from attributes */
FNiagaraDynamicDataBase *NiagaraEffectRendererLights::GenerateVertexData(const FNiagaraSceneProxy* Proxy, const FNiagaraDataSet &Data)
{
	SCOPE_CYCLE_COUNTER(STAT_NiagaraGenLights);

	SimpleTimer VertexDataTimer;

	//I'm not a great fan of pulling scalar components out to a structured vert buffer like this.
	//TODO: Experiment with a new VF that reads the data directly from the scalar layout.
	FNiagaraDataSetIterator<FVector> PosItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), TEXT("Position")));
	FNiagaraDataSetIterator<FLinearColor> ColItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), TEXT("Color")));
	FNiagaraDataSetIterator<FVector2D> SizeItr(Data, FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), TEXT("Size")));

	//Bail if we don't have the required attributes to render this emitter.
	if (!PosItr.IsValid() || !ColItr.IsValid() || !SizeItr.IsValid())
	{
		return nullptr;
	}

	FNiagaraDynamicDataLights *DynamicData = new FNiagaraDynamicDataLights;

	DynamicData->LightArray.Empty();

	for (uint32 ParticleIndex = 0; ParticleIndex < Data.GetNumInstances(); ParticleIndex++)
	{
		SimpleLightData LightData;
		LightData.LightEntry.Radius = (*SizeItr).X;	//LightPayload->RadiusScale * (Size.X + Size.Y) / 2.0f;
		LightData.LightEntry.Color = FVector((*ColItr));				//FVector(Particle.Color) * Particle.Color.A * LightPayload->ColorScale;
		LightData.LightEntry.Exponent = 1.0;
		LightData.LightEntry.bAffectTranslucency = true;
		LightData.PerViewEntry.Position = (*PosItr);

		DynamicData->LightArray.Add(LightData);

		PosItr.Advance();
		ColItr.Advance();
		SizeItr.Advance();
	}

	CPUTimeMS = VertexDataTimer.GetElapsedMilliseconds();

	return DynamicData;
}


void NiagaraEffectRendererLights::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const
{

}

void NiagaraEffectRendererLights::SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData)
{
	if (DynamicDataRender)
	{
		delete static_cast<FNiagaraDynamicDataLights*>(DynamicDataRender);
		DynamicDataRender = NULL;
	}
	DynamicDataRender = static_cast<FNiagaraDynamicDataLights*>(NewDynamicData);
}

int NiagaraEffectRendererLights::GetDynamicDataSize()
{
	return 0;
}
bool NiagaraEffectRendererLights::HasDynamicData()
{
	return false;
}

bool NiagaraEffectRendererLights::SetMaterialUsage()
{
	return false;
}

const TArray<FNiagaraVariable>& NiagaraEffectRendererLights::GetRequiredAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		FNiagaraVariable PositionStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetVec3Def(), "Position");
		FNiagaraVariable ColorStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetColorDef(), "Color");
		FNiagaraVariable SizeStruct = FNiagaraVariable(FNiagaraTypeDefinition::GetVec2Def(), "Size");

		Attrs.Add(PositionStruct);
		Attrs.Add(ColorStruct);
		Attrs.Add(SizeStruct);
	}

	return Attrs;
}












