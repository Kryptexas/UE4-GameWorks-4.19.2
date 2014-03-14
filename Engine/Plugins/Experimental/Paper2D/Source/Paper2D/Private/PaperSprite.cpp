// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Paper2DPrivatePCH.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"

#if WITH_EDITORONLY_DATA
#include "GeomTools.h"

//////////////////////////////////////////////////////////////////////////
// FTextureReader

struct FTextureReader
{
	FTextureReader(UTexture2D* SourceTexture)
		: Width(0)
		, Height(0)
		, BytesPerPixel(0)
		, TransparentBlack(0,0,0,0)
		, AlphaThreshold(0)
		, X0(0)
		, Y0(0)
		, X1(0)
		, Y1(0)
	{
		// use the source art if it exists
		FTextureSource* Source = NULL;
		if ((SourceTexture != NULL) && SourceTexture->Source.IsValid())
		{
			switch (SourceTexture->Source.GetFormat())
			{
			case TSF_G8:
			case TSF_BGRA8:
				Source = &(SourceTexture->Source);
				break;
			default:
				break;
			};
		}

		if (Source != NULL)
		{
			Source->GetMipData(RawData, 0);
			Width = Source->GetSizeX();
			Height = Source->GetSizeY();
			BytesPerPixel = Source->GetBytesPerPixel();
			PixelFormat = Source->GetFormat();

			X0 = 0;
			Y0 = 0;
			X1 = Width-1;
			Y1 = Height-1;
		}
	}

	void SetBounds(int32 NewX0, int32 NewY0, int32 NewX1, int32 NewY1)
	{
		X0 = NewX0;
		Y0 = NewY0;
		X1 = NewX1;
		Y1 = NewY1;
	}

	bool IsValid() const
	{
		return (Source != NULL);
	}

	FColor GetColor(int32 X, int32 Y) const
	{
		if ((X < 0) || (Y < 0) || (X >= Width) || (Y >= Height))
		{
			return TransparentBlack;
		}

		int32 PixelByteOffset = (X + Y * Width) * BytesPerPixel;
		const uint8* PixelPtr = RawData.GetTypedData() + PixelByteOffset;

		if (PixelFormat == TSF_BGRA8)
		{
			return *((FColor*)PixelPtr);
		}
		else
		{
			checkSlow(PixelFormat == TSF_G8);
			const uint8 Intensity = *PixelPtr;
			return FColor(Intensity, Intensity, Intensity, Intensity);
		}
	}

	bool PixelPassesThreshold(int32 X, int32 Y) const
	{
		return GetColor(X, Y).A > AlphaThreshold;
	}

	bool PixelPassesThreshold(int32 X, int32 Y, bool bOutOfBoundsValue) const
	{
		if ((X < X0) || (Y < Y0) || (X > X1) || (Y >= Y1))
		{
			return bOutOfBoundsValue;
		}
		else
		{
			return GetColor(X, Y).A > AlphaThreshold;
		}
	}
public:
	FTextureSource* Source;
	TArray<uint8> RawData;
	int32 Width;
	int32 Height;
	int32 BytesPerPixel;
	ETextureSourceFormat PixelFormat;
	FColor TransparentBlack;
	int32 AlphaThreshold;

	// Bounds for the second form of PixelPassesThreshold
	int32 X0;
	int32 Y0;
	int32 X1;
	int32 Y1;
};

bool IsClearRow(FTextureReader& Reader, int32 X0, int32 X1, int32 Y)
{
	for (int32 X = X0; X <= X1; ++X)
	{
		if (Reader.PixelPassesThreshold(X, Y))
		{
			return false;
		}
	}
	return true;
}

