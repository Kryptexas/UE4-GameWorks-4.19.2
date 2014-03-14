// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleAttractorPoint.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "Point Attractor"))
class UParticleModuleAttractorPoint : public UParticleModuleAttractorBase
{
	GENERATED_UCLASS_BODY()

	/**	The position of the point attractor from the source of the emitter.		*/
	UPROPERTY(EditAnywhere, Category=Attractor)
	struct FRawDistributionVector Position;

	/**	The radial range of the attractor.										*/
	UPROPERTY(EditAnywhere, Category=Attractor)
	struct FRawDistributionFloat Range;

	/**	The strength of the point attractor.									*/
	UPROPERTY(EditAnywhere, Category=Attractor)
	struct FRawDistributionFloat Strength;

	/**	The strength curve is a function of distance or of time.				*/
	UPROPERTY(EditAnywhere, Category=Attractor)
	uint32 StrengthByDistance:1;

	/**	If true, the velocity adjustment will be applied to the base velocity.	*/
	UPROPERTY(EditAnywhere, Category=Attractor)
	uint32 bAffectBaseVelocity:1;

	/**	If true, set the velocity.												*/
	UPROPERTY(EditAnywhere, Category=Attractor)
	uint32 bOverrideVelocity:1;

	/**	If true, treat the position as world space.  So don't transform the the point to localspace. */
	UPROPERTY(EditAnywhere, Category=Attractor)
	uint32 bUseWorldSpacePosition:1;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void PostInitProperties() OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	//Begin UParticleModule Interface
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual void Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI) OVERRIDE;
	//End UParticleModule Interface
};



