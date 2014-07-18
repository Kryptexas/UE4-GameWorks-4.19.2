// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryDeferredDecal.generated.h"

UCLASS(MinimalAPI, config=Editor)
class UActorFactoryDeferredDecal : public UActorFactory
{
public:
	GENERATED_UCLASS_BODY()

protected:
	// Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) override;
	virtual void PostCreateBlueprint( UObject* Asset, AActor* CDO ) override;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;
	// End UActorFactory Interface

private:
	UMaterialInterface* GetMaterial( UObject* Asset ) const;

};



