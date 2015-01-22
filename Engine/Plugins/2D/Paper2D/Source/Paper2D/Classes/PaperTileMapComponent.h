// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PaperSprite.h"

#include "PaperTileMapComponent.generated.h"

/**
 * A component that handles rendering and collision for a single instance of a UPaperTileMap asset.
 *
 * This component is created when you drag a tile map asset from the content browser into a Blueprint, or
 * contained inside of the actor created when you drag one into the level.
 *
 * NOTE: This is an experimental class and certain functionality is not properly supported yet (such as collision).  Use at your own risk!
 *
 * @see UPrimitiveComponent, UPaperTileMap
 */

UCLASS(hideCategories=Object, ClassGroup=Paper2D, Experimental, meta=(BlueprintSpawnableComponent))
class PAPER2D_API UPaperTileMapComponent : public UMeshComponent
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY()
	int32 MapWidth_DEPRECATED;

	UPROPERTY()
	int32 MapHeight_DEPRECATED;

	UPROPERTY()
	int32 TileWidth_DEPRECATED;

	UPROPERTY()
	int32 TileHeight_DEPRECATED;

	UPROPERTY()
	UPaperTileSet* DefaultLayerTileSet_DEPRECATED;

	UPROPERTY()
	UMaterialInterface* Material_DEPRECATED;

	UPROPERTY()
	TArray<UPaperTileLayer*> TileLayers_DEPRECATED;

public:
	// The tile map used by this component
	UPROPERTY(Category=Setup, EditAnywhere, BlueprintReadOnly)
	class UPaperTileMap* TileMap;

protected:
	friend class FPaperTileMapRenderSceneProxy;

	void RebuildRenderData(class FPaperTileMapRenderSceneProxy* Proxy);

public:
	// UObject interface
	virtual void PostInitProperties() override;
#if WITH_EDITORONLY_DATA
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
#endif
	// End of UObject interface

	// UActorComponent interface
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual const UObject* AdditionalStatObject() const override;
	// End of UActorComponent interface

	// UPrimitiveComponent interface
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual class UBodySetup* GetBodySetup() override;
	virtual void GetUsedTextures(TArray<UTexture*>& OutTextures, EMaterialQualityLevel::Type QualityLevel) override;
	virtual UMaterialInterface* GetMaterial(int32 MaterialIndex) const override;
	virtual int32 GetNumMaterials() const override;
	// End of UPrimitiveComponent interface

	// Creates a new tile map internally, replacing the TileMap reference (or dropping the previous owned one)
	void CreateNewOwnedTileMap();

	// Does this component own the tile map (is it instanced instead of being an asset reference)?
	UFUNCTION(BlueprintCallable, Category="Sprite")
	bool OwnsTileMap() const;

	/** Change the PaperTileMap used by this instance. */
	UFUNCTION(BlueprintCallable, Category="Sprite")
	virtual bool SetTileMap(class UPaperTileMap* NewTileMap);
};

// Allow the old name to continue to work for one release
DEPRECATED(4.7, "UPaperTileMapRenderComponent has been renamed to UPaperTileMapComponent")
typedef UPaperTileMapComponent UPaperTileMapRenderComponent;
