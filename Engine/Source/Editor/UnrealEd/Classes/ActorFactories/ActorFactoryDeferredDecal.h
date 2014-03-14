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
	virtual bool PreSpawnActor( UObject* Asset, FVector& InOutLocation, FRotator& InOutRotation, bool bRotationWasSupplied ) OVERRIDE;
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) OVERRIDE;
	virtual void PostCreateBlueprint( UObject* Asset, AActor* CDO ) OVERRIDE;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	// End UActorFactory Interface

private:
	UMaterialInterface* GetMaterial( UObject* Asset ) const;

};



