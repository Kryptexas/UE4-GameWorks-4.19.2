// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.


#pragma once
#include "ActorFactoryInteractiveFoliage.generated.h"

UCLASS(MinimalAPI, config=Editor)
class UActorFactoryInteractiveFoliage : public UActorFactoryStaticMesh
{
	GENERATED_BODY()
public:
	UNREALED_API UActorFactoryInteractiveFoliage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());


	// Begin UActorFactory Interface
	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override { return false; };
	// End UActorFactory Interface
};



