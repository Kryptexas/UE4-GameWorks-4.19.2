// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

//////////////////////////////////////////////////////////////////////////
// FSelectionTypes

class FSelectionTypes
{
public:
	static const FName Vertex;
	static const FName Edge;
	static const FName Pivot;
	static const FName Socket;
private:
	FSelectionTypes() {}
};

//////////////////////////////////////////////////////////////////////////
// FSelectedItem

class FSelectedItem
{
protected:
	FSelectedItem(FName InTypeName)
		: TypeName(InTypeName)
	{
	}

protected:
	FName TypeName;
public:
	virtual bool IsA(FName TestType) const
	{
		return TestType == TypeName;
	}

	virtual uint32 GetTypeHash() const
	{
		return 0;
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const
	{
		return false;
	}

	virtual void ApplyDelta(const FVector2D& Delta)
	{
	}

	//@TODO: Doesn't belong here in base!
	virtual void Delete()
	{
	}

	//@TODO: Doesn't belong here in base!
	virtual void SplitEdge()
	{
	}

	virtual FVector GetWorldPos() const
	{
		return FVector::ZeroVector;
	}

	virtual ~FSelectedItem() {}
};

inline uint32 GetTypeHash(const FSelectedItem& Vertex)
{
	return Vertex.GetTypeHash();
}

inline bool operator==(const FSelectedItem& V1, const FSelectedItem& V2)
{
	return V1.Equals(V2);
}

//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedVertex

class FSpriteSelectedVertex : public FSelectedItem
{
public:
	int32 VertexIndex;
	int32 PolygonIndex;

	// If true, it's render data, otherwise it's collision data
	bool bRenderData;

	TWeakObjectPtr<UPaperSprite> SpritePtr;
public:
	FSpriteSelectedVertex()
		: FSelectedItem(FSelectionTypes::Vertex)
		, VertexIndex(0)
		, PolygonIndex(0)
		, bRenderData(false)
	{
	}

	virtual uint32 GetTypeHash() const OVERRIDE
	{
		return VertexIndex + (PolygonIndex * 311) + (bRenderData ? 1063 : 0);
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const OVERRIDE
	{
		if (OtherItem.IsA(FSelectionTypes::Vertex))
		{
			const FSpriteSelectedVertex& V1 = *this;
			const FSpriteSelectedVertex& V2 = *(FSpriteSelectedVertex*)(&OtherItem);

			return (V1.VertexIndex == V2.VertexIndex) && (V1.PolygonIndex == V2.PolygonIndex) && (V1.bRenderData == V2.bRenderData) && (V1.SpritePtr == V2.SpritePtr);
		}
		else
		{
			return false;
		}
	}

	virtual void ApplyDeltaIndexed(const FVector2D& Delta, int32 TargetVertexIndex)
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			FSpritePolygonCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;
			if (Geometry.Polygons.IsValidIndex(PolygonIndex))
			{
				TArray<FVector2D>& Vertices = Geometry.Polygons[PolygonIndex].Vertices;
				TargetVertexIndex = (Vertices.Num() > 0) ? (TargetVertexIndex % Vertices.Num()) : 0;
				if (Vertices.IsValidIndex(TargetVertexIndex))
				{
					Vertices[TargetVertexIndex] += Delta;//@TODO: Should apply the inverse transform (in case scale/rotation is supported later)

					Geometry.GeometryType = ESpritePolygonMode::FullyCustom;
				}
			}
		}
	}

	FVector GetWorldPosIndexed(int32 TargetVertexIndex) const
	{
		FVector Result = FVector::ZeroVector;

		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			FSpritePolygonCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;

			if (Geometry.Polygons.IsValidIndex(PolygonIndex))
			{
				const TArray<FVector2D>& Vertices = Geometry.Polygons[PolygonIndex].Vertices;
				TargetVertexIndex = (Vertices.Num() > 0) ? (TargetVertexIndex % Vertices.Num()) : 0;
				if (Vertices.IsValidIndex(TargetVertexIndex))
				{
					UTexture2D* SourceTexture = Sprite->GetSourceTexture();
					const FVector2D SourceDims = (SourceTexture != NULL) ? FVector2D(SourceTexture->GetSurfaceWidth(), SourceTexture->GetSurfaceHeight()) : FVector2D::ZeroVector;

					const FVector2D Result2D = Vertices[TargetVertexIndex];
					Result = (Result2D.X * PaperAxisX) + ((SourceDims.Y - Result2D.Y) * PaperAxisY);
				}
			}
		}

