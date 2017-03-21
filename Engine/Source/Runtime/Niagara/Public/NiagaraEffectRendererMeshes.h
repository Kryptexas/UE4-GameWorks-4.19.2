// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraEffectRenderer.h: Base class for Niagara render modules
==============================================================================*/
#pragma once

#include "NiagaraMeshVertexFactory.h"
#include "NiagaraEffectRenderer.h"

class FNiagaraDataSet;



struct FNiagaraDynamicDataMesh : public FNiagaraDynamicDataBase
{
	TArray<FNiagaraMeshInstanceVertex> VertexData;
	TArray<FNiagaraMeshInstanceVertexDynamicParameter> MaterialParameterVertexData;
};



/**
* NiagaraEffectRendererSprites renders an FNiagaraSimulation as sprite particles
*/
class NIAGARA_API NiagaraEffectRendererMeshes : public NiagaraEffectRenderer
{
public:

	explicit NiagaraEffectRendererMeshes(ERHIFeatureLevel::Type FeatureLevel, UNiagaraEffectRendererProperties *Props);
	~NiagaraEffectRendererMeshes()
	{
		ReleaseRenderThreadResources();
	}


	virtual void ReleaseRenderThreadResources() override;

	// FPrimitiveSceneProxy interface.
	virtual void CreateRenderThreadResources() override;

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const override;
	virtual bool SetMaterialUsage() override;
	/** Update render data buffer from attributes */
	FNiagaraDynamicDataBase *GenerateVertexData(const FNiagaraSceneProxy* Proxy, const FNiagaraDataSet &Data) override;

	virtual void SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData) override;
	int GetDynamicDataSize() override;
	bool HasDynamicData() override;

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View, const FNiagaraSceneProxy *SceneProxy)
	{
		FPrimitiveViewRelevance Result;
		bool bHasDynamicData = HasDynamicData();
		Result.bDrawRelevance = bHasDynamicData && SceneProxy->IsShown(View) && View->Family->EngineShowFlags.Particles;
		Result.bShadowRelevance = bHasDynamicData && SceneProxy->IsShadowCast(View);
		Result.bDynamicRelevance = bHasDynamicData;


		// TODO: terribad. Figure out a way to cache relevance in a less haphazard way
		//  could cache at init time for all possible feature levels
		//
		static UStaticMesh *ParticleMesh = nullptr;
		if (ParticleMesh != Properties->ParticleMesh)
		{
			TArray<UMaterialInterface*> MeshMaterials;
			Properties->GetUsedMaterials(MeshMaterials);
			for (UMaterialInterface *MatInt : MeshMaterials)
			{
				if (MatInt)
				{
					MaterialRelevance |= MatInt->GetRelevance(View->FeatureLevel);
				}
			}
		}

		if (bHasDynamicData)
		{
			Result.bOpaqueRelevance = MaterialRelevance.bOpaque;
			Result.bNormalTranslucencyRelevance = MaterialRelevance.bNormalTranslucency;
			Result.bSeparateTranslucencyRelevance = MaterialRelevance.bSeparateTranslucency;
			Result.bDistortionRelevance = MaterialRelevance.bDistortion;
		}



		return Result;
	}




	UClass *GetPropertiesClass() override { return UNiagaraMeshRendererProperties::StaticClass(); }
	void SetRendererProperties(UNiagaraEffectRendererProperties *Props) override { Properties = Cast<UNiagaraMeshRendererProperties>(Props); }
	virtual const TArray<FNiagaraVariable>& GetRequiredAttributes() override;

	void SetupVertexFactory(FNiagaraMeshVertexFactory *InVertexFactory, const FStaticMeshLODResources& LODResources) const;

private:
	UNiagaraMeshRendererProperties *Properties;
	mutable TUniformBuffer<FPrimitiveUniformShaderParameters> WorldSpacePrimitiveUniformBuffer;
	class FNiagaraMeshVertexFactory* VertexFactory;
};
