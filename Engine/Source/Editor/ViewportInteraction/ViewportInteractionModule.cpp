// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ViewportInteractionModule.h"
#include "ViewportWorldInteractionManager.h"

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