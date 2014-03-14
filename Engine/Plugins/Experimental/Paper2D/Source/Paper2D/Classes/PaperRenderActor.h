// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperRenderComponent.h"
#include "PaperRenderActor.generated.h"

UCLASS(MinimalAPI)
class APaperRenderActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Sprite, VisibleAnywhere)
	TSubobjectPtr<class UPaperRenderComponent> RenderComponent;
};

