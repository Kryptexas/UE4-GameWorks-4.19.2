// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "ViewportInteractionModule.h"
#include "ViewportWorldInteractionManager.h"
#include "ViewportWorldInteraction.h"
#include "ViewportInteractionInputProcessor.h"

FViewportWorldInteractionManager::FViewportWorldInteractionManager() :
	CurrentWorldInteraction( nullptr )
{
	InputProcessor = MakeShareable( new FViewportInteractionInputProcessor( this ) );
	FSlateApplication::Get().SetInputPreProcessor( true, InputProcessor );
}

FViewportWorldInteractionManager::~FViewportWorldInteractionManager()
{
	CurrentWorldInteraction = nullptr;
}

void FViewportWorldInteractionManager::AddReferencedObjects( FReferenceCollector& Collector )
{
	Collector.AddReferencedObject( CurrentWorldInteraction );
}

void FViewportWorldInteractionManager::Tick( float DeltaTime )
{
	if( CurrentWorldInteraction && CurrentWorldInteraction->IsActive() )
	{
		// First give registered functions to tick before the world interaction is ticked
		OnPreWorldInteractionTickEvent.Broadcast( DeltaTime );
		CurrentWorldInteraction->Tick( DeltaTime );
		OnPostWorldInteractionTickEvent.Broadcast( DeltaTime );
 	}
}

bool FViewportWorldInteractionManager::HandleInputKey( const FKey Key, const EInputEvent Event )
{
	bool bResult = false;
	if(CurrentWorldInteraction && CurrentWorldInteraction->IsActive())
	{
		bResult = CurrentWorldInteraction->HandleInputKey( Key, Event );
	}
	return bResult;
}

bool FViewportWorldInteractionManager::HandleInputAxis( const int32 ControllerId, const FKey Key, const float Delta, const float DeltaTime )
{
	bool bResult = false;
	if(CurrentWorldInteraction && CurrentWorldInteraction->IsActive())
	{
		bResult = CurrentWorldInteraction->HandleInputAxis( ControllerId, Key, Delta, DeltaTime );
	}
	return bResult;
}

void FViewportWorldInteractionManager::SetCurrentViewportWorldInteraction( UViewportWorldInteraction* WorldInteraction )
{
	CurrentWorldInteraction = WorldInteraction;
}
