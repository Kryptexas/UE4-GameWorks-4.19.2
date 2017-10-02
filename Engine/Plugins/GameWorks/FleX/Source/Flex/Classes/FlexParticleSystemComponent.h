#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Particles/ParticleSystemComponent.h"
#include "FlexParticleSystemComponent.generated.h"

UCLASS(ClassGroup=(Rendering, Common), hidecategories=Object, hidecategories=Physics, hidecategories=Collision, showcategories=Trigger, editinlinenew, meta=(BlueprintSpawnableComponent))
class FLEX_API UFlexParticleSystemComponent : public UParticleSystemComponent
{
	GENERATED_UCLASS_BODY()

	/**
	* Creates a Dynamic Material Instance for the specified flex material from the supplied material.
	*/
	UFUNCTION(BlueprintCallable, Category = "Rendering|Material")
	virtual class UMaterialInstanceDynamic* CreateFlexDynamicMaterialInstance(class UMaterialInterface* SourceMaterial);

	/** To support the creation of a dynamic material instance for Flex emitters, we need to make a new FlexFluidSurface instance */
	UPROPERTY()
	class UFlexFluidSurface* FlexFluidSurfaceOverride;

	/**
	* Get all of the FleX container templates from all of the emitter instances
	*/
	UFUNCTION(BlueprintCallable, Category = "Flex")
	class UFlexContainer* GetFirstFlexContainerTemplate();

	virtual void UpdateFlexSurfaceDynamicData(FParticleEmitterInstance* EmitterInstance, FDynamicEmitterDataBase* EmitterDynamicData);
	virtual void ClearFlexSurfaceDynamicData();
};
