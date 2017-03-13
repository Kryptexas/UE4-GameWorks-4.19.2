// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AssetManagerSettings.generated.h"

/** Simple structure for redirecting an old asset name/path to a new one */
USTRUCT()
struct FAssetManagerRedirect
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = AssetRedirect)
	FString Old;

	UPROPERTY(EditAnywhere, Category = AssetRedirect)
	FString New;

	friend inline bool operator==(const FAssetManagerRedirect& A, const FAssetManagerRedirect& B)
	{
		return A.Old == B.Old && A.New == B.New;
	}
};

/** Deson't subclass developer settings because the UI shouldn't be enabled until this feature is out of experimental */
UCLASS(config = Game, defaultconfig, notplaceable)
class ENGINE_API UAssetManagerSettings : public UObject
{
	GENERATED_BODY()

public:
	/** Redirect from Type:Name to Type:NameNew */
	UPROPERTY(config, EditAnywhere, Category = "Redirects")
	TArray<FAssetManagerRedirect> PrimaryAssetIdRedirects;

	/** Redirect from Type to TypeNew */
	UPROPERTY(config, EditAnywhere, Category = "Redirects")
	TArray<FAssetManagerRedirect> PrimaryAssetTypeRedirects;

	/** Redirect from /game/assetpath to /game/assetpathnew */
	UPROPERTY(config, EditAnywhere, Category = "Redirects")
	TArray<FAssetManagerRedirect> AssetPathRedirects;

};