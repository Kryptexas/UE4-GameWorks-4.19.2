// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SceneOutlinerPrivatePCH.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE( FSceneOutlinerModule, SceneOutliner );

void FSceneOutlinerModule::StartupModule()
{
}


void FSceneOutlinerModule::ShutdownModule()
{
}


TSharedRef< ISceneOutliner > FSceneOutlinerModule::CreateSceneOutliner( const FSceneOutlinerInitializationOptions& InitOptions, const FOnContextMenuOpening& MakeContextMenuWidgetDelegate, const FOnActorPicked& OnActorPickedDelegate ) const
{
	return SNew( SceneOutliner::SSceneOutliner, InitOptions )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.MakeContextMenuWidgetDelegate( MakeContextMenuWidgetDelegate )
		.OnActorPickedDelegate( OnActorPickedDelegate );
}
