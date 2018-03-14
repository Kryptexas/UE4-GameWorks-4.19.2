// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

/*==============================================================================
NiagaraRenderer.h: Base class for Niagara render modules
==============================================================================*/
#pragma once

#include "NiagaraRibbonVertexFactory.h"
#include "NiagaraRenderer.h"

class FNiagaraDataSet;

/**
* NiagaraRendererRibbons renders an FNiagaraEmitterInstance as a ribbon connecting all particles
* in order by particle age.
*/
class NIAGARA_API NiagaraRendererRibbons : public NiagaraRenderer
{
public:
	NiagaraRendererRibbons(ERHIFeatureLevel::Type FeatureLevel, UNiagaraRendererProperties *Props);
	~NiagaraRendererRibbons()
	{
		ReleaseRenderThreadResources();
	}

	virtual void ReleaseRenderThreadResources() override;
	virtual void CreateRenderThreadResources() override;


	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector, const FNiagaraSceneProxy *SceneProxy) const override;

	virtual bool SetMaterialUsage() override;

	/** Update render data buffer from attributes */
	FNiagaraDynamicDataBase *GenerateVertexData(const FNiagaraSceneProxy* Proxy, FNiagaraDataSet &Data, const ENiagaraSimTarget Target) override;

	void AddRibbonVert(TArray<FNiagaraRibbonVertex>& RenderData, FVector ParticlePos, FVector2D UV1, FVector2D UV2,
		const FLinearColor &Color, const float Age, const float Rotation, const float Size, const FVector NormDir, const FVector CustomFacing)
	{
		FNiagaraRibbonVertex NewVertex;
		NewVertex.Position = ParticlePos;
		NewVertex.Direction = NormDir;
		NewVertex.Color = Color;
		NewVertex.Size = Size;
		NewVertex.Rotation = Rotation;
		NewVertex.Tex_U = UV1.X * Properties->UV0Scale.X;
		NewVertex.Tex_V = UV1.Y * Properties->UV0Scale.Y;
		NewVertex.Tex_U2 = UV2.X * Properties->UV1Scale.X;
		NewVertex.Tex_V2 = UV2.Y * Properties->UV1Scale.Y;
		NewVertex.CustomFacingVector = CustomFacing;
		//NewVertex.NormalizedAge = Age;
		RenderData.Add(NewVertex);
	}

	void AddDynamicParam(TArray<FNiagaraRibbonVertexDynamicParameter>& ParamData, const FVector4& DynamicParam)
	{
		FNiagaraRibbonVertexDynamicParameter Param;
		Param.DynamicValue[0] = DynamicParam.X;
		Param.DynamicValue[1] = DynamicParam.Y;
		Param.DynamicValue[2] = DynamicParam.Z;
		Param.DynamicValue[3] = DynamicParam.W;
		ParamData.Add(Param);
	}



	virtual void SetDynamicData_RenderThread(FNiagaraDynamicDataBase* NewDynamicData) override;
	int GetDynamicDataSize() override;
	bool HasDynamicData() override;

#if WITH_EDITORONLY_DATA
	virtual const TArray<FNiagaraVariable>& GetRequiredAttributes() override;
	virtual const TArray<FNiagaraVariable>& GetOptionalAttributes() override;
#endif

	UClass *GetPropertiesClass() override { return UNiagaraRibbonRendererProperties::StaticClass(); }
	void SetRendererProperties(UNiagaraRendererProperties *Props) override { Properties = Cast<UNiagaraRibbonRendererProperties>(Props); }

private:
	class FNiagaraRibbonVertexFactory *VertexFactory;
	UNiagaraRibbonRendererProperties *Properties;
	mutable TUniformBuffer<FPrimitiveUniformShaderParameters> WorldSpacePrimitiveUniformBuffer;
};


