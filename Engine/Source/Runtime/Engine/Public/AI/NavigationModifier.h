// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once 

struct ENGINE_API FNavigationModifier
{
	FNavigationModifier() : bHasMetaAreas(false) {}
	FORCEINLINE bool HasMetaAreas() const { return !!bHasMetaAreas; }

protected:
	/** set to true if any of areas used by this modifier is a meta nav area (UNavAreaMeta subclass)*/
	int32 bHasMetaAreas : 1;
};

namespace ENavigationShapeType
{
	enum Type
	{
		Unknown,
		Cylinder,
		Box,
		Convex,
	};
}

namespace ENavigationCoordSystem
{
	enum Type
	{
		Unreal,
		Recast,
	};
}

/** Area modifier: cylinder shape */
struct FCylinderNavAreaData
{
	FVector Origin;
	float Radius;
	float Height;
};

/** Area modifier: box shape (AABB) */
struct FBoxNavAreaData
{
	FVector Origin;
	FVector Extent;
};

struct FConvexNavAreaData
{
	TArray<FVector> Points;
	float MinZ;
	float MaxZ;
};

/** Area modifier: base */
struct ENGINE_API FAreaNavModifier : public FNavigationModifier
{
	/** transient value used for navigation modifiers sorting. If < 0 then not set*/
	float Cost;
	float FixedCost;

	FAreaNavModifier() : Cost(0.0f), FixedCost(0.0f), ShapeType(ENavigationShapeType::Unknown), AreaClass(NULL) {}
	FAreaNavModifier(float Radius, float Height, const FTransform& LocalToWorld, const TSubclassOf<class UNavArea> AreaClass);
	FAreaNavModifier(const FVector& Extent, const FTransform& LocalToWorld, const TSubclassOf<class UNavArea> AreaClass);
	FAreaNavModifier(const FBox& Box, const FTransform& LocalToWorld, const TSubclassOf<class UNavArea> AreaClass);
	FAreaNavModifier(const TArray<FVector>& Points, ENavigationCoordSystem::Type CoordType, const FTransform& LocalToWorld, const TSubclassOf<class UNavArea> AreaClass);
	FAreaNavModifier(const TArray<FVector>& Points, const int32 FirstIndex, const int32 LastIndex, ENavigationCoordSystem::Type CoordType, const FTransform& LocalToWorld, const TSubclassOf<class UNavArea> AreaClass);
	FAreaNavModifier(const class UBrushComponent* BrushComponent, const TSubclassOf<class UNavArea> AreaClass);

	FORCEINLINE const FBox& GetBounds() const { return Bounds; }
	FORCEINLINE ENavigationShapeType::Type GetShapeType() const { return ShapeType; }
	FORCEINLINE const TSubclassOf<class UNavArea> GetAreaClass() const { return AreaClass; }
	void SetAreaClass(const TSubclassOf<class UNavArea> AreaClass);

	void GetCylinder(FCylinderNavAreaData& Data) const;
	void GetBox(FBoxNavAreaData& Data) const;
	void GetConvex(FConvexNavAreaData& Data) const;

protected:
	TArray<FVector> Points;
	TEnumAsByte<ENavigationShapeType::Type> ShapeType;

	/** this should take a value of a game specific navigation modifier	*/
	TSubclassOf<class UNavArea> AreaClass;
	FBox Bounds;

	void Init(const TSubclassOf<class UNavArea> AreaClass);
	void SetConvex(const TArray<FVector>& Points, const int32 FirstIndex, const int32 LastIndex, ENavigationCoordSystem::Type CoordType, const FTransform& LocalToWorld);
	void SetBox(const FBox& Box, const FTransform& LocalToWorld);
};

/**
 *	This modifier allows defining ad-hoc navigation links defining 
 *	connections in an straightforward way.
 */
struct ENGINE_API FSimpleLinkNavModifier : public FNavigationModifier
{
	/** use Set/Append/Add function to update links, they will take care of meta areas */
	TArray<FNavigationLink> Links;
	TArray<FNavigationSegmentLink> SegmentLinks;
	FTransform LocalToWorld;
	int32 UserId;

	FSimpleLinkNavModifier() 
		: bHasFallDownLinks(false)
		, bHasMetaAreasPoint(false)
		, bHasMetaAreasSegment(false)
	{
	}

	FSimpleLinkNavModifier(const FNavigationLink& InLink, const FTransform& InLocalToWorld) 
		: LocalToWorld(InLocalToWorld)
		, bHasFallDownLinks(false)
		, bHasMetaAreasPoint(false)
		, bHasMetaAreasSegment(false)
	{
		UserId = InLink.UserId;
		AddLink(InLink);
	}	

