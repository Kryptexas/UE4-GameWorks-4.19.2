// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ParticleModuleRequired.generated.h"

UENUM()
enum EParticleSortMode
{
	PSORTMODE_None,
	PSORTMODE_ViewProjDepth,
	PSORTMODE_DistanceToView,
	PSORTMODE_Age_OldestFirst,
	PSORTMODE_Age_NewestFirst,
	PSORTMODE_MAX,
};

UENUM()
enum EEmitterNormalsMode
{
	/** Default mode, normals are based on the camera facing geometry. */
	ENM_CameraFacing,
	/** Normals are generated from a sphere centered at NormalsSphereCenter. */
	ENM_Spherical,
	/** Normals are generated from a cylinder going through NormalsSphereCenter, in the direction NormalsCylinderDirection. */
	ENM_Cylindrical,
	ENM_MAX,
};

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=(Object, Cascade), meta=(DisplayName = "Required"), MinimalAPI)
class UParticleModuleRequired : public UParticleModule
{
	GENERATED_UCLASS_BODY()

	//
	// General.
	// 
	
	/** The material to utilize for the emitter at this LOD level.						*/
	UPROPERTY(EditAnywhere, Category=Emitter)
	class UMaterialInterface* Material;

	UPROPERTY(EditAnywhere, Category=Emitter)
	FVector EmitterOrigin;
	UPROPERTY(EditAnywhere, Category=Emitter)
	FRotator EmitterRotation;

	/** 
	 *	The screen alignment to utilize for the emitter at this LOD level.
	 *	One of the following:
	 *	PSA_FacingCameraPosition - Faces the camera position, but is not dependent on the camera rotation.  
	 *								This method produces more stable particles under camera rotation.
	 *	PSA_Square			- Uniform scale (via SizeX) facing the camera
	 *	PSA_Rectangle		- Non-uniform scale (via SizeX and SizeY) facing the camera
	 *	PSA_Velocity		- Orient the particle towards both the camera and the direction 
	 *						  the particle is moving. Non-uniform scaling is allowed.
	 *	PSA_TypeSpecific	- Use the alignment method indicated int he type data module.
	 */
	UPROPERTY(EditAnywhere, Category=Emitter)
	TEnumAsByte<enum EParticleScreenAlignment> ScreenAlignment;

	/** If true, update the emitter in local space										*/
	UPROPERTY(EditAnywhere, Category=Emitter)
	uint32 bUseLocalSpace:1;

	/** If true, kill the emitter when the particle system is deactivated				*/
	UPROPERTY(EditAnywhere, Category=Emitter)
	uint32 bKillOnDeactivate:1;

	/** If true, kill the emitter when it completes										*/
	UPROPERTY(EditAnywhere, Category=Emitter)
	uint32 bKillOnCompleted:1;

	/** Whether this emitter requires sorting as specified by artist.					*/
	UPROPERTY()
	uint32 bRequiresSorting_DEPRECATED:1;

	/**
	 *	The sorting mode to use for this emitter.
	 *	PSORTMODE_None				- No sorting required.
	 *	PSORTMODE_ViewProjDepth		- Sort by view projected depth of the particle.
	 *	PSORTMODE_DistanceToView	- Sort by distance of particle to view in world space.
	 *	PSORTMODE_Age_OldestFirst	- Sort by age, oldest drawn first.
	 *	PSORTMODE_Age_NewestFirst	- Sort by age, newest drawn first.
	 *
	 */
	UPROPERTY(EditAnywhere, Category=Emitter)
	TEnumAsByte<enum EParticleSortMode> SortMode;

	/**
	 *	If true, the EmitterTime for the emitter will be calculated by
	 *	modulating the SecondsSinceCreation by the EmitterDuration. As
	 *	this can lead to issues w/ looping and variable duration, a new
	 *	approach has been implemented. 
	 *	If false, this new approach is utilized, and the EmitterTime is
	 *	simply incremented by DeltaTime each tick. When the emitter 
	 *	loops, it adjusts the EmitterTime by the current EmitterDuration
	 *	resulting in proper looping/delay behavior.
	 */
	UPROPERTY(EditAnywhere, Category=Emitter)
	uint32 bUseLegacyEmitterTime:1;

