// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "SGameWidget.h"

void SGameWidget::Construct( const FGameWorldContext& InGameContext )
{
	WorldContext = InGameContext;
}

class UWorld* SGameWidget::GetWorld() const
{
	return WorldContext.GetWorld();
}

class ULocalPlayer* SGameWidget::GetLocalPlayer() const
{
	return WorldContext.GetLocalPlayer();
}

APlayerController* SGameWidget::GetPlayerController() const
{
	return WorldContext.GetPlayerController();
}
