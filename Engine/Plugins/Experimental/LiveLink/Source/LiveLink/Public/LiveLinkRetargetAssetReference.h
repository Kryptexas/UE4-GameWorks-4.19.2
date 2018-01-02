// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "LiveLinkRetargetAssetReference.generated.h"

class ULiveLinkRetargetAsset;

// References a live link retarget asset and handles recreation when
USTRUCT()
struct LIVELINK_API FLiveLinkRetargetAssetReference
{
public:

	GENERATED_BODY()

	FLiveLinkRetargetAssetReference();
	~FLiveLinkRetargetAssetReference() {}

	//UPROPERTY(EditAnywhere, BlueprintReadWrite, NoClear, Category = Retarget, meta = (PinShownByDefault))
	//TSubclassOf<ULiveLinkRetargetAsset> RetargetAsset;

	void Update();

	ULiveLinkRetargetAsset* GetRetargetAsset();

private:

	UPROPERTY(transient)
	ULiveLinkRetargetAsset* CurrentRetargetAsset;
};