// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Info, the root of all information holding classes.
 * Doesn't have any movement / collision related code.
  */

#pragma once
#include "GameFramework/Actor.h"
#include "Info.generated.h"

UCLASS(abstract, hidecategories=(Input, Movement, Collision, Rendering, "Utilities|Transformation"), showcategories=("Input|MouseInput", "Input|TouchInput"), MinimalAPI, NotBlueprintable)
class AInfo : public AActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITORONLY_DATA
private:
	/** Billboard Component displayed in editor */
	UPROPERTY()
	class UBillboardComponent* SpriteComponent;
public:
#endif

	/** Indicates whether this actor should participate in level bounds calculations. */
	virtual bool IsLevelBoundsRelevant() const override { return false; }

public:
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	FORCEINLINE class UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
#endif
};



