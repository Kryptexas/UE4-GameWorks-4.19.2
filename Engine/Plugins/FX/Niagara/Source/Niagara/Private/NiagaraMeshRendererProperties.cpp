// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRendererMeshes.h"
#include "Engine/StaticMesh.h"
#include "NiagaraConstants.h"

UNiagaraMeshRendererProperties::UNiagaraMeshRendererProperties()
	: ParticleMesh(nullptr)
{
}

NiagaraRenderer* UNiagaraMeshRendererProperties::CreateEmitterRenderer(ERHIFeatureLevel::Type FeatureLevel)
{
	return new NiagaraRendererMeshes(FeatureLevel, this);
}

void UNiagaraMeshRendererProperties::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
{
	if (ParticleMesh)
	{
		const FStaticMeshLODResources& LODModel = ParticleMesh->RenderData->LODResources[0];
		if (bOverrideMaterials)
		{
			for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
			{
				const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
				UMaterialInterface* ParticleMeshMaterial = ParticleMesh->GetMaterial(Section.MaterialIndex);

				if (Section.MaterialIndex >= 0 && OverrideMaterials.Num() > Section.MaterialIndex && OverrideMaterials[Section.MaterialIndex] != nullptr)
				{
					OutMaterials.Add(OverrideMaterials[Section.MaterialIndex]);
				}
				else
				{
					OutMaterials.Add(ParticleMeshMaterial);
				}
			}
		}
		else
		{
			for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); SectionIndex++)
			{
				const FStaticMeshSection& Section = LODModel.Sections[SectionIndex];
				UMaterialInterface* ParticleMeshMaterial = ParticleMesh->GetMaterial(Section.MaterialIndex);
				OutMaterials.Add(ParticleMeshMaterial);
			}
		}
	}
}

#if WITH_EDITORONLY_DATA

bool UNiagaraMeshRendererProperties::IsMaterialValidForRenderer(UMaterial* Material, FText& InvalidMessage)
{
	if (Material->bUsedWithNiagaraMeshParticles == false)
	{
		InvalidMessage = NSLOCTEXT("NiagaraMeshRendererProperties", "InvalidMaterialMessage", "The material isn't marked as \"Used with Niagara Mesh particles\"");
		return false;
	}
	return true;
}

void UNiagaraMeshRendererProperties::FixMaterial(UMaterial* Material)
{
	Material->Modify();
	Material->bUsedWithNiagaraMeshParticles = true;
	Material->ForceRecompileForRendering();
}

const TArray<FNiagaraVariable>& UNiagaraMeshRendererProperties::GetRequiredAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		Attrs.Add(SYS_PARAM_PARTICLES_POSITION);
		Attrs.Add(SYS_PARAM_PARTICLES_VELOCITY);
		Attrs.Add(SYS_PARAM_PARTICLES_COLOR);
		Attrs.Add(SYS_PARAM_PARTICLES_NORMALIZED_AGE);
	}

	return Attrs;
}


const TArray<FNiagaraVariable>& UNiagaraMeshRendererProperties::GetOptionalAttributes()
{
	static TArray<FNiagaraVariable> Attrs;

	if (Attrs.Num() == 0)
	{
		Attrs.Add(SYS_PARAM_PARTICLES_SCALE);
		//Attrs.Add(SYS_PARAM_PARTICLES_SPRITE_FACING);
		//Attrs.Add(SYS_PARAM_PARTICLES_SPRITE_ALIGNMENT);
		//Attrs.Add(SYS_PARAM_PARTICLES_SUB_IMAGE_INDEX);
		Attrs.Add(SYS_PARAM_PARTICLES_DYNAMIC_MATERIAL_PARAM);
	}

	return Attrs;
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
				Material->CheckMaterialUsage(MATUSAGE_NiagaraMeshParticles);
			}
		}
	}
}
