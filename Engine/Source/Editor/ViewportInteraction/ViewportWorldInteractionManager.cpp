// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "ViewportWorldInteractionManager.h"
#include "Framework/Application/SlateApplication.h"
#include "ViewportWorldInteraction.h"

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

bool FViewportWorldInteractionManager::PreprocessedInputKey( const FKey Key, const EInputEvent Event )
{
	bool bResult = false;
	if( CurrentWorldInteraction && CurrentWorldInteraction->IsActive() )
	{
		bResult = CurrentWorldInteraction->PreprocessedInputKey( Key, Event );
	}
	return bResult;
}

bool FViewportWorldInteractionManager::PreprocessedInputAxis( const int32 ControllerId, const FKey Key, const float Delta, const float DeltaTime )
{
	bool bResult = false;
	if(CurrentWorldInteraction && CurrentWorldInteraction->IsActive() )
	{
		bResult = CurrentWorldInteraction->PreprocessedInputAxis( ControllerId, Key, Delta, DeltaTime );
	}
	return bResult;
}

void FViewportWorldInteractionManager::SetCurrentViewportWorldInteraction( UViewportWorldInteraction* WorldInteraction )
{
	CurrentWorldInteraction = WorldInteraction;
}
