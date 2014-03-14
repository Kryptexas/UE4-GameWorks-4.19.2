// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NavLinkDefinition.h"
#include "NavigationModifier.h"
#include "NavigationTypes.generated.h"

#define INVALID_NAVNODEREF (0)
#define INVALID_NAVQUERYID uint32(0)
#define INVALID_NAVDATA uint32(0)
#define INVALID_NAVEXTENT (FVector::ZeroVector)
#define INVALID_MOVEREQUESTID uint32(-1)

/** uniform identifier type for navigation data elements may it be a polygon or graph node */
typedef uint64 NavNodeRef;

//////////////////////////////////////////////////////////////////////////
// Navigation data generation

namespace ENavigationDirtyFlag
{
	enum Type
	{
		Geometry		= (1 << 0),
		DynamicModifier	= (1 << 1),

		All				= 0xffff,
	};
}

struct FNavigationDirtyArea
{
	FBox Bounds;
	int32 Flags;
	
	FNavigationDirtyArea() : Flags(0) {}
	FNavigationDirtyArea(const FBox& InBounds, int32 InFlags) : Bounds(InBounds), Flags(InFlags) {}
	FORCEINLINE bool HasFlag(ENavigationDirtyFlag::Type Flag) const { return (Flags & Flag) != 0; }
};

struct FNavigationDirtyElement
{
	/** object owning this element */
	FWeakObjectPtr Owner;
	
	/** override for update flags */
	int32 FlagsOverride;
	
	/** flags of already existing entry for this actor */
	int32 PrevFlags;
	
	/** override to actor bounds */
	FBox BoundsOverride;
	
	/** bounds of already existing entry for this actor */
	FBox PrevBounds;

	/** override bounds are set */
	uint8 bHasBoundsOverride : 1;

	/** prev flags & bounds data are set */
	uint8 bHasPrevData : 1;

	/** request was invalidated while queued, use prev values to dirty area */
	uint8 bInvalidRequest : 1;

	FNavigationDirtyElement()
		: FlagsOverride(0), bHasBoundsOverride(false), bHasPrevData(false), bInvalidRequest(false)
	{

	}

	FNavigationDirtyElement(UObject* InOwner, int32 InFlagsOverride = 0)
		: Owner(InOwner), FlagsOverride(InFlagsOverride), bHasBoundsOverride(false), bHasPrevData(false), bInvalidRequest(false)
	{
	}

	void SetBounds(const FBox& InBounds)
	{
		BoundsOverride = InBounds;
		bHasBoundsOverride = true;
	}

	bool operator==(const FNavigationDirtyElement& Other) const 
	{ 
		if (bHasBoundsOverride)
		{
			if (BoundsOverride.IsValid != Other.BoundsOverride.IsValid ||
				BoundsOverride.Min != Other.BoundsOverride.Min ||
				BoundsOverride.Max != Other.BoundsOverride.Max)
			{
				return false;
			}
		}
	
		return Owner == Other.Owner; 
	}

	bool operator==(const UObject*& OtherOwner) const 
	{ 
		return (Owner == OtherOwner);
	}

	FORCEINLINE friend uint32 GetTypeHash(const FNavigationDirtyElement& Info)
	{
		return GetTypeHash(Info.Owner);
	}
};

//////////////////////////////////////////////////////////////////////////
// Path

/** Describes a point in navigation data */
struct FNavLocation
{
	/** location relative to path's base */
	FVector Location;

	/** node reference in navigation data */
	NavNodeRef NodeRef;

	FNavLocation() : Location(FVector::ZeroVector), NodeRef(INVALID_NAVNODEREF) {}
	FNavLocation(const FVector& InLocation, NavNodeRef InNodeRef = INVALID_NAVNODEREF) : Location(InLocation), NodeRef(InNodeRef) {}
};

/** Describes node in navigation path */
struct FNavPathPoint : public FNavLocation
{
	/** extra node flags */
	uint32 Flags;

	// always add smart link data to path points (8 bytes), if there will be more "edge types" 
	// we change this implementation to something more generic

	/** smart link owning this node */
	TWeakObjectPtr<class USmartNavLinkComponent> SmartLink;

	FNavPathPoint() : Flags(0) {}
	FNavPathPoint(const FVector& InLocation, NavNodeRef InNodeRef = INVALID_NAVNODEREF, uint32 InFlags = 0) : FNavLocation(InLocation, InNodeRef), Flags(InFlags) {}
};

/** path type data */
struct FNavPathType
{
	FNavPathType() : Id(++NextUniqueId) {}
	FNavPathType(const FNavPathType& Src) : Id(Src.Id) {}
	
	bool operator==(const FNavPathType& Other) const
	{
		return Id == Other.Id;
	}

private:
	static uint32 NextUniqueId;
	uint32 Id;
};

typedef TSharedPtr<struct FNavigationPath, ESPMode::ThreadSafe> FNavPathSharedPtr;
typedef TWeakPtr<struct FNavigationPath, ESPMode::ThreadSafe> FNavPathWeakPtr;

struct FNavigationPath : public TSharedFromThis<FNavigationPath, ESPMode::ThreadSafe>
{
	DECLARE_DELEGATE_OneParam(FPathObserverDelegate, FNavigationPath*);

	FNavigationPath();
	FNavigationPath(const TArray<FVector>& Points, AActor* Base = NULL);
	virtual ~FNavigationPath() {}

