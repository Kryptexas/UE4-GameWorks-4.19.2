// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

//=============================================================================
//=============================================================================

#pragma once
#include "DocumentationActor.generated.h"

UCLASS(hidecategories = (Sprite, MaterialSprite, Actor, Transform, Tags, Materials, Rendering, Components, Blueprint, bject, Collision, Display, Rendering, Physics, Input, Lighting, Layers))
class ENGINE_API ADocumentationActor : public AActor
{
	GENERATED_UCLASS_BODY()

	/* Open the document link this actor relates to */
	bool OpenDocumentLink() const;
	
	/* Is the document link this actor has valid */
	bool HasValidDocumentLink() const;

#if WITH_EDITORONLY_DATA
	/** Scales forces applied from damage events. */
	UPROPERTY(Category = HelpDocumentation, EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
	FString DocumentLink; 
	
 	UPROPERTY(Category = Sprite, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UMaterialBillboardComponent> Billboard;

#endif

};