bool IsClearColumn(FTextureReader& Reader, int32 X, int32 Y0, int32 Y1)
{
	for (int32 Y = Y0; Y <= Y1; ++Y)
	{
		if (Reader.PixelPassesThreshold(X, Y))
		{
			return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
// maf

void RemoveCollinearPoints(TArray<FIntPoint>& PointList)
{
	if (PointList.Num() < 3)
	{
		return;
	}

	for (int32 VertexIndex = 1; VertexIndex < PointList.Num(); )
	{
		const FVector2D A(PointList[VertexIndex-1]);
		const FVector2D B(PointList[VertexIndex]);
		const FVector2D C(PointList[(VertexIndex+1) % PointList.Num()]);

		// Determine if the area of the triangle ABC is zero (if so, they're collinear)
		const float AreaABC = (A.X * (B.Y - C.Y)) + (B.X * (C.Y - A.Y)) + (C.X * (A.Y - B.Y));

		if (FMath::Abs(AreaABC) < KINDA_SMALL_NUMBER)
		{
			// Remove B
			PointList.RemoveAt(VertexIndex);
		}
		else
		{
			// Continue onwards
			++VertexIndex;
		}
	}
}

float DotPoints(const FIntPoint& V1, const FIntPoint& V2)
{
	return (V1.X * V2.X) + (V1.Y * V2.Y);
}

struct FDouglasPeuckerSimplifier
{
	FDouglasPeuckerSimplifier(const TArray<FIntPoint>& InSourcePoints, float Epsilon)
		: SourcePoints(InSourcePoints)
		, EpsilonSquared(Epsilon * Epsilon)
		, NumRemoved(0)
	{
		OmitPoints.AddZeroed(SourcePoints.Num());
	}

	TArray<FIntPoint> SourcePoints;
	TArray<bool> OmitPoints;
	const float EpsilonSquared;
	int32 NumRemoved;

	// Removes all points between Index1 and Index2, not including them
	void RemovePoints(int32 Index1, int32 Index2)
	{
		for (int32 Index = Index1 + 1; Index < Index2; ++Index)
		{
			OmitPoints[Index] = true;
			++NumRemoved;
		}
	}

	void SimplifyPointsInner(int Index1, int Index2)
	{
		if (Index2 - Index1 < 2)
		{
			return;
		}

		// Find furthest point from the V1..V2 line
		const FVector V1(SourcePoints[Index1].X, 0.0f, SourcePoints[Index1].Y);
		const FVector V2(SourcePoints[Index2].X, 0.0f, SourcePoints[Index2].Y);
		const FVector V1V2 = V2 - V1;
		const float LineScale = 1.0f / V1V2.SizeSquared();

		float FarthestDistanceSquared = -1.0f;
		int32 FarthestIndex = INDEX_NONE;

		for (int32 Index = Index1; Index < Index2; ++Index)
		{
			const FVector VTest(SourcePoints[Index].X, 0.0f, SourcePoints[Index].Y);
			const FVector V1VTest = VTest - V1;

			const float t = FMath::Clamp(FVector::DotProduct(V1VTest, V1V2) * LineScale, 0.0f, 1.0f);
			const FVector ClosestPointOnV1V2 = V1 + t * V1V2;

			const float DistanceToLineSquared = FVector::DistSquared(ClosestPointOnV1V2, VTest);
			if (DistanceToLineSquared > FarthestDistanceSquared)
			{
				FarthestDistanceSquared = DistanceToLineSquared;
				FarthestIndex = Index;
			}
		}

		if (FarthestDistanceSquared > EpsilonSquared)
		{
			// Too far, subdivide further
			SimplifyPointsInner(Index1, FarthestIndex);
			SimplifyPointsInner(FarthestIndex, Index2);
		}
		else
		{
			// The farthest point wasn't too far, so omit all the points in between
			RemovePoints(Index1, Index2);
		}
	}

	void Execute(TArray<FIntPoint>& Result)
	{
		SimplifyPointsInner(0, SourcePoints.Num() - 1);

		Result.Empty(SourcePoints.Num() - NumRemoved);
		for (int32 Index = 0; Index < SourcePoints.Num(); ++Index)
		{
			if (!OmitPoints[Index])
			{
				Result.Add(SourcePoints[Index]);
			}
		}
	}
};

void SimplifyPoints(TArray<FIntPoint>& Points, float Epsilon)
{
	FDouglasPeuckerSimplifier Simplifier(Points, Epsilon);
	Simplifier.Execute(Points);
}

	
//////////////////////////////////////////////////////////////////////////
// FBoundaryImage

struct FBoundaryImage
{
	TArray<int8> Pixels;

	// Value to return out of bounds
	int8 OutOfBoundsValue;

	int32 X0;
	int32 Y0;
	int32 Width;
	int32 Height;

	FBoundaryImage(const FIntPoint& Pos, const FIntPoint& Size)
	{
		OutOfBoundsValue = 0;

		X0 = Pos.X - 1;
		Y0 = Pos.Y - 1;
		Width = Size.X + 2;
		Height = Size.Y + 2;

		Pixels.AddZeroed(Width * Height);
	}

	int32 GetIndex(int32 X, int32 Y) const
	{
		const int32 LocalX = X - X0;
		const int32 LocalY = Y - Y0;

		if ((LocalX >= 0) && (LocalX < Width) && (LocalY >= 0) && (LocalY < Height))
		{
			return LocalX + (LocalY * Width);
		}
		else
		{
			return INDEX_NONE;
		}
	}

	int8 GetPixel(int32 X, int32 Y) const
	{
		const int32 Index = GetIndex(X, Y);
		if (Index != INDEX_NONE)
		{
			return Pixels[Index];
		}
		else
		{
			return OutOfBoundsValue;
		}
	}

	void SetPixel(int32 X, int32 Y, int8 Value)
	{
		const int32 Index = GetIndex(X, Y);
		if (Index != INDEX_NONE)
		{
			Pixels[Index] = Value;
		}
	}
};

#endif

//////////////////////////////////////////////////////////////////////////
// FSpriteDrawCallRecord

void FSpriteDrawCallRecord::BuildFromSprite(class UPaperSprite* Sprite)
{
	if (Sprite != NULL)
	{
		//SourceDimension = Sprite->GetSourceSize();
		Destination = FVector::ZeroVector;
		//DestinationDimension = Sprite->GetSourceSize();
		Texture = Sprite->GetPrimaryAtlasTexture();
		Color = FLinearColor::White;
		BlendMode = Sprite->BlendMode;

		RenderVerts = Sprite->BakedRenderData;
	}
}

//////////////////////////////////////////////////////////////////////////
// UPaperSprite

UPaperSprite::UPaperSprite(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// Default to using physics
	SpriteCollisionDomain = ESpriteCollisionMode::Use3DPhysics;

#if WITH_EDITORONLY_DATA
	PivotMode = ESpritePivotMode::Center_Center;

	CollisionGeometry.GeometryType = ESpritePolygonMode::TightBoundingBox;
	CollisionThickness = 10.0f;
#endif

	static ConstructorHelpers::FObjectFinder<UMaterial> DefaultMaterialRef(TEXT("/Paper2D/DefaultSpriteMaterial.DefaultSpriteMaterial"));
	DefaultMaterial = DefaultMaterialRef.Object;
}

#if WITH_EDITORONLY_DATA
void UPaperSprite::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	//@TODO: Determine when these are really needed!
	TComponentReregisterContext<UPaperRenderComponent> ReregisterStaticComponents;
	TComponentReregisterContext<UPaperAnimatedRenderComponent> ReregisterAnimatedComponents;

	// Update the pivot
	if (PivotMode != ESpritePivotMode::Custom)
	{
		CustomPivotPoint = GetPivotPosition();
	}

	// Look for changed properties

	const FName PropertyName = (PropertyChangedEvent.Property != NULL) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	bool bRenderDataModified = false;
	bool bCollisionDataModified = false;
	bool bBothModified = false;

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, SpriteCollisionDomain)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, BodySetup3D)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, CollisionGeometry)) )
	{
		bCollisionDataModified = true;
	}

	// Properties inside one of the geom structures (we don't know which one)
