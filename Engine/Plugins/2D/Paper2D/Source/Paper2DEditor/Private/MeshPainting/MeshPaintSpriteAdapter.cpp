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
	UPaperSprite* Sprite = SpriteComponent->GetSprite();

	const FTransform& ComponentToWorld = SpriteComponent->ComponentToWorld;

	// Can we possibly intersect with the sprite?
	const FBoxSphereBounds& Bounds = SpriteComponent->Bounds;
	if (FMath::PointDistToSegment(Bounds.Origin, Start, End) <= Bounds.SphereRadius)
	{
		const FVector LocalStart = ComponentToWorld.InverseTransformPosition(Start);
		const FVector LocalEnd = ComponentToWorld.InverseTransformPosition(End);

		FPlane LocalSpacePlane(FVector::ZeroVector, PaperAxisX, PaperAxisY);

		FVector Intersection;
		if (FMath::SegmentPlaneIntersection(LocalStart, LocalEnd, LocalSpacePlane, /*out*/ Intersection))
		{
			const float LocalX = FVector::DotProduct(Intersection, PaperAxisX);
			const float LocalY = FVector::DotProduct(Intersection, PaperAxisY);
			const FVector LocalPoint(LocalX, LocalY, 0.0f);

			const TArray<FVector4>& BakedPoints = Sprite->BakedRenderData;
			check((BakedPoints.Num() % 3) == 0);

			for (int32 VertexIndex = 0; VertexIndex < BakedPoints.Num(); VertexIndex += 3)
			{
				const FVector& A = BakedPoints[VertexIndex + 0];
				const FVector& B = BakedPoints[VertexIndex + 1];
				const FVector& C = BakedPoints[VertexIndex + 2];
				const FVector Q = FMath::GetBaryCentric2D(LocalPoint, A, B, C);

				if ((Q.X >= 0.0f) && (Q.Y >= 0.0f) && (Q.Z >= 0.0f) && FMath::IsNearlyEqual(Q.X + Q.Y + Q.Z, 1.0f))
				{
					const FVector WorldIntersection = ComponentToWorld.TransformPosition(Intersection);

					const FVector WorldNormalFront = ComponentToWorld.TransformVectorNoScale(PaperAxisZ);
					const FVector WorldNormal = (LocalSpacePlane.PlaneDot(LocalStart) >= 0.0f) ? WorldNormalFront : -WorldNormalFront;

					OutHit.bBlockingHit = true;
					OutHit.Time = (WorldIntersection - Start).Size() / (End - Start).Size();
					OutHit.Location = WorldIntersection;
					OutHit.Normal = WorldNormal;
					OutHit.ImpactPoint = WorldIntersection;
					OutHit.ImpactNormal = WorldNormal;
					OutHit.TraceStart = Start;
					OutHit.TraceEnd = End;
					OutHit.Actor = SpriteComponent->GetOwner();
					OutHit.Component = SpriteComponent;
					OutHit.FaceIndex = VertexIndex / 3;

					return true;
				}
			}
		}
	}

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
