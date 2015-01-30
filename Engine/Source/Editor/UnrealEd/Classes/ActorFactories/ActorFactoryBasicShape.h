// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryBasicShape.generated.h"

UCLASS(MinimalAPI,config=Editor)
class UActorFactoryBasicShape : public UActorFactoryStaticMesh
{
	GENERATED_UCLASS_BODY()

	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
};