// 	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, GeometryType)) ||
// 		(PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, AlphaThreshold)) ||
// 		)
// 		BoxSize
// 		BoxPosition
// 		Vertices
// 		VertexCount
// 		Polygons
// 	{
		bBothModified = true;
//	}

	if ((PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, SourceUV)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, SourceDimension)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, SourceTexture)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, CustomPivotPoint)) ||
		(PropertyName == GET_MEMBER_NAME_CHECKED(UPaperSprite, PivotMode)) )
	{
		bBothModified = true;
	}

	if (bCollisionDataModified || bBothModified)
	{
		RebuildCollisionData();
	}

	if (bRenderDataModified || bBothModified)
	{
		RebuildRenderData();
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UPaperSprite::RebuildCollisionData()
{
	// Ensure we have the data structure for the desired collision method
	switch (SpriteCollisionDomain)
	{
	case ESpriteCollisionMode::Use3DPhysics:
		if (BodySetup3D == NULL)
		{
			BodySetup3D = ConstructObject<UBodySetup>(UBodySetup::StaticClass(), this);
		}
		break;
	case ESpriteCollisionMode::Use2DPhysics:
		BodySetup3D = NULL;
		break;
	case ESpriteCollisionMode::None:
		BodySetup3D = NULL;
		CollisionGeometry.Reset();
		break;
	}

	if (SpriteCollisionDomain != ESpriteCollisionMode::None)
	{
		switch (CollisionGeometry.GeometryType)
		{
		case ESpritePolygonMode::SourceBoundingBox:
			BuildBoundingBoxCollisionData(/*bUseTightBounds=*/ false);
			break;

		case ESpritePolygonMode::TightBoundingBox:
			BuildBoundingBoxCollisionData(/*bUseTightBounds=*/ true);
			break;

		case ESpritePolygonMode::ShrinkWrapped:
			BuildGeometryFromContours(CollisionGeometry);
			BuildCustomCollisionData();
			break;

		case ESpritePolygonMode::FullyCustom:
			BuildCustomCollisionData();
			break;
		default:
			check(false); // unknown mode
		};
	}
}

void UPaperSprite::BuildCustomCollisionData()
{
	// Rebuild the runtime geometry
	TArray<FVector2D> CollisionData;
	Triangulate(CollisionGeometry, CollisionData);

	// Adjust the collision data to be relative to the pivot
	for (int32 PointIndex = 0; PointIndex < CollisionData.Num(); ++PointIndex)
	{
		FVector2D& Point = CollisionData[PointIndex];
		Point = ConvertTextureSpaceToPivotSpace(Point);
	}

	// Bake it to the runtime structure
	switch (SpriteCollisionDomain)
	{
	case ESpriteCollisionMode::Use3DPhysics:
		{
			check(BodySetup3D);
			BodySetup3D->AggGeom.EmptyElements();

			//@TODO: Observe if the custom data is really a hand-edited bounding box, and generate box geom instead of convex geom!

			const FVector HalfThicknessVector = PaperAxisZ * 0.5f * CollisionThickness;
			//@TODO: Use this guy instead: DecomposeMeshToHulls
			//@TODO: Merge triangles that are convex together!
			int32 RunningIndex = 0;
			for (int32 TriIndex = 0; TriIndex < CollisionData.Num() / 3; ++TriIndex)
			{
				FKConvexElem& ConvexTri = *new (BodySetup3D->AggGeom.ConvexElems) FKConvexElem();
				ConvexTri.VertexData.Empty(6);
				for (int32 Index = 0; Index < 3; ++Index)
				{
					const FVector2D& Pos2D = CollisionData[RunningIndex++];
					
					const FVector Pos3D = (PaperAxisX * Pos2D.X) + (PaperAxisY * Pos2D.X);

					new (ConvexTri.VertexData) FVector(Pos3D - HalfThicknessVector);
					new (ConvexTri.VertexData) FVector(Pos3D + HalfThicknessVector);
				}
				ConvexTri.UpdateElemBox();
			}

			BodySetup3D->InvalidatePhysicsData();
			BodySetup3D->CreatePhysicsMeshes();
		}
		break;
	case ESpriteCollisionMode::Use2DPhysics:
		break;
	default:
		check(false);
		break;
	}
}

void UPaperSprite::BuildBoundingBoxCollisionData(bool bUseTightBounds)
{
	// Update the polygon data
	CreatePolygonFromBoundingBox(CollisionGeometry, bUseTightBounds);

	// Bake it to the runtime structure
	switch (SpriteCollisionDomain)
	{
	case ESpriteCollisionMode::Use3DPhysics:
		{
			// Store the bounding box as an actual box for 3D physics
			check(BodySetup3D);
			BodySetup3D->AggGeom.EmptyElements();

			// Determine the box center in pivot space
			const FVector2D& BoxSize2D = CollisionGeometry.Polygons[0].BoxSize;
			const FVector2D& BoxPos = CollisionGeometry.Polygons[0].BoxPosition;
			const FVector2D CenterInTextureSpace = BoxPos + (BoxSize2D * 0.5f);
			const FVector2D CenterInPivotSpace = ConvertTextureSpaceToPivotSpace(CenterInTextureSpace);

			// Create a new box primitive
			const FVector BoxSize3D = (PaperAxisX * BoxSize2D.X) + (PaperAxisY * BoxSize2D.Y) + (PaperAxisZ * CollisionThickness);

			FKBoxElem& Box = *new (BodySetup3D->AggGeom.BoxElems) FKBoxElem(BoxSize3D.X, BoxSize3D.Y, BoxSize3D.Z);
			Box.Center = (PaperAxisX * CenterInPivotSpace.X) + (PaperAxisY * CenterInPivotSpace.Y);

			BodySetup3D->InvalidatePhysicsData();
			BodySetup3D->CreatePhysicsMeshes();
		}
		break;
	case ESpriteCollisionMode::Use2DPhysics:
		//@TODO:!!!
		break;
	default:
		check(false);
		break;
	}
}

void UPaperSprite::RebuildRenderData()
{
	switch (RenderGeometry.GeometryType)
	{
	case ESpritePolygonMode::SourceBoundingBox:
		CreatePolygonFromBoundingBox(RenderGeometry, /*bUseTightBounds=*/ false);
		break;

	case ESpritePolygonMode::TightBoundingBox:
		CreatePolygonFromBoundingBox(RenderGeometry, /*bUseTightBounds=*/ true);
		break;

	case ESpritePolygonMode::ShrinkWrapped:
		BuildGeometryFromContours(RenderGeometry);
		break;

	case ESpritePolygonMode::FullyCustom:
		// Do nothing special, the data is already in the polygon
		break;
	default:
		check(false); // unknown mode
	};

	// Triangulate the render geometry
	TArray<FVector2D> TriangluatedPoints;
	Triangulate(RenderGeometry, /*out*/ TriangluatedPoints);

	// Adjust for the pivot and store in the baked geometry buffer
	const float InverseWidth = (SourceTexture != NULL) ? (1.0f / SourceTexture->GetSizeX()) : 1.0f;
	const float InverseHeight = (SourceTexture != NULL) ? (1.0f / SourceTexture->GetSizeY()) : 1.0f;

	BakedRenderData.Empty(TriangluatedPoints.Num());
	for (int32 PointIndex = 0; PointIndex < TriangluatedPoints.Num(); ++PointIndex)
	{
		const FVector2D& UV = TriangluatedPoints[PointIndex];
		const FVector2D PivotSpacePos = ConvertTextureSpaceToPivotSpace(UV);

		new (BakedRenderData) FVector4(PivotSpacePos.X, PivotSpacePos.Y, UV.X * InverseWidth, UV.Y * InverseHeight);
	}
	check((BakedRenderData.Num() % 3) == 0);
}

void UPaperSprite::FindTextureBoundingBox(float AlphaThreshold, /*out*/ FVector2D& OutBoxPosition, /*out*/ FVector2D& OutBoxSize)
{
	// Create an initial guess at the bounds based on the source rectangle
	int32 LeftBound = (int32)SourceUV.X;
	int32 RightBound = (int32)(SourceUV.X + SourceDimension.X - 1);

	int32 TopBound = (int32)SourceUV.Y;
	int32 BottomBound = (int32)(SourceUV.Y + SourceDimension.Y - 1);


	FTextureReader Reader(SourceTexture);
	if (Reader.IsValid())
	{
		Reader.AlphaThreshold = FMath::Clamp<int32>(AlphaThreshold * 255, 0, 255);

		// Make sure the initial bounds starts in the texture
		LeftBound = FMath::Clamp(LeftBound, 0, Reader.Width-1);
		RightBound = FMath::Clamp(RightBound, 0, Reader.Width-1);
		TopBound = FMath::Clamp(TopBound, 0, Reader.Height-1);
		BottomBound = FMath::Clamp(BottomBound, 0, Reader.Height-1);

		// Pull it in from the top
		while ((TopBound < BottomBound) && IsClearRow(Reader, LeftBound, RightBound, TopBound))
		{
			++TopBound;
		}

		// Pull it in from the bottom
		while ((BottomBound > TopBound) && IsClearRow(Reader, LeftBound, RightBound, BottomBound))
		{
			--BottomBound;
		}

		// Pull it in from the left
		while ((LeftBound < RightBound) && IsClearColumn(Reader, LeftBound, TopBound, BottomBound))
		{
			++LeftBound;
		}

		// Pull it in from the right
		while ((RightBound > LeftBound) && IsClearColumn(Reader, RightBound, TopBound, BottomBound))
		{
			--RightBound;
		}
	}

	OutBoxSize.X = RightBound - LeftBound + 1;
	OutBoxSize.Y = BottomBound - TopBound + 1;
	OutBoxPosition.X = LeftBound;
	OutBoxPosition.Y = TopBound;
}

void UPaperSprite::BuildGeometryFromContours(FSpritePolygonCollection& GeomOwner)
{
	// First trim the image to the tight fitting bounding box (the other pixels can't matter)
	FVector2D InitialBoxSizeFloat;
	FVector2D InitialBoxPosFloat;
	FindTextureBoundingBox(GeomOwner.AlphaThreshold, /*out*/ InitialBoxPosFloat, /*out*/ InitialBoxSizeFloat);

	const FIntPoint InitialPos((int32)InitialBoxPosFloat.X, (int32)InitialBoxPosFloat.Y);
	const FIntPoint InitialSize((int32)InitialBoxSizeFloat.X, (int32)InitialBoxSizeFloat.Y);

	// Find the contours
	TArray< TArray<FIntPoint> > Contours;
	FindContours(InitialPos, InitialSize, GeomOwner.AlphaThreshold, SourceTexture, /*out*/ Contours);

	//@TODO: Remove fully enclosed contours - may not be needed if we're tracing only outsides

	// Convert the contours into geometry
	GeomOwner.Polygons.Empty();
	for (int32 ContourIndex = 0; ContourIndex < Contours.Num(); ++ContourIndex)
	{
		//@TODO: Optimize contours
		TArray<FIntPoint>& Contour = Contours[ContourIndex];

		SimplifyPoints(Contour, GeomOwner.SimplifyEpsilon);

		if (Contour.Num() > 0)
		{
			FSpritePolygon& Polygon = *new (GeomOwner.Polygons) FSpritePolygon();
			Polygon.Vertices.Empty(Contour.Num());

			for (int32 PointIndex = 0; PointIndex < Contour.Num(); ++PointIndex)
			{
				new (Polygon.Vertices) FVector2D(Contour[PointIndex]);
			}
		}
	}
}


// findme

void UPaperSprite::FindContours(const FIntPoint& ScanPos, const FIntPoint& ScanSize, float AlphaThreshold, UTexture2D* Texture, TArray< TArray<FIntPoint> >& OutPoints)
{
	OutPoints.Empty();

	if ((ScanSize.X <= 0) || (ScanSize.Y <= 0))
	{
		return;
	}

	const int32 LeftBound = ScanPos.X;
	const int32 RightBound = ScanPos.X + ScanSize.X - 1;
	const int32 TopBound = ScanPos.Y;
	const int32 BottomBound = ScanPos.Y + ScanSize.Y - 1;

	// Neighborhood array (clockwise starting at -X,-Y; assuming prev is at -X)
	const int32 NeighborX[] = {-1, 0,+1,+1,+1, 0,-1,-1};
	const int32 NeighborY[] = {-1,-1,-1, 0,+1,+1,+1, 0};
	//                       0  1  2  3  4  5  6  7
	// 0 1 2
	// 7   3
	// 6 5 4
	const int32 StateMutation[] = {
		5, //from0
		6, //from1
		7, //from2
		0, //from3
		1, //from4
		2, //from5
		3, //from6
		4, //from7
	};

	FTextureReader Reader(Texture);
	if (Reader.IsValid())
	{
		checkSlow((LeftBound >= 0) && (TopBound >= 0) && (RightBound < Reader.Width) && (BottomBound < Reader.Height));
		Reader.SetBounds(LeftBound, TopBound, RightBound, BottomBound);

		Reader.AlphaThreshold = FMath::Clamp<int32>(AlphaThreshold * 255, 0, 255);

		// Create the 'output' boundary image
		FBoundaryImage BoundaryImage(ScanPos, ScanSize);

		bool bInsideBoundary = false;

		for (int32 Y = TopBound - 1; Y < BottomBound + 2; ++Y)
		{
			for (int32 X = LeftBound - 1; X < RightBound + 2; ++X)
			{
				const bool bAlreadyTaggedAsBoundary = BoundaryImage.GetPixel(X, Y) > 0;
				const bool bIsFilledPixel = Reader.PixelPassesThreshold(X, Y, false);

				if (bInsideBoundary)
				{
					if (!bIsFilledPixel)
					{
						// We're leaving the known boundary
						bInsideBoundary = false;
					}
				}
				else
				{
					if (bAlreadyTaggedAsBoundary)
					{
						// We're re-entering a known boundary
						bInsideBoundary = true;
					}
					else if (bIsFilledPixel)
					{
						// Create the output chain we'll build from the boundary image
						TArray<FIntPoint>& Contour = *new (OutPoints) TArray<FIntPoint>();

						// Moving into an undiscovered boundary
						BoundaryImage.SetPixel(X, Y, 1);
						new (Contour) FIntPoint(X, Y);

						// Current pixel
						int32 NeighborPhase = 0;
						int32 PX = X;
						int32 PY = Y;

						int32 EnteredStartingSquareCount = 0;
						int32 SinglePixelCounter = 0;

						for (;;)
						{
							// Test pixel (clockwise from the current pixel)
							const int32 CX = PX + NeighborX[NeighborPhase];
							const int32 CY = PY + NeighborY[NeighborPhase];
							const bool bTestPixelPasses = Reader.PixelPassesThreshold(CX, CY, false);

							UE_LOG(LogPaper2D, Log, TEXT("Outer P(%d,%d), C(%d,%d) Ph%d %s"),
								PX, PY, CX, CY, NeighborPhase, bTestPixelPasses ? TEXT("[BORDER]") : TEXT("[]"));

							if (bTestPixelPasses)
							{
								// Move to the next pixel

								// Check to see if we closed the loop
 								if ((CX == X) && (CY == Y))
 								{
// 									// If we went thru the boundary pixel more than two times, or
// 									// entered it from the same way we started, then we're done
// 									++EnteredStartingSquareCount;
// 									if ((EnteredStartingSquareCount > 2) || (NeighborPhase == 0))
// 									{
									//@TODO: Not good enough, will early out too soon some of the time!
 										bInsideBoundary = true;
 										break;
 									}
// 								}

								BoundaryImage.SetPixel(CX, CY, NeighborPhase+1);
								new (Contour) FIntPoint(CX, CY);

								PX = CX;
								PY = CY;
								NeighborPhase = StateMutation[NeighborPhase];

								SinglePixelCounter = 0;
								//NeighborPhase = (NeighborPhase + 1) % 8;
							}
							else
							{
								NeighborPhase = (NeighborPhase + 1) % 8;

								++SinglePixelCounter;
								if (SinglePixelCounter > 8)
								{
									// Went all the way around the neighborhood; it's an island of a single pixel
									break;
								}
							}
						}
					
						// Remove collinear points from the result
						RemoveCollinearPoints(/*inout*/ Contour);
					}
				}
			}
		}
	}
}

void UPaperSprite::CreatePolygonFromBoundingBox(FSpritePolygonCollection& GeomOwner, bool bUseTightBounds)
{
	FVector2D BoxSize;
	FVector2D BoxPosition;

	if (bUseTightBounds)
	{
		FindTextureBoundingBox(GeomOwner.AlphaThreshold, /*out*/ BoxPosition, /*out*/ BoxSize);
	}
	else
	{
		BoxSize = SourceDimension;
		BoxPosition = SourceUV;
	}

	// Put the bounding box into the geometry array
	GeomOwner.Polygons.Empty();
	FSpritePolygon& Poly = *new (GeomOwner.Polygons) FSpritePolygon();
	new (Poly.Vertices) FVector2D(BoxPosition.X, BoxPosition.Y);
	new (Poly.Vertices) FVector2D(BoxPosition.X + BoxSize.X, BoxPosition.Y);
	new (Poly.Vertices) FVector2D(BoxPosition.X + BoxSize.X, BoxPosition.Y + BoxSize.Y);
	new (Poly.Vertices) FVector2D(BoxPosition.X, BoxPosition.Y + BoxSize.Y);
	Poly.BoxSize = BoxSize;
	Poly.BoxPosition = BoxPosition;
}

void UPaperSprite::Triangulate(const FSpritePolygonCollection& Source, TArray<FVector2D>& Target)
{
	Target.Empty();

	for (int32 PolygonIndex = 0; PolygonIndex < Source.Polygons.Num(); ++PolygonIndex)
	{
		const FSpritePolygon& SourcePoly = Source.Polygons[PolygonIndex];
		if (SourcePoly.Vertices.Num() >= 3)
		{
			// Convert our format into one the triangulation library supports
			FClipSMPolygon ClipPolygon(0);
			for (int32 VertexIndex = 0; VertexIndex < SourcePoly.Vertices.Num() ; ++VertexIndex)
			{
				FClipSMVertex* ClipVertex = new (ClipPolygon.Vertices) FClipSMVertex;
				FMemory::Memzero(ClipVertex, sizeof(FClipSMVertex));

				const FVector2D& SourceVertex = SourcePoly.Vertices[VertexIndex];
				ClipVertex->Pos.X = SourceVertex.X;
				ClipVertex->Pos.Z = SourceVertex.Y;
			}
			ClipPolygon.FaceNormal = FVector(0.0f, -1.0f, 0.0f);

			// Attempt to triangulate this polygon
			TArray<FClipSMTriangle> GeneratedTriangles;
			if (TriangulatePoly(/*out*/ GeneratedTriangles, ClipPolygon))
			{
				// Convert the triangles back to our 2D data structure
				for (int32 TriangleIndex = 0; TriangleIndex < GeneratedTriangles.Num(); ++TriangleIndex)
				{
					const FClipSMTriangle& Triangle = GeneratedTriangles[TriangleIndex];

					new (Target) FVector2D(Triangle.Vertices[0].Pos.X, Triangle.Vertices[0].Pos.Z);
					new (Target) FVector2D(Triangle.Vertices[1].Pos.X, Triangle.Vertices[1].Pos.Z);
					new (Target) FVector2D(Triangle.Vertices[2].Pos.X, Triangle.Vertices[2].Pos.Z);
				}
			}
		}
	}
}

void UPaperSprite::InitializeSprite(UTexture2D* Texture)
{
	if (Texture != NULL)
	{
		const FVector2D Dimension(Texture->GetSizeX(), Texture->GetSizeY());
		InitializeSprite(Texture, FVector2D::ZeroVector, Dimension);
	}
	else
	{
		InitializeSprite(NULL, FVector2D::ZeroVector, FVector2D(1.0f, 1.0f));
	}
}

void UPaperSprite::InitializeSprite(UTexture2D* Texture, const FVector2D& Offset, const FVector2D& Dimension)
{
	SourceTexture = Texture;
	SourceUV = Offset;
	SourceDimension = Dimension;
	RebuildCollisionData();
	RebuildRenderData();
}

FVector2D UPaperSprite::ConvertTextureSpaceToPivotSpace(FVector2D Input) const
{
	const FVector2D Pivot = GetPivotPosition();

	const float X = Input.X - Pivot.X;
	const float Y = -Input.Y + Pivot.Y;
	
	return FVector2D(X, Y);
}

FVector2D UPaperSprite::ConvertPivotSpaceToTextureSpace(FVector2D Input) const
{
	const FVector2D Pivot = GetPivotPosition();

	const float X = Input.X + Pivot.X;
	const float Y = -Input.Y + Pivot.Y;

	return FVector2D(X, Y);
}

FVector UPaperSprite::ConvertTextureSpaceToPivotSpace(FVector Input) const
{
	const FVector2D Pivot = GetPivotPosition();

	const float X = Input.X - Pivot.X;
	const float Z = -Input.Z + Pivot.Y;

	return FVector(X, Input.Y, Z);
}

FVector UPaperSprite::ConvertPivotSpaceToTextureSpace(FVector Input) const
{
	const FVector2D Pivot = GetPivotPosition();

	const float X = Input.X + Pivot.X;
	const float Z = -Input.Z + Pivot.Y;

	return FVector(X, Input.Y, Z);
}

FVector UPaperSprite::ConvertTextureSpaceToWorldSpace(const FVector2D& SourcePoint) const
{
	const FVector2D SourceDims = (SourceTexture != NULL) ? FVector2D(SourceTexture->GetSurfaceWidth(), SourceTexture->GetSurfaceHeight()) : FVector2D::ZeroVector;
	return (PaperAxisX * SourcePoint.X) + (PaperAxisY * (SourceDims.Y - SourcePoint.Y));
}

// FVector UPaperSprite::ConvertTextureSpaceToWorldSpace(const FVector& SourcePoint) const
// {
// 	const FVector2D SourceDims = (SourceTexture != NULL) ? FVector2D(SourceTexture->GetSurfaceWidth(), SourceTexture->GetSurfaceHeight()) : FVector2D::ZeroVector;
// 	return FVector(SourcePoint.X, SourcePoint.Y, SourceDims.Y - SourcePoint.Z);
// }

FTransform UPaperSprite::GetPivotToWorld() const
{
	const FVector2D Pivot = GetPivotPosition();
	const FVector2D SourceDims = (SourceTexture != NULL) ? FVector2D(SourceTexture->GetSurfaceWidth(), SourceTexture->GetSurfaceHeight()) : FVector2D::ZeroVector;

	const FVector Translation((Pivot.X * PaperAxisX) + ((SourceDims.Y - Pivot.Y) * PaperAxisY));
	return FTransform(Translation);
}

FVector2D UPaperSprite::GetPivotPosition() const
{
	switch (PivotMode)
	{
	case ESpritePivotMode::Top_Left:
		return SourceUV;
	case ESpritePivotMode::Top_Center:
		return FVector2D(SourceUV.X + SourceDimension.X * 0.5f, SourceUV.Y);
	case ESpritePivotMode::Top_Right:
		return FVector2D(SourceUV.X + SourceDimension.X, SourceUV.Y);
	case ESpritePivotMode::Center_Left:
		return FVector2D(SourceUV.X, SourceUV.Y + SourceDimension.Y * 0.5f);
	case ESpritePivotMode::Center_Center:
		return FVector2D(SourceUV.X + SourceDimension.X * 0.5f, SourceUV.Y + SourceDimension.Y * 0.5f);
	case ESpritePivotMode::Center_Right:
		return FVector2D(SourceUV.X + SourceDimension.X, SourceUV.Y + SourceDimension.Y * 0.5f);
	case ESpritePivotMode::Bottom_Left:
		return FVector2D(SourceUV.X, SourceUV.Y + SourceDimension.Y);
	case ESpritePivotMode::Bottom_Center:
		return FVector2D(SourceUV.X + SourceDimension.X * 0.5f, SourceUV.Y + SourceDimension.Y);
	case ESpritePivotMode::Bottom_Right:
		return SourceUV + SourceDimension;

	default:
	case ESpritePivotMode::Custom:
		return CustomPivotPoint;
		break;
	};
}

#endif

bool UPaperSprite::GetPhysicsTriMeshData(FTriMeshCollisionData* OutCollisionData, bool InUseAllTriData)
{
	//@TODO: Probably want to support this
	return false;
}

bool UPaperSprite::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	//@TODO: Probably want to support this
	return false;
}

