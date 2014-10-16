// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Base class for all SceneCapture actors
 *
 */

#pragma once
#include "SceneCapture.generated.h"

UCLASS(abstract, hidecategories=(Collision, Attachment, Actor), MinimalAPI)
class ASceneCapture : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	/** To display the 3d camera in the editor. */
	UPROPERTY()
	class UStaticMeshComponent* MeshComp;

public:
	/** Returns MeshComp subobject **/
	FORCEINLINE class UStaticMeshComponent* GetMeshComp() const { return MeshComp; }
};