	FSimpleLinkNavModifier(const TArray<FNavigationLink>& InLinks, const FTransform& InLocalToWorld) 
		: LocalToWorld(InLocalToWorld)
		, bHasFallDownLinks(false)
		, bHasMetaAreasPoint(false)
		, bHasMetaAreasSegment(false)
	{
		if (InLinks.Num() > 0)
		{
			UserId = InLinks[0].UserId;
			SetLinks(InLinks);
		}
	}

	FSimpleLinkNavModifier(const FNavigationSegmentLink& InLink, const FTransform& InLocalToWorld) 
		: LocalToWorld(InLocalToWorld)
		, bHasFallDownLinks(false)
		, bHasMetaAreasPoint(false)
		, bHasMetaAreasSegment(false)
	{
		UserId = InLink.UserId;
		AddSegmentLink(InLink);
	}	

	FSimpleLinkNavModifier(const TArray<FNavigationSegmentLink>& InSegmentLinks, const FTransform& InLocalToWorld) 
		: LocalToWorld(InLocalToWorld)
		, bHasFallDownLinks(false)
		, bHasMetaAreasPoint(false)
		, bHasMetaAreasSegment(false)
	{
		if (InSegmentLinks.Num() > 0)
		{
			UserId = InSegmentLinks[0].UserId;
			SetSegmentLinks(InSegmentLinks);
		}
	}

	FORCEINLINE bool HasFallDownLinks() const { return !!bHasFallDownLinks; }
	void SetLinks(const TArray<FNavigationLink>& InLinks);
	void SetSegmentLinks(const TArray<FNavigationSegmentLink>& InLinks);
	void AppendLinks(const TArray<FNavigationLink>& InLinks);
	void AppendSegmentLinks(const TArray<FNavigationSegmentLink>& InLinks);
	void AddLink(const FNavigationLink& InLink);
	void AddSegmentLink(const FNavigationSegmentLink& InLink);

protected:
	/** set to true if any of links stored is a "fall down" link, i.e. requires vertical snapping to geometry */
	int32 bHasFallDownLinks : 1;
	int32 bHasMetaAreasPoint : 1;
	int32 bHasMetaAreasSegment : 1;
};

struct ENGINE_API FCustomLinkNavModifier : public FNavigationModifier
{
	FTransform LocalToWorld;

	FCustomLinkNavModifier() : LinkDefinitionClass(NULL) {}
	void Set(TSubclassOf<class UNavLinkDefinition> LinkDefinitionClass, const FTransform& LocalToWorld);
	FORCEINLINE const TSubclassOf<class UNavLinkDefinition> GetNavLinkClass() const { return LinkDefinitionClass; }

protected:
	TSubclassOf<class UNavLinkDefinition> LinkDefinitionClass;
};

struct ENGINE_API FCompositeNavModifier : public FNavigationModifier
{
	void Reset();
	void Empty();

	FORCEINLINE bool IsEmpty() const 
	{ 
		return (Areas.Num() == 0) && (SimpleLinks.Num() == 0) && (CustomLinks.Num() == 0);
	}

	void Add(const FAreaNavModifier& Area) { Areas.Add(Area); bHasMetaAreas |= Area.HasMetaAreas(); }
	void Add(const FSimpleLinkNavModifier& Link) { SimpleLinks.Add(Link); bHasMetaAreas |= Link.HasMetaAreas(); }
	void Add(const FCustomLinkNavModifier& Link) { CustomLinks.Add(Link); bHasMetaAreas |= Link.HasMetaAreas(); }
	void Add(const FCompositeNavModifier& Modifiers) { Areas.Append(Modifiers.Areas); SimpleLinks.Append(Modifiers.SimpleLinks); CustomLinks.Append(Modifiers.CustomLinks); bHasMetaAreas |= Modifiers.bHasMetaAreas; }

	FORCEINLINE const TArray<FAreaNavModifier>& GetAreas() const { return Areas; }
	FORCEINLINE const TArray<FSimpleLinkNavModifier>& GetSimpleLinks() const { return SimpleLinks; }
	FORCEINLINE const TArray<FCustomLinkNavModifier>& GetCustomLinks() const { return CustomLinks; }
	
	FORCEINLINE bool HasLinks() const { return (SimpleLinks.Num() > 0) || (CustomLinks.Num() > 0); }
	FORCEINLINE bool HasAreas() const { return Areas.Num() > 0; }

	/** returns a copy of Modifier */
	FCompositeNavModifier GetInstantiatedMetaModifier(const struct FNavAgentProperties* NavAgent, TWeakObjectPtr<UObject> WeakOwnerPtr) const;
	uint32 GetAllocatedSize() const;

private:
	TArray<FAreaNavModifier> Areas;
	TArray<FSimpleLinkNavModifier> SimpleLinks;
	TArray<FCustomLinkNavModifier> CustomLinks;
};
