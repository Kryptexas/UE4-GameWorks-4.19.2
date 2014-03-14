// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryAnimationAsset.generated.h"

UCLASS(MinimalAPI, config=Editor, hidecategories=Object)
class UActorFactoryAnimationAsset : public UActorFactorySkeletalMesh
{
	GENERATED_UCLASS_BODY()

protected:
	// Begin UActorFactory Interface
	virtual void PostSpawnActor( UObject* Asset, AActor* NewActor ) OVERRIDE;
	virtual void PostCreateBlueprint( UObject* Asset, AActor* CDO ) OVERRIDE;
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE;
	// End UActorFactory Interface

	virtual USkeletalMesh* GetSkeletalMeshFromAsset( UObject* Asset ) const OVERRIDE;
};



