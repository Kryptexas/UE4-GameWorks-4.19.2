// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperTileMapRenderComponent.generated.h"

UCLASS(DependsOn=UPaperSprite, hideCategories=Object)
class PAPER2D_API UPaperTileMapRenderComponent : public UPrimitiveComponent
{
	GENERATED_UCLASS_BODY()

private:
	// Width of map (in tiles)
	UPROPERTY()
	int32 MapWidth_DEPRECATED;

	UPROPERTY()
	int32 MapHeight_DEPRECATED;

	UPROPERTY()
	int32 TileWidth_DEPRECATED;

	UPROPERTY()
	int32 TileHeight_DEPRECATED;

	// Default tile set to use for new layers
	UPROPERTY()
	UPaperTileSet* DefaultLayerTileSet_DEPRECATED;

	// Test material
	UPROPERTY()
	UMaterialInterface* Material_DEPRECATED;

	// The list of layers
	UPROPERTY()
	TArray<UPaperTileLayer*> TileLayers_DEPRECATED;

public:
	// The tile map used by this component
	UPROPERTY(Category=Setup, EditAnywhere, EditInline, BlueprintReadOnly)
	class UPaperTileMap* TileMap;

protected:
	friend class FPaperTileMapRenderSceneProxy;

public:
	// UObject interface
	virtual void PostInitProperties() OVERRIDE;
#if WITH_EDITORONLY_DATA
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
	virtual void Serialize(FArchive& Ar) OVERRIDE;
	virtual void PostLoad() OVERRIDE;
#endif
	// End of UObject interface

	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) OVERRIDE;
	virtual const UObject* AdditionalStatObject() const OVERRIDE;
	// End of UActorComponent interface

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() OVERRIDE;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const OVERRIDE;
	virtual class UBodySetup* GetBodySetup() OVERRIDE;
	// End of UPrimitiveComponent interface
};
