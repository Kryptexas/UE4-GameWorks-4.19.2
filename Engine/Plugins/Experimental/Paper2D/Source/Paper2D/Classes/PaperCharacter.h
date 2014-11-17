// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperCharacter.generated.h"

// APaperCharacter behaves like ACharacter, but uses a UPaperFlipbookComponent instead of a USkeletalMeshComponent as a visual representation
// Note: The variable named Mesh will not be set up on this actor!
UCLASS(MinimalAPI, EarlyAccessPreview)
class APaperCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()
	
	// Name of the Sprite component
	static FName SpriteComponentName;

	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UPaperFlipbookComponent> Sprite;
};
