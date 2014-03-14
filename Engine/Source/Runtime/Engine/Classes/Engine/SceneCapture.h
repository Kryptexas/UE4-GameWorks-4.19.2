// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Base class for all SceneCapture actors
 *
 */

#pragma once
#include "SceneCapture.generated.h"

UCLASS(abstract, hidecategories=(Collision, Attachment, Actor), HeaderGroup=Decal, MinimalAPI)
class ASceneCapture : public AActor
{
	GENERATED_UCLASS_BODY()

	/** To display the 3d camera in the editor. */
	UPROPERTY()
	TSubobjectPtr<class UStaticMeshComponent> MeshComp;
public:
};



