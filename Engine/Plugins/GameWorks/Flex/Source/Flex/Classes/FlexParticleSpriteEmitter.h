#pragma once
#include "Particles/ParticleSpriteEmitter.h"
#include "FlexAsset.h"
#include "FlexParticleSpriteEmitter.generated.h"


UCLASS(collapsecategories, hidecategories=Object, editinlinenew, MinimalAPI)
class UFlexParticleSpriteEmitter : public UParticleSpriteEmitter
{
	GENERATED_UCLASS_BODY()

	/** The Flex container to emit into */
	UPROPERTY(EditAnywhere, Category = Flex)
	class UFlexContainer* FlexContainerTemplate;

	/** Phase assigned to spawned Flex particles */
	UPROPERTY(EditAnywhere, Category = Flex)
	FFlexPhase Phase;

	/** Enable local-space simulation when parented */
	UPROPERTY(EditAnywhere, Category = Flex)
	uint32 bLocalSpace : 1;

	/** Control Local Inertial components */
	UPROPERTY(EditAnywhere, Category = Flex)
	FFlexInertialScale InertialScale;

	/** Mass assigned to Flex particles */
	UPROPERTY(EditAnywhere, Category = Flex)
	float Mass;

	/** Not supported anymore! See FlexContainer for FlexFluidSurface property.*/
	UPROPERTY(EditAnywhere, Category = Flex)
	class UFlexFluidSurface* FlexFluidSurfaceTemplate;

	FLEX_API static const FName DefaultEmitterName;
};
