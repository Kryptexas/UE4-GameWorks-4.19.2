// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * To render text in the 3d world (quads with UV assignment to render text with material support).
 */

#pragma once

#include "TextRenderActor.generated.h"

UCLASS(MinimalAPI, hidecategories=(Collision, Attachment, Actor))
class ATextRenderActor : public AActor
{
	GENERATED_UCLASS_BODY()

	friend class UActorFactoryTextRender;

	/** Component to render a text in 3d with a font */
	UPROPERTY(Category=TextRenderActor, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UTextRenderComponent> TextRender;

#if WITH_EDITORONLY_DATA
	// Reference to the billboard component
	UPROPERTY()
	TSubobjectPtr<UBillboardComponent> SpriteComponent;
#endif
};



