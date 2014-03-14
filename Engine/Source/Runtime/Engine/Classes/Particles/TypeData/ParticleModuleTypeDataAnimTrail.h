// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	ParticleModuleTypeDataAnimTrail
 *	Provides the base data for animation-based trail emitters.
 *
 */

#pragma once
#include "ParticleModuleTypeDataAnimTrail.generated.h"

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, meta=(DisplayName = "AnimTrail Data"))
class UParticleModuleTypeDataAnimTrail : public UParticleModuleTypeDataBase
{
	GENERATED_UCLASS_BODY()

	/**
	 *	The name of the socket that supplied the control edge for this emitter.
	 */
	UPROPERTY(EditAnywhere, Category=Anim)
	FName ControlEdgeName;

	//*****************************************************************************
	// General Trail Variables
	//*****************************************************************************
	/**
	 *	The number of sheets to render for the trail.
	 */
	UPROPERTY(EditAnywhere, Category=Trail)
	int32 SheetsPerTrail;

	/** 
	 *	If true, when the system is deactivated, mark trails as dead.
	 *	This means they will still render, but will not have more particles
	 *	added to them, even if the system re-activates...
	 */
	UPROPERTY(EditAnywhere, Category=Trail)
	uint32 bDeadTrailsOnDeactivate:1;

	/** If true, do not join the trail to the source position 		*/
	UPROPERTY(EditAnywhere, Category=Trail)
	uint32 bClipSourceSegement:1;

	/** If true, recalculate the previous tangent when a new particle is spawned */
	UPROPERTY(EditAnywhere, Category=Trail)
	uint32 bEnablePreviousTangentRecalculation:1;

	/** If true, recalculate tangents every frame to allow velocity/acceleration to be applied */
	UPROPERTY(EditAnywhere, Category=Trail)
	uint32 bTangentRecalculationEveryFrame:1;

	//*************************************************************************************************
	// Trail Rendering Variables
	//*************************************************************************************************
	/** If true, render the trail geometry (this should typically be on) */
	UPROPERTY(EditAnywhere, Category=Rendering)
	uint32 bRenderGeometry:1;

	/** If true, render stars at each spawned particle point along the trail */
	UPROPERTY(EditAnywhere, Category=Rendering)
	uint32 bRenderSpawnPoints:1;

	/** If true, render a line showing the tangent at each spawned particle point along the trail */
	UPROPERTY(EditAnywhere, Category=Rendering)
	uint32 bRenderTangents:1;

	/** If true, render the tessellated path between spawned particles */
	UPROPERTY(EditAnywhere, Category=Rendering)
	uint32 bRenderTessellation:1;

	/** 
	 *	The (estimated) covered distance to tile the 2nd UV set at.
	 *	If 0.0, a second UV set will not be passed in.
	 */
	UPROPERTY(EditAnywhere, Category=Rendering)
	float TilingDistance;

	/** 
	 *	The distance step size for tessellation.
	 *	# Tessellation Points = Trunc((Distance Between Spawned Particles) / DistanceTessellationStepSize))
	 */
	UPROPERTY(EditAnywhere, Category=Rendering)
	float DistanceTessellationStepSize;

	/** 
	 *	The tangent scalar for tessellation.
	 *	Angles between tangent A and B are mapped to [0.0f .. 1.0f]
	 *	This is then multiplied by TangentTessellationScalar to give the number of points to tessellate
	 */
	UPROPERTY(EditAnywhere, Category=Rendering)
	float TangentTessellationScalar;


	// Begin UParticleModule Interface
	virtual uint32 RequiredBytes(FParticleEmitterInstance* Owner = NULL) OVERRIDE;
	virtual bool CanTickInAnyThread() OVERRIDE
	{
		return false;
	}
	// End UParticleModule Interface

	// Begin UParticleModuleTypeDataBase Interface
	virtual FParticleEmitterInstance* CreateInstance(UParticleEmitter* InEmitterParent, UParticleSystemComponent* InComponent) OVERRIDE;
	// End UParticleModuleTypeDataBase Interface
};



