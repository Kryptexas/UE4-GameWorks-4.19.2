// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryEmitter.generated.h"

UCLASS(MinimalAPI, config=Editor, collapsecategories, hidecategories=Object)
class UActorFactoryEmitter : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) OVERRIDE;
	virtual void PostCreateBlueprint( UObject* Asset, AActor* CDO ) OVERRIDE;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) OVERRIDE;
	// End UActorFactory Interface
};



