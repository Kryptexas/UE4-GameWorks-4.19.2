// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "NavModifierVolume.generated.h"

/** 
 *	allows applying selected NavAreaDefinition to navmesh, using Volume's shape
 */
UCLASS(hidecategories=(Navigation, "AI|Navigation"))
class ANavModifierVolume : public AVolume, public INavRelevantActorInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Default)
	TSubclassOf<class UNavArea> AreaClass;

	virtual bool GetNavigationRelevantData(struct FNavigationRelevantData& Data) const OVERRIDE;
	virtual bool UpdateNavigationRelevancy() OVERRIDE { SetNavigationRelevancy(true); return true; }
};