		return Result;
	}

	virtual void ApplyDelta(const FVector2D& Delta) OVERRIDE
	{
		ApplyDeltaIndexed(Delta, VertexIndex);
	}

	FVector GetWorldPos() const OVERRIDE
	{
		return GetWorldPosIndexed(VertexIndex);
	}

	virtual void Delete() OVERRIDE
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			FSpritePolygonCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;
			Geometry.GeometryType = ESpritePolygonMode::FullyCustom;

			if (Geometry.Polygons.IsValidIndex(PolygonIndex))
			{
				TArray<FVector2D>& Vertices = Geometry.Polygons[PolygonIndex].Vertices;
				if (Vertices.IsValidIndex(VertexIndex))
				{
					Vertices.RemoveAt(VertexIndex);
				}

				if (Vertices.Num() < 1)
				{
					Geometry.Polygons.RemoveAt(PolygonIndex);
				}
			}
		}
	}

	virtual void SplitEdge() OVERRIDE
	{
		// Nonsense operation on a vertex, do nothing
	}
};


//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedEdge
//@TODO: Much hackery lies here; need a real data structure!


class FSpriteSelectedEdge : public FSpriteSelectedVertex
{
	// Note: Defined based on a vertex index; this is the edge between the vertex and the next one.
public:
	FSpriteSelectedEdge()
	{
		TypeName = FSelectionTypes::Edge;
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const OVERRIDE
	{
		if (OtherItem.IsA(FSelectionTypes::Edge))
		{
			const FSpriteSelectedEdge& E1 = *this;
			const FSpriteSelectedEdge& E2 = *(FSpriteSelectedEdge*)(&OtherItem);

			return (E1.VertexIndex == E2.VertexIndex) && (E1.PolygonIndex == E2.PolygonIndex) && (E1.bRenderData == E2.bRenderData) && (E1.SpritePtr == E2.SpritePtr);
		}
		else
		{
			return false;
		}
	}

	virtual void ApplyDelta(const FVector2D& Delta) OVERRIDE
	{
		ApplyDeltaIndexed(Delta, VertexIndex);
		ApplyDeltaIndexed(Delta, VertexIndex+1);
	}

	FVector GetWorldPos() const OVERRIDE
	{
		const FVector Pos1 = GetWorldPosIndexed(VertexIndex);
		const FVector Pos2 = GetWorldPosIndexed(VertexIndex+1);

		return (Pos1 + Pos2) * 0.5f;
	}

	virtual void Delete() OVERRIDE
	{
		//@TODO: Support deleting edges
	}

	virtual void SplitEdge() OVERRIDE
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			FSpritePolygonCollection& Geometry = bRenderData ? Sprite->RenderGeometry : Sprite->CollisionGeometry;
			Geometry.GeometryType = ESpritePolygonMode::FullyCustom;

			if (Geometry.Polygons.IsValidIndex(PolygonIndex))
			{
				TArray<FVector2D>& Vertices = Geometry.Polygons[PolygonIndex].Vertices;
				if (Vertices.IsValidIndex(VertexIndex))
				{
					const int32 NextVertexIndex = (VertexIndex + 1) % Vertices.Num();

					const FVector2D NewPos = (Vertices[VertexIndex] + Vertices[NextVertexIndex]) * 0.5f;
					
					Vertices.Insert(NewPos, VertexIndex+1);
				}
			}
		}
	}
};



//////////////////////////////////////////////////////////////////////////
// FSpriteSelectedSocket

class FSpriteSelectedSocket : public FSelectedItem
{
public:
	FName SocketName;
	TWeakObjectPtr<UPaperSprite> SpritePtr;

public:
	FSpriteSelectedSocket()
		: FSelectedItem(FSelectionTypes::Socket)
	{
	}

	virtual bool Equals(const FSelectedItem& OtherItem) const OVERRIDE
	{
		if (OtherItem.IsA(FSelectionTypes::Socket))
		{
			const FSpriteSelectedSocket& S1 = *this;
			const FSpriteSelectedSocket& S2 = *(FSpriteSelectedSocket*)(&OtherItem);

			return (S1.SocketName == S2.SocketName) && (S1.SpritePtr == S2.SpritePtr);
		}
		else
		{
			return false;
		}
	}

	//@TODO: Currently sockets are in unflipped pivot space,
	virtual void ApplyDelta(const FVector2D& Delta) OVERRIDE
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			if (FPaperSpriteSocket* Socket = Sprite->FindSocket(SocketName))
			{
				const FVector Delta3D = (PaperAxisX * Delta.X) + (PaperAxisY * -Delta.Y);
				Socket->LocalTransform.SetLocation(Socket->LocalTransform.GetLocation() + Delta3D);
			}
		}
	}

	FVector GetWorldPos() const OVERRIDE
	{
		if (UPaperSprite* Sprite = SpritePtr.Get())
		{
			if (FPaperSpriteSocket* Socket = Sprite->FindSocket(SocketName))
			{
 				const FVector PivotSpacePos = Socket->LocalTransform.GetLocation();
				return Sprite->GetPivotToWorld().TransformPosition(PivotSpacePos);
			}
		}

		return FVector::ZeroVector;
	}

	virtual void Delete() OVERRIDE
	{
		//@TODO: Support deleting edges
	}

	virtual void SplitEdge() OVERRIDE
	{
		// Nonsense operation on a socket, do nothing
	}
};
