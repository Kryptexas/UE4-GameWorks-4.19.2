// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "Paper2DEditorPrivatePCH.h"
#include "MeshPaintSpriteAdapter.h"

//////////////////////////////////////////////////////////////////////////
// FMeshPaintSpriteAdapter

bool FMeshPaintSpriteAdapter::Construct(UMeshComponent* InComponent, int32 InPaintingMeshLODIndex, int32 InUVChannelIndex)
{
	SpriteComponent = CastChecked<UPaperSpriteComponent>(InComponent);
	return SpriteComponent->GetSprite() != nullptr;
}

int32 FMeshPaintSpriteAdapter::GetNumTexCoords() const
{
	return 1;
}

void FMeshPaintSpriteAdapter::GetTriangleInfo(int32 TriIndex, struct FTexturePaintTriangleInfo& OutTriInfo) const
{
	//@TODO: MESHPAINT: Write this
}


bool FMeshPaintSpriteAdapter::LineTraceComponent(struct FHitResult& OutHit, const FVector Start, const FVector End, const struct FCollisionQueryParams& Params) const
{
	//@TODO: MESHPAINT: Write this
	return false;
}

//////////////////////////////////////////////////////////////////////////
// FMeshPaintSpriteAdapterFactory

TSharedPtr<IMeshPaintGeometryAdapter> FMeshPaintSpriteAdapterFactory::Construct(class UMeshComponent* InComponent, int32 InPaintingMeshLODIndex, int32 InUVChannelIndex) const
{
	if (UPaperSpriteComponent* SpriteComponent = Cast<UPaperSpriteComponent>(InComponent))
	{
		if (SpriteComponent->GetSprite() != nullptr)
		{
			TSharedRef<FMeshPaintSpriteAdapter> Result = MakeShareable(new FMeshPaintSpriteAdapter());
			if (Result->Construct(InComponent, InPaintingMeshLODIndex, InUVChannelIndex))
			{
				return Result;
			}
		}
	}

	return nullptr;
}
