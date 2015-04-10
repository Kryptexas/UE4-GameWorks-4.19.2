// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperTileMapRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "PaperCustomVersion.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// UPaperTileMapComponent

UPaperTileMapComponent::UPaperTileMapComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	MapWidth_DEPRECATED = 4;
	MapHeight_DEPRECATED = 4;
	TileWidth_DEPRECATED = 32;
	TileHeight_DEPRECATED = 32;

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial"));
	Material_DEPRECATED = DefaultMaterial.Object;

	CastShadow = false;
	bUseAsOccluder = false;
}

FPrimitiveSceneProxy* UPaperTileMapComponent::CreateSceneProxy()
{
	FPaperTileMapRenderSceneProxy* Proxy = new FPaperTileMapRenderSceneProxy(this);
	RebuildRenderData(Proxy);

	return Proxy;
}

void UPaperTileMapComponent::PostInitProperties()
{
	TileMap = NewObject<UPaperTileMap>(this);
	TileMap->SetFlags(RF_Transactional);
	Super::PostInitProperties();
}

FBoxSphereBounds UPaperTileMapComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (TileMap != nullptr)
	{
		// Graphics bounds.
		FBoxSphereBounds NewBounds = TileMap->GetRenderBounds().TransformBy(LocalToWorld);

		// Add bounds of collision geometry (if present).
		if (UBodySetup* BodySetup = TileMap->BodySetup)
		{
			const FBox AggGeomBox = BodySetup->AggGeom.CalcAABB(LocalToWorld);
			if (AggGeomBox.IsValid)
			{
				NewBounds = Union(NewBounds, FBoxSphereBounds(AggGeomBox));
			}
		}

		// Apply bounds scale
		NewBounds.BoxExtent *= BoundsScale;
		NewBounds.SphereRadius *= BoundsScale;

		return NewBounds;
	}
	else
	{
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector::ZeroVector, 0.f);
	}
}

#if WITH_EDITORONLY_DATA
void UPaperTileMapComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPaperCustomVersion::GUID);
}

void UPaperTileMapComponent::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	if (GetLinkerCustomVersion(FPaperCustomVersion::GUID) < FPaperCustomVersion::MovedTileMapDataToSeparateClass)
	{
		// Create a tile map object and move our old properties over to it
		TileMap = NewObject<UPaperTileMap>(this);
		TileMap->SetFlags(RF_Transactional);
		TileMap->MapWidth = MapWidth_DEPRECATED;
		TileMap->MapHeight = MapHeight_DEPRECATED;
		TileMap->TileWidth = TileWidth_DEPRECATED;
		TileMap->TileHeight = TileHeight_DEPRECATED;
		TileMap->PixelsPerUnrealUnit = 1.0f;
		TileMap->SelectedTileSet = DefaultLayerTileSet_DEPRECATED;
		TileMap->Material = Material_DEPRECATED;
		TileMap->TileLayers = TileLayers_DEPRECATED;

		// Convert the layers
		for (UPaperTileLayer* Layer : TileMap->TileLayers)
		{
			Layer->Rename(nullptr, TileMap, REN_ForceNoResetLoaders | REN_DontCreateRedirectors);
			Layer->ConvertToTileSetPerCell();
		}

		// Remove references in the deprecated variables to prevent issues with deleting referenced assets, etc...
		DefaultLayerTileSet_DEPRECATED = nullptr;
		Material_DEPRECATED = nullptr;
		TileLayers_DEPRECATED.Empty();
	}
#endif
}
#endif

UBodySetup* UPaperTileMapComponent::GetBodySetup()
{
	return (TileMap != nullptr) ? TileMap->BodySetup : nullptr;
}

void UPaperTileMapComponent::GetUsedTextures(TArray<UTexture*>& OutTextures, EMaterialQualityLevel::Type QualityLevel)
{
	// Get the texture referenced by the tile maps
	if (TileMap != nullptr)
	{
		for (UPaperTileLayer* Layer : TileMap->TileLayers)
		{
			for (int32 Y = 0; Y < TileMap->MapHeight; ++Y)
			{
				for (int32 X = 0; X < TileMap->MapWidth; ++X)
				{
					FPaperTileInfo TileInfo = Layer->GetCell(X, Y);
					if (TileInfo.IsValid() && (TileInfo.TileSet != nullptr) && (TileInfo.TileSet->TileSheet != nullptr))
					{
						OutTextures.AddUnique(TileInfo.TileSet->TileSheet);
					}
				}
			}
		}
	}
		
	// Get any textures referenced by our materials
	Super::GetUsedTextures(OutTextures, QualityLevel);
}

