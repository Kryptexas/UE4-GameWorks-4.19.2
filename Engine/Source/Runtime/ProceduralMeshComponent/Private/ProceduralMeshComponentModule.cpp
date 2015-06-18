// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ProceduralMeshComponentPrivatePCH.h"

#include "IProceduralMeshComponentModule.h"

class FProceduralMeshComponent : public IProceduralMeshComponent
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FProceduralMeshComponent, ProceduralMeshComponent )



void FProceduralMeshComponent::StartupModule()
{
	
}


void FProceduralMeshComponent::ShutdownModule()
{
	
}



