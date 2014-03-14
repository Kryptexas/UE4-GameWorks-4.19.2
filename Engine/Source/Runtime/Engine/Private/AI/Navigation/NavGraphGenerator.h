// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "../../Public/AI/NavDataGenerator.h"

/**
 * Class that handles generation of the ANavigationGraph data
 */
class FNavGraphGenerator : public FNavDataGenerator
{
public:
	FNavGraphGenerator(class ANavigationGraph* InDestNavGraph);
	virtual ~FNavGraphGenerator();

private:
	/** Prevent copying. */
	FNavGraphGenerator(FNavGraphGenerator const& NoCopy) { check(0); };
	FNavGraphGenerator& operator=(FNavGraphGenerator const& NoCopy) { check(0); return *this; }

public:
	//----------------------------------------------------------------------//
	// FNavDataGenerator overrides
	//----------------------------------------------------------------------//
	// Triggers navmesh building process
	virtual bool Generate() OVERRIDE;

	virtual bool IsBuildInProgress(bool bCheckDirtyToo = false) const OVERRIDE;

	virtual void OnWorldInitDone(bool bAllowedToRebuild) OVERRIDE;

	virtual void OnPostBeginPlay() OVERRIDE;
		
	//--- accessors --- //

private:
	// Performs initial setup of member variables so that generator is ready to do its thing from this point on
	void Init();

	bool IsBuildingLocked() const;

	void CleanUpIntermediateData();

	void UpdateBuilding();
	
	void TiggerGeneration();

public:
	virtual void OnNavigationBuildingLocked() OVERRIDE;
	virtual void OnNavigationBuildingUnlocked(bool bForce) OVERRIDE;
	virtual void OnNavigationBoundsUpdated(class AVolume* Volume) OVERRIDE;

	virtual void OnNavigationDataDestroyed(class ANavigationData* NavData) OVERRIDE;

private:
	/** Bounding geometry definition. */
	TArray<AVolume const*> InclusionVolumes;

	FCriticalSection GraphChangingLock;

	class ANavigationGraph* DestNavGraph;

	uint32 bInitialized:1;
	uint32 bBuildFromScratchRequested:1;

	mutable uint32 bBuildingLocked:1;
};
