// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ViewportInteractionModule.h"

FViewportInteractionModule::FViewportInteractionModule()
{
}

FViewportInteractionModule::~FViewportInteractionModule()
{
}

IMPLEMENT_MODULE( FViewportInteractionModule, ViewportInteraction )

void FViewportInteractionModule::StartupModule()
{
}

void FViewportInteractionModule::ShutdownModule()
{
}

void FViewportInteractionModule::PostLoadCallback()
{
}

FViewportWorldInteractionManager& FViewportInteractionModule::GetWorldInteractionManager()
{
	return WorldInteractionManager;
}

void FViewportInteractionModule::Tick( float DeltaTime )
{
	WorldInteractionManager.Tick( DeltaTime );
}
