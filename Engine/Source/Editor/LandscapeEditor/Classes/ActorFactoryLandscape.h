// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ActorFactoryLandscape.generated.h"

UCLASS(MinimalAPI, config=Editor)
class UActorFactoryLandscape : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	//virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	virtual AActor* SpawnActor( UObject* Asset, ULevel* InLevel, const FVector& Location, const FRotator& Rotation, EObjectFlags ObjectFlags, const FName& Name ) OVERRIDE;
	//virtual void PostSpawnActor( UObject* Asset, AActor* NewActor) OVERRIDE;
	//virtual void PostCreateBlueprint( UObject* Asset, AActor* CDO ) OVERRIDE;
	//virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) OVERRIDE;
	// End UActorFactory Interface
};
