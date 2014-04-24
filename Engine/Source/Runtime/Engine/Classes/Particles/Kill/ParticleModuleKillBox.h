// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleKillBox.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "Kill Box"))
class UParticleModuleKillBox : public UParticleModuleKillBase
{
	GENERATED_UCLASS_BODY()

	/** The lower left corner of the box. */
	UPROPERTY(EditAnywhere, Category=Kill)
	struct FRawDistributionVector LowerLeftCorner;

	/** The upper right corner of the box. */
	UPROPERTY(EditAnywhere, Category=Kill)
	struct FRawDistributionVector UpperRightCorner;

	/** If true, the box coordinates are in world space./ */
	UPROPERTY(EditAnywhere, Category=Kill)
	uint32 bAbsolute:1;

	/**
	 *	If true, particles INSIDE the box will be killed. 
	 *	If false (the default), particles OUTSIDE the box will be killed.
	 */
	UPROPERTY(EditAnywhere, Category=Kill)
	uint32 bKillInside:1;

	/** If true, the box will always be axis aligned and non-scalable. */
	UPROPERTY(EditAnywhere, Category=Kill)
	uint32 bAxisAlignedAndFixedSize:1;

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
	virtual void	Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual void	Render3DPreview(FParticleEmitterInstance* Owner, const FSceneView* View,FPrimitiveDrawInterface* PDI) OVERRIDE;
	//End UParticleModule Interface
};



