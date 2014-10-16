// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	VectorFieldVolume: Volume encompassing a vector field.
=============================================================================*/

#pragma once

#include "VectorFieldVolume.generated.h"

UCLASS(hidecategories=(Object, Advanced, Collision), MinimalAPI)
class AVectorFieldVolume : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = VectorFieldVolume, meta = (AllowPrivateAccess = "true"))
	class UVectorFieldComponent* VectorFieldComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	UBillboardComponent* SpriteComponent;
#endif

public:
	/** Returns VectorFieldComponent subobject **/
	FORCEINLINE class UVectorFieldComponent* GetVectorFieldComponent() const { return VectorFieldComponent; }
#if WITH_EDITORONLY_DATA
	/** Returns SpriteComponent subobject **/
	FORCEINLINE UBillboardComponent* GetSpriteComponent() const { return SpriteComponent; }
#endif
};



