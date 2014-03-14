// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTileMapRenderComponent.generated.h"

UCLASS(DependsOn=UPaperSprite, hideCategories=Object)
class PAPER2D_API UPaperTileMapRenderComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

	// Width of map (in tiles)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 MapWidth;

	// Height of map (in tiles)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 MapHeight;

	// Width of one tile (in pixels)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileWidth;

	// Height of one tile (in pixels)
	UPROPERTY(Category=Setup, EditAnywhere, meta=(UIMin=1, ClampMin=1))
	int32 TileHeight;

	// Default tile set to use for new layers
	UPROPERTY(Category=Setup, EditAnywhere)
	UPaperTileSet* DefaultLayerTileSet;

	// Test material
	UPROPERTY(Category=Setup, EditAnywhere)
	UMaterialInterface* Material;

	// The list of layers
	UPROPERTY()
	TArray<UPaperTileLayer*> TileLayers;

protected:
	friend class FPaperTileMapRenderSceneProxy;

	/** Description of collision */
	UPROPERTY(transient, duplicatetransient)
	class UBodySetup* ShapeBodySetup;

public:
	// UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif
	// End of UObject interface

	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) OVERRIDE;
	// End of UActorComponent interface

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE;
	// End of UPrimitiveComponent interface

	FVector ConvertTilePositionToWorldSpace(int32 TileX, int32 TileY, int32 LayerIndex = 0) const;

protected:
	virtual void UpdateBodySetup();
};
