// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "PreviewMaterial.generated.h"

UCLASS(dependson=UMaterial)
class UNREALED_API UPreviewMaterial : public UMaterial
{
	GENERATED_UCLASS_BODY()


	// Begin UMaterial interface.
	virtual FMaterialResource* AllocateResource() OVERRIDE;
	virtual bool IsAsset()  const OVERRIDE  { return false; }
	// End UMaterial interface.
};

