// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#include "SceneOutlinerPrivatePCH.h"
#include "SceneOutlinerTreeItems.h"
#include "ModuleManager.h"

IMPLEMENT_MODULE( FSceneOutlinerModule, SceneOutliner );

void FSceneOutlinerModule::StartupModule()
{
}


void FSceneOutlinerModule::ShutdownModule()
{
}

namespace SceneOutliner
{
	void OnSceneOutlinerItemClicked(TSharedRef<TOutlinerTreeItem> Item, FOnActorPicked OnActorPicked)
	{
		if (Item->Type == TOutlinerTreeItem::Actor)
		{
			AActor* Actor = StaticCastSharedRef<TOutlinerActorTreeItem>(Item)->Actor.Get();
			if (Actor)
			{
				OnActorPicked.ExecuteIfBound(Actor);
			}
		}
	}
}

TSharedRef< ISceneOutliner > FSceneOutlinerModule::CreateSceneOutliner( const FSceneOutlinerInitializationOptions& InitOptions, const FOnActorPicked& OnActorPickedDelegate ) const
{
	auto OnItemPicked = FOnSceneOutlinerItemPicked::CreateStatic( &SceneOutliner::OnSceneOutlinerItemClicked, OnActorPickedDelegate );
	return CreateSceneOutliner(InitOptions, OnItemPicked);
}

TSharedRef< ISceneOutliner > FSceneOutlinerModule::CreateSceneOutliner( const FSceneOutlinerInitializationOptions& InitOptions, const FOnSceneOutlinerItemPicked& OnItemPickedDelegate ) const
{
	return SNew( SceneOutliner::SSceneOutliner, InitOptions )
		.IsEnabled( FSlateApplication::Get().GetNormalExecutionAttribute() )
		.OnItemPickedDelegate( OnItemPickedDelegate );
}
