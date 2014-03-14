// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "NavGraphGenerator.h"

//----------------------------------------------------------------------//
// FNavGraphGenerator
//----------------------------------------------------------------------//
FNavGraphGenerator::FNavGraphGenerator(ANavigationGraph* InDestNavGraph)
{

}

FNavGraphGenerator::~FNavGraphGenerator()
{

}

//----------------------------------------------------------------------//
// FNavDataGenerator overrides
//----------------------------------------------------------------------//
bool FNavGraphGenerator::Generate() 
{
	return true;
}

bool FNavGraphGenerator::IsBuildInProgress(bool bCheckDirtyToo) const
{
	return false;
}

void FNavGraphGenerator::OnWorldInitDone(bool bAllowedToRebuild)
{

}

void FNavGraphGenerator::OnPostBeginPlay()
{

}

void FNavGraphGenerator::OnNavigationBuildingLocked() 
{

}

void FNavGraphGenerator::OnNavigationBuildingUnlocked(bool bForce) 
{

}

void FNavGraphGenerator::OnNavigationBoundsUpdated(class AVolume* Volume) 
{

}

void FNavGraphGenerator::OnNavigationDataDestroyed(class ANavigationData* NavData) 
{

}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
void FNavGraphGenerator::Init()
{

}

bool FNavGraphGenerator::IsBuildingLocked() const
{
	return false;
}

void FNavGraphGenerator::CleanUpIntermediateData()
{

}

void FNavGraphGenerator::UpdateBuilding()
{

}

void FNavGraphGenerator::TiggerGeneration()
{

}
