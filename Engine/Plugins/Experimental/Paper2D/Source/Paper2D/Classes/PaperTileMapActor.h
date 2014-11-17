// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/EngineTypes.h"

#include "PaperTileMapActor.generated.h"

UCLASS(MinimalAPI)
class APaperTileMapActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=TileMapActor, VisibleAnywhere, EditInline)
	TSubobjectPtr<class UPaperTileMapRenderComponent> RenderComponent;

	// AActor interface
#if WITH_EDITOR
	virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
#endif
	// End of AActor interface
};
