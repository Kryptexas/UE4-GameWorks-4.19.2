// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleSubUV.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, MinimalAPI, meta=(DisplayName = "SubImage Index"))
class UParticleModuleSubUV : public UParticleModuleSubUVBase
{
	GENERATED_UCLASS_BODY()

	/**
	 *	The index of the sub-image that should be used for the particle.
	 *	The value is retrieved using the RelativeTime of the particles.
	 */
	UPROPERTY(EditAnywhere, Category=SubUV)
	struct FRawDistributionFloat SubImageIndex;

	/** 
	 *	If true, use *real* time when updating the image index.
	 *	The movie will update regardless of the slomo settings of the game.
	 */
	UPROPERTY(EditAnywhere, Category=Realtime)
	uint32 bUseRealTime:1;

	/** Initializes the default values for this property */
	void InitializeDefaults();
	
	//Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif // WITH_EDITOR
	virtual void	PostInitProperties() OVERRIDE;
	virtual void	Serialize(FArchive& Ar) OVERRIDE;
	//End UObject Interface

	// Begin UParticleModule Interface
	virtual void CompileModule( struct FParticleEmitterBuildInfo& EmitterInfo ) OVERRIDE;
	virtual void Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase) OVERRIDE;
	virtual void Update(FParticleEmitterInstance* Owner, int32 Offset, float DeltaTime) OVERRIDE;
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) OVERRIDE;
	// End UParticleModule Interface

	/**
	 *	Determine the current image index to use...
	 *
	 *	@param	Owner					The emitter instance being updated.
	 *	@param	Offset					The offset to the particle payload for this module.
	 *	@param	Particle				The particle that the image index is being determined for.
	 *	@param	InterpMethod			The EParticleSubUVInterpMethod method used to update the subUV.
	 *	@param	SubUVPayload			The FFullSubUVPayload for this particle.
	 *
	 *	@return	float					The image index with interpolation amount as the fractional portion.
	 */
	virtual float DetermineImageIndex(FParticleEmitterInstance* Owner, int32 Offset, FBaseParticle* Particle, 
		EParticleSubUVInterpMethod InterpMethod, FFullSubUVPayload& SubUVPayload, float DeltaTime);

#if WITH_EDITOR
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) OVERRIDE;
#endif

protected:
	friend class FParticleModuleSubUVDetails;
};



