// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperCharacter.generated.h"

class UPaperAnimatedRenderComponent;

//=============================================================================
// Paper Characters Exactly like Characters but are sprite based.
//=============================================================================
UCLASS(MinimalAPI)
class APaperCharacter : public ACharacter
{
	GENERATED_UCLASS_BODY()
	
	static FName SpriteComponentName;
	/** The main skeletal mesh associated with this Character (optional sub-object). */
	UPROPERTY(Category = Character, VisibleAnywhere, BlueprintReadOnly)
	TSubobjectPtr<class UPaperAnimatedRenderComponent> Sprite;

protected:

public:	

};
