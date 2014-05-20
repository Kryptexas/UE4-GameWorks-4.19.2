// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"

//////////////////////////////////////////////////////////////////////////
// UBodySetup2D

UBodySetup2D::UBodySetup2D(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}


#if WITH_EDITOR

void UBodySetup2D::InvalidatePhysicsData()
{
	//ClearPhysicsMeshes();
	//BodySetupGuid = FGuid::NewGuid(); // change the guid
	//CookedFormatData.FlushData();
}

#endif


void UBodySetup2D::CreatePhysicsMeshes()
{
}
