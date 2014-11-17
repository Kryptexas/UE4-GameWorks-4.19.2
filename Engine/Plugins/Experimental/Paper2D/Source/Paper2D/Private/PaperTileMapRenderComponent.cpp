// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperTileMapRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"
#include "PaperCustomVersion.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// UPaperTileMapRenderComponent

UPaperTileMapRenderComponent::UPaperTileMapRenderComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	MapWidth_DEPRECATED = 4;
	MapHeight_DEPRECATED = 4;
	TileWidth_DEPRECATED = 32;
	TileHeight_DEPRECATED = 32;

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial"));
	Material_DEPRECATED = DefaultMaterial.Object;
}

FPrimitiveSceneProxy* UPaperTileMapRenderComponent::CreateSceneProxy()
{
	return new FPaperTileMapRenderSceneProxy(this);
}

void UPaperTileMapRenderComponent::PostInitProperties()
{
	TileMap = NewObject<UPaperTileMap>(this);
	TileMap->SetFlags(RF_Transactional);
	Super::PostInitProperties();
}

FBoxSphereBounds UPaperTileMapRenderComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (TileMap != NULL)
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

void UPaperTileMapRenderComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Indicate we need to send new dynamic data.
	MarkRenderDynamicDataDirty();
}

#if WITH_EDITORONLY_DATA
void UPaperTileMapRenderComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UPaperTileMapRenderComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	Ar.UsingCustomVersion(FPaperCustomVersion::GUID);
}

void UPaperTileMapRenderComponent::PostLoad()
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
		TileMap->PixelsPerUnit = 1.0f;
		TileMap->DefaultLayerTileSet = DefaultLayerTileSet_DEPRECATED;
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

UBodySetup* UPaperTileMapRenderComponent::GetBodySetup()
{
	return (TileMap != nullptr) ? TileMap->BodySetup : nullptr;
}

const UObject* UPaperTileMapRenderComponent::AdditionalStatObject() const
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

#undef LOCTEXT_NAMESPACE