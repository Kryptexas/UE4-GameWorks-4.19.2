// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 *	ParticleModuleBeamBase
 *
 *	The base class for all Beam modules.
 *
 */

#pragma once
#include "ParticleModuleBeamBase.generated.h"

/** The method to use in determining the source/target. */
UENUM()
enum Beam2SourceTargetMethod
{
	/** Default	- use the distribution. 
	 *	This is the fallback for when other modes can't be resolved.
	 */
	PEB2STM_Default,
	/** UserSet	- use the user set value. 
	 *	Primarily intended for weapon effects.
	 */
	PEB2STM_UserSet,
	/** Emitter	- use the emitter position as the source/target.
	 */
	PEB2STM_Emitter,
	/** Particle	- use the particles from a given emitter in the system.		
	 *	The name of the emitter should be set in <Source/Target>Name.
	 */
	PEB2STM_Particle,
	/** Actor		- use the actor as the source/target.
	 *	The name of the actor should be set in <Source/Target>Name.
	 */
	PEB2STM_Actor,
	PEB2STM_MAX,
};

/** The method to use in determining the source/target tangent. */
UENUM()
enum Beam2SourceTargetTangentMethod
{
	/** Direct - a direct line between source and target.				 */
	PEB2STTM_Direct,
	/** UserSet	- use the user set value.								 */
	PEB2STTM_UserSet,
	/** Distribution - use the distribution.							 */
	PEB2STTM_Distribution,
	/** Emitter	- use the emitter direction.							 */
	PEB2STTM_Emitter,
	PEB2STTM_MAX,
};

UCLASS(HeaderGroup=Particle, editinlinenew, hidecategories=Object, abstract, meta=(DisplayName = "Beam"))
class UParticleModuleBeamBase : public UParticleModule
{
	GENERATED_UCLASS_BODY()


	//Begin UParticleModule Interface
	virtual EModuleType	GetModuleType() const OVERRIDE {	return EPMT_Beam;	}

	virtual bool CanTickInAnyThread() OVERRIDE
	{
		return false;
	}
	//End UParticleModule Interface
};