	/** 
	 *	How long, in seconds, the emitter will run before looping.
	 */
	UPROPERTY(EditAnywhere, Category=Duration)
	float EmitterDuration;

	/** 
	 *	The low end of the emitter duration if using a range.
	 */
	UPROPERTY(EditAnywhere, Category=Duration)
	float EmitterDurationLow;

	/**
	 *	If true, select the emitter duration from the range 
	 *		[EmitterDurationLow..EmitterDuration]
	 */
	UPROPERTY(EditAnywhere, Category=Duration)
	uint32 bEmitterDurationUseRange:1;

	/** 
	 *	If true, recalculate the emitter duration on each loop.
	 */
	UPROPERTY(EditAnywhere, Category=Duration)
	uint32 bDurationRecalcEachLoop:1;

	/** The number of times to loop the emitter.
	 *	0 indicates loop continuously
	 */
	UPROPERTY(EditAnywhere, Category=Duration)
	int32 EmitterLoops;

	//
	// Spawn-related.
	//
	
	/** The rate at which to spawn particles									*/
	UPROPERTY()
	struct FRawDistributionFloat SpawnRate;

	//
	// Burst-related.
	//
	/** The method to utilize when burst-emitting particles						*/
	UPROPERTY()
	TEnumAsByte<enum EParticleBurstMethod> ParticleBurstMethod;

	/** The array of burst entries.												*/
	UPROPERTY(export, noclear)
	TArray<struct FParticleBurst> BurstList;

	//
	// Delay-related.
	//
	
	/**
	 *	Indicates the time (in seconds) that this emitter should be delayed in the particle system.
	 */
	UPROPERTY(EditAnywhere, Category=Delay)
	float EmitterDelay;

	/** 
	 *	The low end of the emitter delay if using a range.
	 */
	UPROPERTY(EditAnywhere, Category=Delay)
	float EmitterDelayLow;

	/**
	 *	If true, select the emitter delay from the range 
	 *		[EmitterDelayLow..EmitterDelay]
	 */
	UPROPERTY(EditAnywhere, Category=Delay)
	uint32 bEmitterDelayUseRange:1;

	/**
	 *	If true, the emitter will be delayed only on the first loop.
	 */
	UPROPERTY(EditAnywhere, Category=Delay)
	uint32 bDelayFirstLoopOnly:1;

	//
	// SubUV-related.
	//
	
	/** 
	 *	The interpolation method to used for the SubUV image selection.
	 *	One of the following:
	 *	PSUVIM_None			- Do not apply SubUV modules to this emitter. 
	 *	PSUVIM_Linear		- Smoothly transition between sub-images in the given order, 
	 *						  with no blending between the current and the next
	 *	PSUVIM_Linear_Blend	- Smoothly transition between sub-images in the given order, 
	 *						  blending between the current and the next 
	 *	PSUVIM_Random		- Pick the next image at random, with no blending between 
	 *						  the current and the next 
	 *	PSUVIM_Random_Blend	- Pick the next image at random, blending between the current 
	 *						  and the next 
	 */
	UPROPERTY(EditAnywhere, Category=SubUV)
	TEnumAsByte<enum EParticleSubUVInterpMethod> InterpolationMethod;

	/** The number of sub-images horizontally in the texture							*/
	UPROPERTY(EditAnywhere, Category=SubUV)
	int32 SubImages_Horizontal;

	/** The number of sub-images vertically in the texture								*/
	UPROPERTY(EditAnywhere, Category=SubUV)
	int32 SubImages_Vertical;

	/** Whether to scale the UV or not - ie, the model wasn't setup with sub uvs		*/
	UPROPERTY(EditAnywhere, Category=SubUV)
	uint32 bScaleUV:1;

