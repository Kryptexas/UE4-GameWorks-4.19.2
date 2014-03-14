// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryMatineeActor.generated.h"

UCLASS(MinimalAPI, config=Editor)
class UActorFactoryMatineeActor : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) OVERRIDE;
	virtual void PostCreateBlueprint( UObject* Asset, AActor* CDO ) OVERRIDE;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	// End UActorFactory Interface
};



