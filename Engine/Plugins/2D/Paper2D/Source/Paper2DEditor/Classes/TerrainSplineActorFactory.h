// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "TerrainSplineActorFactory.generated.h"

UCLASS()
class UTerrainSplineActorFactory : public UActorFactory
{
	GENERATED_BODY()
public:
	UTerrainSplineActorFactory(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UActorFactory interface
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual void PostCreateBlueprint(UObject* Asset, AActor* CDO) override;
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	// End of UActorFactory interface
};
