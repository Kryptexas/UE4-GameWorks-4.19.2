// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

class FNavDataGenerator : public TSharedFromThis<FNavDataGenerator, ESPMode::ThreadSafe>
{
public:
	virtual ~FNavDataGenerator() {}

	/** 
	 * Needs to be called on main thread (may need to reallocate main-mem containers and iterate actor list)
	 */
	virtual bool Generate() PURE_VIRTUAL(FNavDataGenerator::Generate, return false;);

	/** Asks generator to update navigation affected by DirtyAreas */
	virtual void RebuildDirtyAreas(const TArray<FNavigationDirtyArea>& DirtyAreas) {}

	/** Rebuilds all known navigation data */
	virtual void RebuildAll() PURE_VIRTUAL(FNavDataGenerator::RebuildAll, Generate(););

	/** 
	 * Schedules generation call in GameThread
	 */
	virtual void TiggerGeneration();
	
	/** determines whether this generator is performing navigation building actions at the moment*/
	virtual bool IsBuildInProgress(bool bCheckDirtyToo = false) const { return false; }	

	/** Called when world is loaded and all actors are initialized - to be used for post-load steps like data validating */
	virtual void OnWorldInitDone(bool bAllowedToRebuild) {}	

	/** called from owner's BeginPlay function */
	virtual void OnPostBeginPlay() {}

	virtual void OnNavigationBuildingLocked() {}

	/** Lets Generator know that building navigation is now allowed. 
	 *	@param bForce if true means it's requested from this Generator to 
	 *	actually rebuild relevant data*/
	virtual void OnNavigationBuildingUnlocked(bool bForce) {}

	virtual void OnNavigationBoundsUpdated(class AVolume* Volume) {}

	/** notifies navigation generator that given ANavigationData is being destroyed
	 *	and it's possible it's its parent 
	 */
	virtual void OnNavigationDataDestroyed(class ANavigationData* NavData) {}

	//----------------------------------------------------------------------//
	// debug
	//----------------------------------------------------------------------//
	virtual uint32 LogMemUsed() const { return 0; }

#if WITH_EDITOR
	virtual void ExportNavigationData(const FString& FileName) const {}
#endif

};