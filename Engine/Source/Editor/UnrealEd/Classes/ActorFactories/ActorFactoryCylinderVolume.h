// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryCylinderVolume.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryCylinderVolume : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) OVERRIDE;
	// End UActorFactory Interface
};
