// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "PaperSpriteActorFactory.generated.h"

UCLASS()
class UPaperSpriteActorFactory : public UActorFactory
{
	GENERATED_UCLASS_BODY()

	// Begin UActorFactory Interface
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) OVERRIDE;
	virtual void PostCreateBlueprint(UObject* Asset, AActor* CDO) OVERRIDE;
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) OVERRIDE;
	// End UActorFactory Interface
};
