// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperSpriteComponent.h"
#include "PaperSpriteActor.generated.h"

UCLASS(MinimalAPI)
class APaperSpriteActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Sprite, VisibleAnywhere)
	TSubobjectPtr<class UPaperSpriteComponent> RenderComponent;

	// AActor interface
#if WITH_EDITOR
	virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
#endif
	// End of AActor interface
};

// Allow the old name to continue to work for one release
DEPRECATED(4.3, "APaperRenderActor has been renamed to APaperSpriteActor")
typedef APaperSpriteActor APaperRenderActor;
