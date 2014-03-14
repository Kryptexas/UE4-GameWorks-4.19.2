// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryBlueprint.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryBlueprint : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual bool PreSpawnActor( UObject* Asset, FVector& InOutLocation, FRotator& InOutRotation, bool bRotationWasSupplied ) OVERRIDE;
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) OVERRIDE;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	virtual AActor* GetDefaultActor( const FAssetData& AssetData ) OVERRIDE;
	// End UActorFactory Interface	
};



