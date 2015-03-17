// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryVectorFieldVolume.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryVectorFieldVolume : public UActorFactory
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryVectorFieldVolume(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) override;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;
	// End UActorFactory Interface
};



