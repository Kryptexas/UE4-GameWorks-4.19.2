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

private:
	/** Component to render a text in 3d with a font */
	UPROPERTY(Category = TextRenderActor, VisibleAnywhere, BlueprintReadOnly, meta = (ExposeFunctionCategories = "Rendering|Components|TextRender", AllowPrivateAccess = "true"))
	class UTextRenderComponent* TextRender;

#if WITH_EDITORONLY_DATA
	// Reference to the billboard component
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif

public:
	/** Returns TextRender subobject **/
	FORCEINLINE class UTextRenderComponent* GetTextRender() const { return TextRender; }
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	FORCEINLINE UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
#endif
};



