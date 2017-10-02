#include "FlexParticleSystemComponent.h"
#include "FlexFluidSurface.h"
#include "FlexContainer.h"
#include "GameWorks/IFlexPluginBridge.h"
#include "Particles/ParticleEmitter.h"
#include "FlexParticleEmitter.h"
#include "FlexContainerInstance.h"

UFlexParticleSystemComponent::UFlexParticleSystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FlexFluidSurfaceOverride = NULL;
}

UFlexContainer* UFlexParticleSystemComponent::GetFirstFlexContainerTemplate()
{
	for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
	{
		FParticleEmitterInstance* EmitterInstance = EmitterInstances[EmitterIndex];
		if (EmitterInstance && EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
			if (FlexEmitter && FlexEmitter->FlexContainerTemplate)
			{
				FPhysScene* Scene = EmitterInstance->Component->GetWorld()->GetPhysicsScene();

				FFlexContainerInstance* ContainerInstance = GFlexPluginBridge->GetFlexContainer(Scene, FlexEmitter->FlexContainerTemplate);
				return ContainerInstance ? ContainerInstance->Template : nullptr;
			}
		}
	}
	return nullptr;
}

UMaterialInstanceDynamic* UFlexParticleSystemComponent::CreateFlexDynamicMaterialInstance(class UMaterialInterface* SourceMaterial)
{
	if (!SourceMaterial)
	{
		return NULL;
	}

	for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
	{
		FParticleEmitterInstance* EmitterInstance = EmitterInstances[EmitterIndex];
		if (EmitterInstance && 	EmitterInstance->SpriteTemplate)
		{
			auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
			if (FlexEmitter && FlexEmitter->FlexFluidSurfaceTemplate && FlexEmitter->FlexFluidSurfaceTemplate->Material)
			{
				UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(SourceMaterial);

				if (!MID)
				{
					// Create and set the dynamic material instance.
					MID = UMaterialInstanceDynamic::Create(SourceMaterial, this);
				}

				if (MID)
				{
					// Make a copy of the FlexFluidSurfaceTemplate
					FlexFluidSurfaceOverride = DuplicateObject<UFlexFluidSurface>(FlexEmitter->FlexFluidSurfaceTemplate, this);
					// Set the material in the new FlexFluidSurfaceTemplate
					FlexFluidSurfaceOverride->Material = MID;
					// Tell the ParticleEmiterInstance to update its FlexFluidSurfaceComponent
					GFlexPluginBridge->RegisterNewFlexFluidSurfaceComponent(EmitterInstance, FlexFluidSurfaceOverride);
				}
				else
				{
					UE_LOG(LogFlex, Warning, TEXT("CreateFlexDynamicMaterialInstance on %s: Material is invalid."), *GetPathName());
				}

				return MID;
			}
		}
	}
	return NULL;
}

void UFlexParticleSystemComponent::UpdateFlexSurfaceDynamicData(FParticleEmitterInstance* EmitterInstance, FDynamicEmitterDataBase* EmitterDynamicData)
{
	check(EmitterInstance);
	check(EmitterDynamicData);

	if (SceneProxy)
	{
		auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
		auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;

		UFlexFluidSurface* FlexFluidSurface = FlexFluidSurfaceOverride ? FlexFluidSurfaceOverride : FlexFluidSurfaceTemplate;
		if (FlexFluidSurface)
		{
			UFlexFluidSurfaceComponent* SurfaceComponent = GFlexPluginBridge->GetFlexFluidSurface(GetWorld(), FlexFluidSurface);
			check(SurfaceComponent);

			SurfaceComponent->SendRenderEmitterDynamicData_Concurrent((FParticleSystemSceneProxy*)SceneProxy, EmitterDynamicData);
		}
	}
}

void UFlexParticleSystemComponent::ClearFlexSurfaceDynamicData()
{
	if (SceneProxy)
	{
		for (int32 EmitterIndex = 0; EmitterIndex < EmitterInstances.Num(); EmitterIndex++)
		{
			FParticleEmitterInstance* EmitterInstance = EmitterInstances[EmitterIndex];
			if (EmitterInstance)
			{
				auto FlexEmitter = Cast<UFlexParticleEmitter>(EmitterInstance->SpriteTemplate);
				auto FlexFluidSurfaceTemplate = FlexEmitter ? FlexEmitter->FlexFluidSurfaceTemplate : nullptr;

				UFlexFluidSurface* FlexFluidSurface = FlexFluidSurfaceOverride ? FlexFluidSurfaceOverride : FlexFluidSurfaceTemplate;
				UFlexFluidSurfaceComponent* SurfaceComponent = GFlexPluginBridge->GetFlexFluidSurface(GetWorld(), FlexFluidSurface);
				if (SurfaceComponent)
				{
					SurfaceComponent->SendRenderEmitterDynamicData_Concurrent((FParticleSystemSceneProxy*)SceneProxy, nullptr);
				}
			}
		}
	}
}