UMaterialInterface* UPaperTileMapComponent::GetMaterial(int32 MaterialIndex) const
{
	if (OverrideMaterials.IsValidIndex(MaterialIndex) && (OverrideMaterials[MaterialIndex] != nullptr))
	{
		return OverrideMaterials[MaterialIndex];
	}
	else if (TileMap != nullptr)
	{
		return TileMap->Material;
	}

	return nullptr;
}

int32 UPaperTileMapComponent::GetNumMaterials() const
{
	return FMath::Max<int32>(OverrideMaterials.Num(), 1);
}

const UObject* UPaperTileMapComponent::AdditionalStatObject() const
{
	if (TileMap != nullptr)
	{
		if (TileMap->GetOuter() != this)
		{
			return TileMap;
		}
	}

	return nullptr;
}

void UPaperTileMapComponent::RebuildRenderData(FPaperTileMapRenderSceneProxy* Proxy)
{
	TArray<FSpriteDrawCallRecord> BatchedSprites;
	
	if (TileMap == nullptr)
	{
		return;
	}

	// Handles the rotation and flipping of UV coordinates in a tile
	// 0123 = BL BR TR TL
	const static uint8 PermutationTable[8][4] = 
	{
		{0, 1, 2, 3}, // 000 - normal
		{2, 1, 0, 3}, // 001 - diagonal
		{3, 2, 1, 0}, // 010 - flip Y
		{3, 0, 1, 2}, // 011 - diagonal then flip Y
		{1, 0, 3, 2}, // 100 - flip X
		{1, 2, 3, 0}, // 101 - diagonal then flip X
		{2, 3, 0, 1}, // 110 - flip X and flip Y
		{0, 3, 2, 1}  // 111 - diagonal then flip X and Y
	};

	FVector CornerOffset;
	FVector OffsetYFactor;
	FVector StepPerTileX;
	FVector StepPerTileY;
	TileMap->GetTileToLocalParameters(/*out*/ CornerOffset, /*out*/ StepPerTileX, /*out*/ StepPerTileY, /*out*/ OffsetYFactor);
	
	UTexture2D* LastSourceTexture = nullptr;
	FVector TileSetOffset = FVector::ZeroVector;
	FVector2D InverseTextureSize(1.0f, 1.0f);
	FVector2D SourceDimensionsUV(1.0f, 1.0f);
	FVector2D TileSizeXY(0.0f, 0.0f);

	const float UnrealUnitsPerPixel = TileMap->GetUnrealUnitsPerPixel();

	for (int32 Z = 0; Z < TileMap->TileLayers.Num(); ++Z)
	{
		UPaperTileLayer* Layer = TileMap->TileLayers[Z];

		if (Layer == nullptr)
		{
			continue;
		}

		FLinearColor DrawColor = FLinearColor::White;
#if WITH_EDITORONLY_DATA
		if (Layer->bHiddenInEditor)
		{
			continue;
		}

		DrawColor.A = Layer->LayerOpacity;
#endif

		FSpriteDrawCallRecord* CurrentBatch = nullptr;

		for (int32 Y = 0; Y < TileMap->MapHeight; ++Y)
		{
			// In pixels
			FVector EffectiveTopLeftCorner;

			switch (TileMap->ProjectionMode)
			{
			case ETileMapProjectionMode::Orthogonal:
			default:
				EffectiveTopLeftCorner = CornerOffset;
				break;
			case ETileMapProjectionMode::IsometricDiamond:
				EffectiveTopLeftCorner = CornerOffset - StepPerTileX;
				break;
			case ETileMapProjectionMode::IsometricStaggered:
			case ETileMapProjectionMode::HexagonalStaggered:
				EffectiveTopLeftCorner = CornerOffset + (Y & 1) * OffsetYFactor;
				break;
			}

			for (int32 X = 0; X < TileMap->MapWidth; ++X)
			{
				const FPaperTileInfo TileInfo = Layer->GetCell(X, Y);

				// do stuff
				const float TotalSeparation = (TileMap->SeparationPerLayer * Z) + (TileMap->SeparationPerTileX * X) + (TileMap->SeparationPerTileY * Y);
				FVector TopLeftCornerOfTile = (StepPerTileX * X) + (StepPerTileY * Y) + EffectiveTopLeftCorner;
				TopLeftCornerOfTile += TotalSeparation * PaperAxisZ;

				const int32 TileWidth = TileMap->TileWidth;
				const int32 TileHeight = TileMap->TileHeight;


				{
					UTexture2D* SourceTexture = nullptr;

					FVector2D SourceUV = FVector2D::ZeroVector;

					if (TileInfo.TileSet == nullptr)
					{
						continue;
					}

					if (!TileInfo.TileSet->GetTileUV(TileInfo.GetTileIndex(), /*out*/ SourceUV))
					{
						continue;
					}

					SourceTexture = TileInfo.TileSet->TileSheet;
					if (SourceTexture == nullptr)
					{
						continue;
					}

					if ((SourceTexture != LastSourceTexture) || (CurrentBatch == nullptr))
					{
						CurrentBatch = (new (BatchedSprites) FSpriteDrawCallRecord());
						CurrentBatch->BaseTexture = SourceTexture;
						//CurrentBatch->AdditionalTextures = ?; //@TODO: PAPER2D: Need to add multi-texture support to tile sets / tile maps
						// Probably also need to change the batch check here to TileSet changing to avoid checking each texture in the array
						CurrentBatch->Color = DrawColor;
						CurrentBatch->Destination = TopLeftCornerOfTile.ProjectOnTo(PaperAxisZ);
					}

					if (SourceTexture != LastSourceTexture)
					{
						InverseTextureSize = FVector2D(1.0f / SourceTexture->GetSizeX(), 1.0f / SourceTexture->GetSizeY());

						if (TileInfo.TileSet != nullptr)
						{
							SourceDimensionsUV = FVector2D(TileInfo.TileSet->TileWidth * InverseTextureSize.X, TileInfo.TileSet->TileHeight * InverseTextureSize.Y);
							TileSizeXY = FVector2D(UnrealUnitsPerPixel * TileInfo.TileSet->TileWidth, UnrealUnitsPerPixel * TileInfo.TileSet->TileHeight);
							TileSetOffset = (TileInfo.TileSet->DrawingOffset.X * PaperAxisX) + (TileInfo.TileSet->DrawingOffset.Y * PaperAxisY);
						}
						else
						{
							SourceDimensionsUV = FVector2D(TileWidth * InverseTextureSize.X, TileHeight * InverseTextureSize.Y);
							TileSizeXY = FVector2D(UnrealUnitsPerPixel * TileWidth, UnrealUnitsPerPixel * TileHeight);
							TileSetOffset = FVector::ZeroVector;
						}
						LastSourceTexture = SourceTexture;
					}
					TopLeftCornerOfTile += TileSetOffset;

					SourceUV.X *= InverseTextureSize.X;
					SourceUV.Y *= InverseTextureSize.Y;

					FSpriteDrawCallRecord& NewTile = *CurrentBatch;

					const float WX0 = FVector::DotProduct(TopLeftCornerOfTile, PaperAxisX);
					const float WY0 = FVector::DotProduct(TopLeftCornerOfTile, PaperAxisY);

					const int32 Flags = TileInfo.GetFlagsAsIndex();

					const FVector2D TileSizeWithFlip = TileInfo.HasFlag(EPaperTileFlags::FlipDiagonal) ? FVector2D(TileSizeXY.Y, TileSizeXY.X) : TileSizeXY;
					const float UValues[4] = { SourceUV.X, SourceUV.X + SourceDimensionsUV.X, SourceUV.X + SourceDimensionsUV.X, SourceUV.X };
					const float VValues[4] = { SourceUV.Y + SourceDimensionsUV.Y, SourceUV.Y + SourceDimensionsUV.Y, SourceUV.Y, SourceUV.Y };

					const uint8 UVIndex0 = PermutationTable[Flags][0];
					const uint8 UVIndex1 = PermutationTable[Flags][1];
					const uint8 UVIndex2 = PermutationTable[Flags][2];
					const uint8 UVIndex3 = PermutationTable[Flags][3];

					const FVector4 BottomLeft(WX0, WY0 - TileSizeWithFlip.Y, UValues[UVIndex0], VValues[UVIndex0]);
					const FVector4 BottomRight(WX0 + TileSizeWithFlip.X, WY0 - TileSizeWithFlip.Y, UValues[UVIndex1], VValues[UVIndex1]);
					const FVector4 TopRight(WX0 + TileSizeWithFlip.X, WY0, UValues[UVIndex2], VValues[UVIndex2]);
					const FVector4 TopLeft(WX0, WY0, UValues[UVIndex3], VValues[UVIndex3]);

					new (NewTile.RenderVerts) FVector4(BottomLeft);
					new (NewTile.RenderVerts) FVector4(TopRight);
					new (NewTile.RenderVerts) FVector4(BottomRight);

					new (NewTile.RenderVerts) FVector4(BottomLeft);
					new (NewTile.RenderVerts) FVector4(TopLeft);
					new (NewTile.RenderVerts) FVector4(TopRight);
				}
			}
		}
	}

	Proxy->SetBatchesHack(BatchedSprites);
}

