// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "TileMapActorFactory.generated.h"

UCLASS()
class UTileMapActorFactory : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// UActorFactory interface
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	// End of UActorFactory interface
};