	/**
	 *	The amount of time (particle-relative, 0.0 to 1.0) to 'lock' on a random sub image
	 *	    0.0 = change every frame
	 *      1.0 = select a random image at spawn and hold for the life of the particle
	 */
	UPROPERTY()
	float RandomImageTime;

	/** The number of times to change a random image over the life of the particle.		*/
	UPROPERTY(EditAnywhere, Category=SubUV)
	int32 RandomImageChanges;

	/** Override the system MacroUV settings                                            */
	UPROPERTY(EditAnywhere, Category=MacroUV)
	uint32 bOverrideSystemMacroUV:1;

	/** Local space position that UVs generated with the ParticleMacroUV material node will be centered on. */
	UPROPERTY(EditAnywhere, Category=MacroUV)
	FVector MacroUVPosition;

	/** World space radius that UVs generated with the ParticleMacroUV material node will tile based on. */
	UPROPERTY(EditAnywhere, Category=MacroUV)
	float MacroUVRadius;

	/**
	 *	If true, use the MaxDrawCount to limit the number of particles rendered.
	 *	NOTE: This does not limit the number spawned/updated, only what is drawn.
	 */
	UPROPERTY(EditAnywhere, Category=Rendering)
	uint32 bUseMaxDrawCount:1;

	/**
	 *	The maximum number of particles to DRAW for this emitter.
	 *	If set to 0, it will use whatever number are present.
	 */
	UPROPERTY(EditAnywhere, Category=Rendering)
	int32 MaxDrawCount;

	/** Normal generation mode for this emitter LOD. */
	UPROPERTY(EditAnywhere, Category=Normals)
	TEnumAsByte<enum EEmitterNormalsMode> EmitterNormalsMode;

	/** 
	 * When EmitterNormalsMode is ENM_Spherical, particle normals are created to face away from NormalsSphereCenter. 
	 * NormalsSphereCenter is in local space.
	 */
	UPROPERTY(EditAnywhere, Category=Normals)
	FVector NormalsSphereCenter;

	/** 
	 * When EmitterNormalsMode is ENM_Cylindrical, 
	 * particle normals are created to face away from the cylinder going through NormalsSphereCenter in the direction NormalsCylinderDirection. 
	 * NormalsCylinderDirection is in local space.
	 */
	UPROPERTY(EditAnywhere, Category=Normals)
	FVector NormalsCylinderDirection;

	/**
	 * Ensures that movement generated from the orbit module is applied to velocity-aligned particles
	 */
	UPROPERTY(EditAnywhere, Category=Emitter)
	uint32 bOrbitModuleAffectsVelocityAlignment:1;

	/** Initializes the default values for this property */
	void InitializeDefaults();

	// Begin UObject Interface
#if WITH_EDITOR
	virtual void	PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual bool IsValidForLODLevel(UParticleLODLevel* LODLevel, FString& OutErrorString) OVERRIDE;
#endif // WITH_EDITOR
	virtual void	PostLoad() OVERRIDE;
	virtual void	PostInitProperties() OVERRIDE;
	virtual void	Serialize(FArchive& Ar) OVERRIDE;
	// End UObject Interface

	// Begin UParticleModule Interface
	virtual void SetToSensibleDefaults(UParticleEmitter* Owner) OVERRIDE;
	virtual	bool AddModuleCurvesToEditor(UInterpCurveEdSetup* EdSetup, TArray<const FCurveEdEntry*>& OutCurveEntries) OVERRIDE
	{
		// Overide the base implementation to prevent old SpawnRate from being added...
		return true;
	}
	virtual EModuleType	GetModuleType() const OVERRIDE {	return EPMT_Required;	}
	virtual bool	GenerateLODModuleValues(UParticleModule* SourceModule, float Percentage, UParticleLODLevel* LODLevel) OVERRIDE;
	// End UParticleModule Interface

protected:
	friend class FParticleModuleRequiredDetails;
};



