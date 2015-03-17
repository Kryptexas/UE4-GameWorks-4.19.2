// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactorySphereVolume.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactorySphereVolume : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactorySphereVolume(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UActorFactory Interface
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) override;
	// End UActorFactory Interface
};
