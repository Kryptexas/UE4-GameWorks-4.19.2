// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperAnimatedRenderComponent.h"
#include "PaperFlipbookActor.generated.h"

UCLASS(MinimalAPI)
class APaperFlipbookActor : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(Category=Sprite, VisibleAnywhere)
	TSubobjectPtr<class UPaperAnimatedRenderComponent> RenderComponent;

	// AActor interface
#if WITH_EDITOR
	virtual bool GetReferencedContentObjects(TArray<UObject*>& Objects) const override;
#endif
	// End of AActor interface
};

