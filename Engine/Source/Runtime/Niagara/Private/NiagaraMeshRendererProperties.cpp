// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "NiagaraMeshRendererProperties.h"
#include "NiagaraEffectRendererMeshes.h"

UNiagaraMeshRendererProperties::UNiagaraMeshRendererProperties()
	: ParticleMesh(nullptr)
{
}

NiagaraEffectRenderer* UNiagaraMeshRendererProperties::CreateEffectRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraEffectRendererMeshes(FeatureLevel, this);
}

void UNiagaraMeshRendererProperties::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	if (ParticleMesh)
	{
		for(FStaticMaterial& Mat : ParticleMesh->StaticMaterials)
		{
			OutMaterials.Add(Mat.MaterialInterface);
		}
	}
}

#if WITH_EDITORONLY_DATA

bool UNiagaraMeshRendererProperties::IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage)
{
	if (Material->bUsedWithMeshParticles == false)
	{
		InvalidMessage = NSLOCTEXT("NiagaraMeshRendererProperties", "InvalidMaterialMessage", "The material isn't marked as \"Used with mesh particles\"");
		return false;
	}
	return true;
}

void UNiagaraMeshRendererProperties::FixMaterial(UMaterial* Material)
{
	Material->Modify();
	Material->bUsedWithMeshParticles = true;
	Material->ForceRecompileForRendering();
}

#endif // WITH_EDITORONLY_DATA

void UNiagaraMeshRendererProperties::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (ParticleMesh && PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetName() == "ParticleMesh")
	{
		const FStaticMeshLODResources& LODModel = ParticleMesh->RenderData->LODResources[0];
		for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
		{
			const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
			UMaterialInterface *Material = ParticleMesh->GetMaterial(Section.MaterialIndex);
			if (Material)
			{
				FMaterialRenderProxy* MaterialProxy = Material->GetRenderProxy(false, false);
				Material->CheckMaterialUsage(MATUSAGE_MeshParticles);
			}
		}
	}
}
