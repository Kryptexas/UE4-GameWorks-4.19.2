// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryInteractiveFoliage.generated.h"

UCLASS(MinimalAPI, config=Editor)
class UActorFactoryInteractiveFoliage : public UActorFactoryStaticMesh
{
	GENERATED_UCLASS_BODY()


	// Begin UActorFactory Interface
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) OVERRIDE { return false; };
	// End UActorFactory Interface
};