FBoxSphereBounds UPaperSprite::GetRenderBounds() const
{
	FBox BoundingBox(ForceInit);
	
	for (int32 VertexIndex = 0; VertexIndex < BakedRenderData.Num(); ++VertexIndex)
	{
		const FVector4& VertXYUV = BakedRenderData[VertexIndex];
		const FVector Vert((PaperAxisX * VertXYUV.X) + (PaperAxisY * VertXYUV.Y));
		BoundingBox += Vert;
	}
	
	// Make the whole thing a single unit 'deep'
	const FVector HalfThicknessVector = 0.5f * PaperAxisZ;
	BoundingBox.Min -= HalfThicknessVector;
	BoundingBox.Max += HalfThicknessVector;

	return FBoxSphereBounds(BoundingBox);
}

FPaperSpriteSocket* UPaperSprite::FindSocket(FName SocketName)
{
	for (int32 SocketIndex = 0; SocketIndex < Sockets.Num(); ++SocketIndex)
	{
		FPaperSpriteSocket& Socket = Sockets[SocketIndex];
		if (Socket.SocketName == SocketName)
		{
			return &Socket; 
		}
	}

	return NULL;
}

void UPaperSprite::QuerySupportedSockets(TArray<FComponentSocketDescription>& OutSockets) const
{
	for (int32 SocketIndex = 0; SocketIndex < Sockets.Num(); ++SocketIndex)
	{
		const FPaperSpriteSocket& Socket = Sockets[SocketIndex];
		new (OutSockets) FComponentSocketDescription(Socket.SocketName, EComponentSocketType::Socket);
	}
}