	FORCEINLINE bool IsValid() const { return bIsReady && PathPoints.Num() > 1 && bUpToDate; }
	FORCEINLINE bool IsUpToDate() const { return bUpToDate; }
	FORCEINLINE bool IsReady() const { return bIsReady; }
	FORCEINLINE bool IsPartial() const { return bIsPartial; }
	FORCEINLINE bool DidSearchReachedLimit() const { return bReachedSearchLimit; }
	FORCEINLINE class ANavigationData *GetOwner() const { return Owner.Get(); }
	FORCEINLINE FVector GetDestinationLocation() const { return IsValid() ? PathPoints.Last().Location : INVALID_NAVEXTENT; }

	FORCEINLINE void MarkReady() { bIsReady = true; }
	FORCEINLINE void SetOwner(const class ANavigationData* const NewOwner) { Owner = NewOwner; }
	FORCEINLINE void SetObserver(const FPathObserverDelegate& Observer) { ObserverDelegate = Observer; }
	FORCEINLINE void SetIsPartial(const bool bPartial) { bIsPartial = bPartial; }
	FORCEINLINE void SetSearchReachedLimit(const bool bLimited) { bReachedSearchLimit = bLimited; }
	
	FORCEINLINE void Invalidate() 
	{ 
		bUpToDate = false; 
		ObserverDelegate.ExecuteIfBound(this); 
	}

	virtual void DebugDraw(const class ANavigationData* NavData, FColor PathColor, class UCanvas* Canvas, bool bPersistent, const uint32 NextPathPointIndex = 0) const;
#if ENABLE_VISUAL_LOG
	virtual void DescribeSelfToVisLog(struct FVisLogEntry* Snapshot) const;
#endif // ENABLE_VISUAL_LOG

	/** check if path contains specific smart nav link */
	bool ContainsSmartLink(class USmartNavLinkComponent* Link) const;

	/** check if path contains any smart nav link */
	bool ContainsAnySmartLink() const;

	/** check if path contains given node */
	virtual bool ContainsNode(NavNodeRef NodeRef) const;

	/** get cost of path, starting from given point */
	virtual float GetCostFromIndex(int32 PathPointIndex) const { return 0.0f; }

	/** get cost of path, starting from given node */
	virtual float GetCostFromNode(NavNodeRef PathNode) const { return 0.0f; }

	FORCEINLINE float GetCost() const { return GetCostFromIndex(0); }

	/** calculates total length of segments from NextPathPoint to the end of path, plus distance from CurrentPosition to NextPathPoint */
	ENGINE_API float GetLengthFromPosition(FVector SegmentStart, uint32 NextPathPointIndex) const;

	FORCEINLINE float GetLength() const { return PathPoints.Num() ? GetLengthFromPosition(PathPoints[0].Location, 1) : 0.0f; }

	static bool GetPathPoint(const FNavigationPath* Path, uint32 PathVertIdx, FNavPathPoint& PathPoint)
	{
		if (Path && Path->PathPoints.IsValidIndex((int32)PathVertIdx))
		{
			PathPoint = Path->PathPoints[PathVertIdx];
			return true;
		}

		return false;
	}

	/** type safe casts */
	template<typename PathClass>
	FORCEINLINE const PathClass* CastPath() const
	{
		return PathType == PathClass::Type ? static_cast<PathClass*>(this) : NULL;
	}

	template<typename PathClass>
	FORCEINLINE PathClass* CastPath()
	{
		return PathType == PathClass::Type ? static_cast<PathClass*>(this) : NULL;
	}

public:

	/** 
	 * IMPORTANT: path is assumed to be valid if it contains _MORE_ than _ONE_ point 
	 *	point 0 is path's starting point - if it's the only point on the path then there's no path per se
	 */
	TArray<FNavPathPoint> PathPoints;

	/** base actor, if exist path points locations will be relative to it */
	TWeakObjectPtr<AActor> Base;

	/** type of path */
	static const FNavPathType Type;

protected:

	FNavPathType PathType;

	/** A delegate that will be called when path becomes invalid */
	FPathObserverDelegate ObserverDelegate;

	/** "true" until navigation data used to generate this path has been changed/invalidated */
	uint32 bUpToDate : 1;

	/** when false it means path instance has been created, but not filled with data yet */
	uint32 bIsReady : 1;

	/** "true" when path is only partially generated, when goal is unreachable and path represent best guess */
	uint32 bIsPartial : 1;

	/** set to true when path finding algorithm reached a technical limit (like limit of A* nodes).
	 *	This generally means path cannot be trusted to lead to requested destination
	 *	although it might lead closer to destination. */
	uint32 bReachedSearchLimit : 1;

	/** Identifier of navigation data used to generate this path */
	TWeakObjectPtr<class ANavigationData> Owner;
};

//////////////////////////////////////////////////////////////////////////
// Custom path following data

/** Custom data passed to movement requests. */
struct ENGINE_API FMoveRequestCustomData
{
};

typedef TSharedPtr<FMoveRequestCustomData, ESPMode::ThreadSafe> FCustomMoveSharedPtr;
typedef TWeakPtr<FMoveRequestCustomData, ESPMode::ThreadSafe> FCustomMoveWeakPtr;

UCLASS(Abstract, CustomConstructor)
class UNavigationTypes : public UObject
{
	GENERATED_UCLASS_BODY()

	UNavigationTypes(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP) { }
};