void UPaperTileMapComponent::CreateNewOwnedTileMap()
{
	TGuardValue<TEnumAsByte<EComponentMobility::Type>> MobilitySaver(Mobility, EComponentMobility::Movable);

	UPaperTileMap* NewTileMap = NewObject<UPaperTileMap>(this);
	NewTileMap->SetFlags(RF_Transactional);
	NewTileMap->InitializeNewEmptyTileMap();

	if (TileMap != nullptr)
	{
		NewTileMap->SelectedTileSet = TileMap->SelectedTileSet;
	}

	SetTileMap(NewTileMap);
}

bool UPaperTileMapComponent::OwnsTileMap() const
{
	return (TileMap != nullptr) && (TileMap->GetOuter() == this);
}

bool UPaperTileMapComponent::SetTileMap(class UPaperTileMap* NewTileMap)
{
	if (NewTileMap != TileMap)
	{
		// Don't allow changing the tile map if we are "static".
		AActor* Owner = GetOwner();
		if (!IsRegistered() || (Owner == nullptr) || (Mobility != EComponentMobility::Static))
		{
			TileMap = NewTileMap;

			// Need to send this to render thread at some point
			MarkRenderStateDirty();

			// Update physics representation right away
			RecreatePhysicsState();

			// Since we have new mesh, we need to update bounds
			UpdateBounds();

			return true;
		}
	}

	return false;
}

