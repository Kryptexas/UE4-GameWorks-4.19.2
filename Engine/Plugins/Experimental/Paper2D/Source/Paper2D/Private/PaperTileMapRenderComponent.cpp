// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperTileMapRenderSceneProxy.h"
#include "PhysicsEngine/BodySetup.h"

#define LOCTEXT_NAMESPACE "Paper2D"

//////////////////////////////////////////////////////////////////////////
// UPaperTileMapRenderComponent

UPaperTileMapRenderComponent::UPaperTileMapRenderComponent(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	BodyInstance.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	MapWidth = 4;
	MapHeight = 4;
	TileWidth = 32;
	TileHeight = 32;

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterial(TEXT("/Paper2D/DefaultSpriteMaterial"));
	Material = DefaultMaterial.Object;
}

FPrimitiveSceneProxy* UPaperTileMapRenderComponent::CreateSceneProxy()
{
	return new FPaperTileMapRenderSceneProxy(this);
}

FBoxSphereBounds UPaperTileMapRenderComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	//@TODO: Tile pivot issue
	//@TODO: Layer thickness issue
	const float HalfThickness = 2.0f;	
	const FVector TopLeft((-0.5f)*TileWidth, -HalfThickness, -(MapHeight - 0.5f) * TileHeight);
	const FVector Dimenisons(MapWidth*TileWidth, 2*HalfThickness, MapHeight * TileHeight);

	const FBox Box(TopLeft, TopLeft + Dimenisons);
	return FBoxSphereBounds(Box.TransformBy(LocalToWorld));
}

void UPaperTileMapRenderComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	// Indicate we need to send new dynamic data.
	MarkRenderDynamicDataDirty();
}

#if WITH_EDITOR
void UPaperTileMapRenderComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileMapRenderComponent, MapWidth)) || (PropertyName == GET_MEMBER_NAME_CHECKED(UPaperTileMapRenderComponent, MapHeight)))
	{
		MapWidth = FMath::Max(MapWidth, 1);
		MapHeight = FMath::Max(MapHeight, 1);

		// Resize all of the existing layers
		for (int32 LayerIndex = 0; LayerIndex < TileLayers.Num(); ++LayerIndex)
		{
			UPaperTileLayer* TileLayer = TileLayers[LayerIndex];
			TileLayer->ResizeMap(MapWidth, MapHeight);
		}
	}

	if (!IsTemplate())
	{
		UpdateBodySetup();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif

FVector UPaperTileMapRenderComponent::ConvertTilePositionToWorldSpace(int32 TileX, int32 TileY, int32 LayerIndex) const
{
	//@TODO: Layer depth, multiply by layer depth here!
	//@TODO: Tile pivot issue
	const FVector LocalPos((TileX - 0.5f) * TileWidth, 0, -(TileY - 0.5f) * TileHeight);

	return ComponentToWorld.TransformPosition(LocalPos);
}

UBodySetup* UPaperTileMapRenderComponent::GetBodySetup()
{
	UpdateBodySetup();

	return ShapeBodySetup;
}

void UPaperTileMapRenderComponent::UpdateBodySetup()
{
	if (ShapeBodySetup == NULL)
	{
		ShapeBodySetup = ConstructObject<UBodySetup>(UBodySetup::StaticClass(), this);
		ShapeBodySetup->CollisionTraceFlag = CTF_UseSimpleAsComplex;
	}

	ShapeBodySetup->AggGeom.BoxElems.Empty();
	for (int32 LayerIndex = 0; LayerIndex < TileLayers.Num(); ++LayerIndex)
	{
		TileLayers[LayerIndex]->AugmentBodySetup(ShapeBodySetup);
	}
}

#undef LOCTEXT_NAMESPACE