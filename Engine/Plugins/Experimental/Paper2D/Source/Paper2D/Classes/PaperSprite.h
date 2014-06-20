// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SpriteEditorOnlyTypes.h"
#include "PaperSprite.generated.h"

UENUM()
namespace ESpriteCollisionMode
{
	enum Type
	{
		None,
		Use2DPhysics,
		Use3DPhysics
	};
}

//@TODO: Should have some nice UI and enforce unique names, etc...
USTRUCT()
struct FPaperSpriteSocket
{
	GENERATED_USTRUCT_BODY()

	// Transform in pivot space (*not* texture space)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Sockets)
	FTransform LocalTransform;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Sockets)
	FName SocketName;
};

/**
 * Sprite Asset
 *
 * Stores the data necessary to render a single 2D sprite (from a region of a texture)
 * Can also contain collision shapes for the sprite.
 */

UCLASS(DependsOn=UEngineTypes, BlueprintType, meta=(DisplayThumbnail = "true"))
class PAPER2D_API UPaperSprite : public UObject, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

protected:

#if WITH_EDITORONLY_DATA
	// Position with SourceTexture
	UPROPERTY(Category=Sprite, EditAnywhere, AssetRegistrySearchable)
	FVector2D SourceUV;

	// Dimensions within SourceTexture
	UPROPERTY(Category=Sprite, EditAnywhere, AssetRegistrySearchable)
	FVector2D SourceDimension;

	// Position within BakedSourceTexture
	UPROPERTY()
	FVector2D BakedSourceUV;
#endif

	UPROPERTY(Category=Sprite, EditAnywhere, AssetRegistrySearchable)
	UTexture2D* SourceTexture;

	UPROPERTY()
	UTexture2D* BakedSourceTexture;

	// The material to use on a sprite instance if not overridden
	UPROPERTY(Category=Sprite, EditAnywhere, BlueprintReadOnly)
	UMaterialInterface* DefaultMaterial;

	// List of sockets on this sprite
	UPROPERTY(Category=Sprite, EditAnywhere)
	TArray<FPaperSpriteSocket> Sockets;

public:
	UPROPERTY(Category=Sprite, EditAnywhere)
	TEnumAsByte<EBlendMode> BlendMode;

	// Collision domain (no collision, 2D, or 3D)
	UPROPERTY(Category=Collision, EditAnywhere)
	TEnumAsByte<ESpriteCollisionMode::Type> SpriteCollisionDomain;

public:
	// Baked physics data.
	UPROPERTY() //@TODO: Is anything in this worth exposing to edit? Category=Physics, EditAnywhere, EditInline
	class UBodySetup* BodySetup;

#if WITH_EDITORONLY_DATA

protected:
	// Pivot mode
	UPROPERTY(Category=Sprite, EditAnywhere)
	TEnumAsByte<ESpritePivotMode::Type> PivotMode;

	// Custom pivot point (relative to the sprite rectangle)
	UPROPERTY(Category=Sprite, EditAnywhere)
	FVector2D CustomPivotPoint;

	// Custom collision geometry polygons (in texture space)
	UPROPERTY(Category=Collision, EditAnywhere)
	FSpritePolygonCollection CollisionGeometry;

	// The extrusion thickness of collision geometry when using a 3D collision domain
	UPROPERTY(Category=Collision, EditAnywhere)
	float CollisionThickness;

	// The scaling factor between pixels and Unreal units (cm) (e.g., 0.64 would make a 64 pixel wide sprite take up 100 cm)
	UPROPERTY(Category=Sprite, EditAnywhere)
	float PixelsPerUnrealUnit;

	// Custom render geometry polygons (in texture space)
	UPROPERTY(Category=Rendering, EditAnywhere)
	FSpritePolygonCollection RenderGeometry;

	// Spritesheet group that this sprite belongs to
	UPROPERTY(Category=Sprite, EditAnywhere, AssetRegistrySearchable)
	class UPaperSpriteAtlas* AtlasGroup;
