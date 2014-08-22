// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 
#include "GameFramework/Volume.h"
#include "NavRelevantInterface.h"
#include "NavModifierVolume.generated.h"

/** 
 *	Allows applying selected AreaClass to navmesh, using Volume's shape
 */
UCLASS(hidecategories=(Navigation, "AI|Navigation"))
class ANavModifierVolume : public AVolume, public INavRelevantInterface
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Default)
	TSubclassOf<class UNavArea> AreaClass;

	virtual void GetNavigationData(struct FNavigationRelevantData& Data) const override;
	virtual FBox GetNavigationBounds() const;
};