FPaperTileInfo UPaperTileMapComponent::GetTile(int32 X, int32 Y, int32 Layer) const
{
	FPaperTileInfo Result;
	if (TileMap != nullptr)
	{
		if (TileMap->TileLayers.IsValidIndex(Layer))
		{
			Result = TileMap->TileLayers[Layer]->GetCell(X, Y);
		}
	}

	return Result;
}

void UPaperTileMapComponent::SetTile(int32 X, int32 Y, int32 Layer, FPaperTileInfo NewValue)
{
	if (OwnsTileMap())
	{
		if (TileMap->TileLayers.IsValidIndex(Layer))
		{
			TileMap->TileLayers[Layer]->SetCell(X, Y, NewValue);

			MarkRenderStateDirty();
		}
	}
}

void UPaperTileMapComponent::ResizeMap(int32 NewWidthInTiles, int32 NewHeightInTiles)
{
	if (OwnsTileMap())
	{
		TileMap->ResizeMap(NewWidthInTiles, NewHeightInTiles);
		
		MarkRenderStateDirty();
		RecreatePhysicsState();
		UpdateBounds();
	}
}

UPaperTileLayer* UPaperTileMapComponent::AddNewLayer()
{
	UPaperTileLayer* Result = nullptr;

	if (OwnsTileMap())
	{
		Result = TileMap->AddNewLayer();

		MarkRenderStateDirty();
		RecreatePhysicsState();
		UpdateBounds();
	}

	return Result;
}


#undef LOCTEXT_NAMESPACE