#endif

public:
	// Baked render data (triangle vertices, stored as XY UV tuples)
	//   XY is the XZ position in world space, relative to the pivot
	//   UV is normalized (0..1)
	//   There should always be a multiple of three elements in this array
	UPROPERTY()
	TArray<FVector4> BakedRenderData;

public:

#if WITH_EDITORONLY_DATA
	// UObject interface
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void Serialize(FArchive& Ar) override;
	virtual void PostLoad() override;
	virtual void GetAssetRegistryTags(TArray<FAssetRegistryTag>& OutTags) const override;
	// End of UObject interface

	FVector2D ConvertTextureSpaceToPivotSpace(FVector2D Input) const;
	FVector2D ConvertPivotSpaceToTextureSpace(FVector2D Input) const;
	FVector ConvertTextureSpaceToPivotSpace(FVector Input) const;
	FVector ConvertPivotSpaceToTextureSpace(FVector Input) const;
	FVector2D ConvertWorldSpaceToTextureSpace(const FVector& WorldPoint) const;

	// World space WRT the sprite editor *only*
	FVector ConvertTextureSpaceToWorldSpace(const FVector2D& SourcePoint) const;
	//FVector ConvertTextureSpaceToWorldSpace(const FVector& SourcePoint) const;
	FTransform GetPivotToWorld() const;

	// Returns the current pivot position in texture space
	FVector2D GetPivotPosition() const;

	void RebuildCollisionData();
	void RebuildRenderData();

	void ExtractSourceRegionFromTexturePoint(const FVector2D& Point);

	void FindTextureBoundingBox(float AlphaThreshold, /*out*/ FVector2D& OutBoxPosition, /*out*/ FVector2D& OutBoxSize);
	static void FindContours(const FIntPoint& ScanPos, const FIntPoint& ScanSize, float AlphaThreshold, UTexture2D* Texture, TArray< TArray<FIntPoint> >& OutPoints);
	static void ExtractRectsFromTexture(UTexture2D* Texture, TArray<FIntRect>& OutRects);
	void BuildGeometryFromContours(FSpritePolygonCollection& GeomOwner);

	void BuildBoundingBoxCollisionData(bool bUseTightBounds);
	void BuildCustomCollisionData();

	void CreatePolygonFromBoundingBox(FSpritePolygonCollection& GeomOwner, bool bUseTightBounds);

	void Triangulate(const FSpritePolygonCollection& Source, TArray<FVector2D>& Target);

	// Reinitializes this sprite (NOTE: Does not register existing components in the world)
	void InitializeSprite(UTexture2D* Texture);
	void InitializeSprite(UTexture2D* Texture, const FVector2D& Offset, const FVector2D& Dimension);

	FVector2D GetSourceUV() const { return SourceUV; }
	FVector2D GetSourceSize() const { return SourceDimension; }
	UTexture2D* GetSourceTexture() const { return SourceTexture; }
	float GetPixelsPerUnrealUnit() const { return PixelsPerUnrealUnit; }
#endif

	// Returns the texture this should be rendered with
	UTexture2D* GetBakedTexture() const;

	UMaterialInterface* GetDefaultMaterial() const { return DefaultMaterial; }

	// Returns the render bounds of this sprite
	FBoxSphereBounds GetRenderBounds() const;

	// Search for a socket (note: do not cache this pointer; it's unsafe if the Socket array is edited)
	FPaperSpriteSocket* FindSocket(FName SocketName);
	bool HasAnySockets() const { return Sockets.Num() > 0; }
	void QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const;

	// IInterface_CollisionDataProvider interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	// End of IInterface_CollisionDataProvider


	//@TODO: HACKERY:
	friend class FSpriteEditorViewportClient;
	friend class FSpriteDetailsCustomization;
	friend class FSpriteSelectedVertex;
	friend class FSpriteSelectedEdge;
	friend class FSpriteSelectedSourceRegion;
	friend struct FPaperAtlasGenerator;
};

