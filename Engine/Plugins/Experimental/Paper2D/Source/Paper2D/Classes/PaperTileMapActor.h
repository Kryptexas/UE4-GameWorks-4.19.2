// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTileMapActor.generated.h"

UCLASS(DependsOn=UEngineTypes, MinimalAPI)
class APaperTileMapActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=TileMapActor, VisibleAnywhere, EditInline)
	TSubobjectPtr<class UPaperTileMapRenderComponent> RenderComponent;
};
