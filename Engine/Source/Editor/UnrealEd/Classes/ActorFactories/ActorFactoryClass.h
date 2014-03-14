// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryClass.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryClass : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual bool PreSpawnActor( UObject* Asset, FVector& InOutLocation, FRotator& InOutRotation, bool bRotationWasSupplied ) OVERRIDE;
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) OVERRIDE;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	virtual AActor* GetDefaultActor( const FAssetData& AssetData ) OVERRIDE;
	// End UActorFactory Interface	

protected:
	virtual AActor* SpawnActor( UObject* Asset, ULevel* InLevel, const FVector& Location, const FRotator& Rotation, EObjectFlags ObjectFlags, const FName& Name ) OVERRIDE;
};
