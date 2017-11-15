// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "FlexFluidSurfaceActor.generated.h"

UCLASS(notplaceable, transient, MinimalAPI)
class AFlexFluidSurfaceActor : public AInfo
{
	GENERATED_UCLASS_BODY()

public:

	static class UFlexFluidSurfaceComponent* SpawnActor(class UFlexFluidSurface* FlexFluidSurface, uint32 MaxParticles, UWorld* World);
};



