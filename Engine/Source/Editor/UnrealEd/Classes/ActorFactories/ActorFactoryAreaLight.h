// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "ActorFactories/ActorFactoryBasicShape.h"
#include "ActorFactoryAreaLight.generated.h"

class AActor;
struct FAssetData;

UCLASS(MinimalAPI,config=Editor)
class UActorFactoryAreaLight : public UActorFactoryBasicShape
{
	GENERATED_UCLASS_BODY()

	virtual bool CanCreateActorFrom( const FAssetData& AssetData, FText& OutErrorMsg ) override;

	virtual FString GetDefaultLabel(UObject* Asset) override;
};
