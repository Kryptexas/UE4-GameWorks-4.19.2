// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NavLinkDefinition.h"
#include "NavigationModifier.h"
#include "NavigationAvoidanceTypes.h"
#include "NavigationTypes.generated.h"

#define INVALID_NAVNODEREF (0)
#define INVALID_NAVQUERYID uint32(0)
#define INVALID_NAVDATA uint32(0)
#define INVALID_NAVEXTENT (FVector::ZeroVector)

#define DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL 50.f
#define DEFAULT_NAV_QUERY_EXTENT_VERTICAL 100.f

/** Whether to compile in navigation data generation - should be compiled at least when WITH_EDITOR is true */
#define WITH_NAVIGATION_GENERATOR (WITH_RECAST || WITH_EDITOR)

/** uniform identifier type for navigation data elements may it be a polygon or graph node */
typedef uint64 NavNodeRef;

namespace FNavigationSystem
{
	/** used as a fallback value for navigation agent radius, when none specified via UNavigationSystem::SupportedAgents */
	extern const float FallbackAgentRadius;

	/** used as a fallback value for navigation agent height, when none specified via UNavigationSystem::SupportedAgents */
	extern const float FallbackAgentHeight;

	static const FBox InvalidBoundingBox(0);

	enum ECreateIfEmpty
	{
		Invalid = -1,
		DontCreate = 0,
		Create = 1,
	};
}

//////////////////////////////////////////////////////////////////////////
// Navigation data generation

namespace ENavigationDirtyFlag
{
	enum Type
	{
		Geometry		= (1 << 0),
		DynamicModifier	= (1 << 1),
		UseAgentHeight  = (1 << 2),

		All				= Geometry | DynamicModifier,		// all rebuild steps here without additional flags
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

struct FNavigationPortalEdge
{
	FVector Left;
	FVector Right;
	NavNodeRef ToRef;

	FNavigationPortalEdge() : Left(0.f), Right(0.f)
	{}

	FNavigationPortalEdge(const FVector& InLeft, const FVector& InRight, NavNodeRef InToRef)
		: Left(InLeft), Right(InRight), ToRef(InToRef)
	{}

	FORCEINLINE FVector GetPoint(const int32 Index) const
	{
		check(Index >= 0 && Index < 2);
		return ((FVector*)&Left)[Index];
	}

	FORCEINLINE float GetLength() const { return FVector::Dist(Left, Right); }

	FORCEINLINE FVector GetMiddlePoint() const { return Left + (Right - Left) / 2; }
};

/** Describes a point in navigation data */
struct FNavLocation
{
	/** location relative to path's base */
	FVector Location;

	/** node reference in navigation data */
	NavNodeRef NodeRef;

	FNavLocation() : Location(FVector::ZeroVector), NodeRef(INVALID_NAVNODEREF) {}
	FNavLocation(const FVector& InLocation, NavNodeRef InNodeRef = INVALID_NAVNODEREF) 
		: Location(InLocation), NodeRef(InNodeRef) {}
};

/** Describes node in navigation path */
struct FNavPathPoint : public FNavLocation
{
	/** extra node flags */
	uint32 Flags;

	/** unique Id of custom navigation link starting at this point */
	uint32 CustomLinkId;

	FNavPathPoint() : Flags(0), CustomLinkId(0) {}
	FNavPathPoint(const FVector& InLocation, NavNodeRef InNodeRef = INVALID_NAVNODEREF, uint32 InFlags = 0) 
		: FNavLocation(InLocation, InNodeRef), Flags(InFlags), CustomLinkId(0) {}
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

struct ENGINE_API FNavigationPath : public TSharedFromThis<FNavigationPath, ESPMode::ThreadSafe>
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
	FORCEINLINE bool IsDirect() const { return Owner.Get() == NULL; }
	FORCEINLINE FVector GetDestinationLocation() const { return IsValid() ? PathPoints.Last().Location : INVALID_NAVEXTENT; }
	FORCEINLINE void GetObserver(FPathObserverDelegate& Observer) const { Observer = ObserverDelegate; }

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
	virtual FString GetDescription() const;
#endif // ENABLE_VISUAL_LOG

	/** check if path contains specific custom nav link */
	bool ContainsCustomLink(uint32 UniqueLinkId) const;

	/** check if path contains any custom nav link */
	bool ContainsAnyCustomLink() const;

	/** check if path contains given node */
	virtual bool ContainsNode(NavNodeRef NodeRef) const;

	/** get cost of path, starting from given point */
	virtual float GetCostFromIndex(int32 PathPointIndex) const { return 0.0f; }

	/** get cost of path, starting from given node */
	virtual float GetCostFromNode(NavNodeRef PathNode) const { return 0.0f; }

	FORCEINLINE float GetCost() const { return GetCostFromIndex(0); }

	/** calculates total length of segments from NextPathPoint to the end of path, plus distance from CurrentPosition to NextPathPoint */
	float GetLengthFromPosition(FVector SegmentStart, uint32 NextPathPointIndex) const;

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

	FORCEINLINE const TArray<FNavPathPoint>& GetPathPoints() const { return PathPoints; }

	virtual bool DoesIntersectBox(const FBox& Box, int32* IntersectingSegmentIndex = NULL) const;

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

	/** additional node refs used during path following shortcuts */
	TArray<NavNodeRef> ShortcutNodeRefs;

	/** base actor, if exist path points locations will be relative to it */
	TWeakObjectPtr<AActor> Base;

	/** filter used to build this path */
	TSharedPtr<const struct FNavigationQueryFilter> Filter;

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

USTRUCT()
struct FMovementProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementProperties)
	uint32 bCanCrouch:1;    // if true, this pawn is capable of crouching

	// movement capabilities - used by AI for reachability tests
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementProperties)
	uint32 bCanJump:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementProperties)
	uint32 bCanWalk:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementProperties)
	uint32 bCanSwim:1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementProperties)
	uint32 bCanFly:1;

	FMovementProperties()
		: bCanCrouch(false)
		, bCanJump(false)
		, bCanWalk(false)
		, bCanSwim(false)
		, bCanFly(false)
	{
	}
};

USTRUCT()
struct FNavAgentProperties : public FMovementProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MovementProperties)
	float AgentRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MovementProperties)
	float AgentHeight;

	FNavAgentProperties(float Radius = -1.f, float Height = -1.f)
		: AgentRadius(Radius)
		, AgentHeight(Height)
	{
	}

	void UpdateWithCollisionComponent(class UShapeComponent* CollisionComponent);

	FORCEINLINE bool IsValid() const { return AgentRadius >= 0 && AgentHeight >= 0; }

	FORCEINLINE bool IsEquivalent(const FNavAgentProperties& Other, float Precision = 5.f) const
	{
		return FGenericPlatformMath::Abs(AgentRadius - Other.AgentRadius) < Precision && FGenericPlatformMath::Abs(AgentHeight - Other.AgentHeight) < Precision;
	}

	bool operator==(const FNavAgentProperties& Other) const
	{
		return IsEquivalent(Other);
	}

	FVector GetExtent() const
	{
		return FVector(AgentRadius, AgentRadius, AgentHeight / 2);
	}
};

inline uint32 GetTypeHash(const FNavAgentProperties& A)
{
	return (int16(A.AgentRadius) << 16) | int16(A.AgentHeight);
}

USTRUCT()
struct ENGINE_API FNavDataConfig : public FNavAgentProperties
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Display)
	FName Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Display)
	FColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Querying, config)
	FVector DefaultQueryExtent;

	FNavDataConfig(float Radius = FNavigationSystem::FallbackAgentRadius, float Height = FNavigationSystem::FallbackAgentHeight)
		: FNavAgentProperties(Radius, Height)
		, Name(TEXT("Default"))
		, Color(140,255,0,164)
		, DefaultQueryExtent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL)
	{
	}	
};

UENUM()
namespace ENavigationQueryResult
{
	enum Type
	{
		Invalid,
		Error,
		Fail,
		Success
	};
}

struct FPathFindingResult
{
	FNavPathSharedPtr Path;
	ENavigationQueryResult::Type Result;

	FPathFindingResult(ENavigationQueryResult::Type InResult = ENavigationQueryResult::Invalid) : Result(InResult)
	{}

	FORCEINLINE bool IsSuccessful() const { return Result == ENavigationQueryResult::Success; }
	FORCEINLINE bool IsPartial() const { return Result == ENavigationQueryResult::Fail && Path.IsValid() && Path->IsPartial(); }
};

struct ENGINE_API FPathFindingQuery
{
	TWeakObjectPtr<const class ANavigationData> NavData;
	TWeakObjectPtr<const UObject> Owner;
	FVector StartLocation;
	FVector EndLocation;
	TSharedPtr<const FNavigationQueryFilter> QueryFilter;

	/** additional flags passed to navigation data handling request */
	int32 NavDataFlags;

	FPathFindingQuery()
		: NavData(NULL)
		, Owner(NULL)
		, StartLocation(FVector::ZeroVector)
		, EndLocation(FVector::ZeroVector)
		, NavDataFlags(0)
	{
	}

	FPathFindingQuery(const FPathFindingQuery& Source);

	FPathFindingQuery(const UObject* InOwner, const class ANavigationData* InNavData, const FVector& Start, const FVector& End, TSharedPtr<const FNavigationQueryFilter> SourceQueryFilter = NULL);
};

namespace EPathFindingMode
{
	enum Type
	{
		Regular,
		Hierarchical,
	};
};

/**
*	Delegate used to communicate that path finding query has been finished.
*	@param uint32 unique Query ID of given query
*	@param ENavigationQueryResult enum expressed query result.
*	@param FNavPathSharedPtr resulting path. Valid only for ENavigationQueryResult == ENavigationQueryResult::Fail
*		(may contain path leading as close to destination as possible)
*		and ENavigationQueryResult == ENavigationQueryResult::Success
*/
DECLARE_DELEGATE_ThreeParams(FNavPathQueryDelegate, uint32, ENavigationQueryResult::Type, FNavPathSharedPtr);

struct FAsyncPathFindingQuery : public FPathFindingQuery
{
	const uint32 QueryID;
	const FNavPathQueryDelegate OnDoneDelegate;
	const TEnumAsByte<EPathFindingMode::Type> Mode;
	FPathFindingResult Result;

	FAsyncPathFindingQuery()
		: QueryID(INVALID_NAVQUERYID)
	{
	}

	FAsyncPathFindingQuery(const UObject* InOwner, const class ANavigationData* InNavData, const FVector& Start, const FVector& End, const FNavPathQueryDelegate& Delegate, TSharedPtr<const FNavigationQueryFilter> SourceQueryFilter);
	FAsyncPathFindingQuery(const FPathFindingQuery& Query, const FNavPathQueryDelegate& Delegate, const EPathFindingMode::Type QueryMode);

protected:
	FORCEINLINE static uint32 GetUniqueID()
	{
		return ++LastPathFindingUniqueID;
	}

	static uint32 LastPathFindingUniqueID;
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

//////////////////////////////////////////////////////////////////////////
// Memory stating

namespace NavMeshMemory
{
#if STATS
	// @todo could be made a more generic solution
	class FNavigationMemoryStat : public FDefaultAllocator
	{
	public:
		typedef FDefaultAllocator Super;

		class ForAnyElementType : public FDefaultAllocator::ForAnyElementType
		{
		public:
			typedef FDefaultAllocator::ForAnyElementType Super;
		private:
			int32 AllocatedSize;
		public:

			ForAnyElementType()
				: AllocatedSize(0)
			{

			}

			/** Destructor. */
			~ForAnyElementType()
			{
				if (AllocatedSize)
				{
					DEC_DWORD_STAT_BY(STAT_NavigationMemory, AllocatedSize);
				}
			}

			void ResizeAllocation(int32 PreviousNumElements, int32 NumElements, int32 NumBytesPerElement)
			{
				const int32 NewSize = NumElements * NumBytesPerElement;
				INC_DWORD_STAT_BY(STAT_NavigationMemory, NewSize - AllocatedSize);
				AllocatedSize = NewSize;

				Super::ResizeAllocation(PreviousNumElements, NumElements, NumBytesPerElement);
			}

		private:
			ForAnyElementType(const ForAnyElementType&);
			ForAnyElementType& operator=(const ForAnyElementType&);
		};
	};

	typedef FNavigationMemoryStat FNavAllocator;
#else
	typedef FDefaultAllocator FNavAllocator;
#endif
}

#if STATS

template <>
struct TAllocatorTraits<NavMeshMemory::FNavigationMemoryStat> : TAllocatorTraits<NavMeshMemory::FNavigationMemoryStat::Super>
{
};

#endif

template<typename InElementType>
class TNavStatArray : public TArray<InElementType, NavMeshMemory::FNavAllocator>
{
public:
	typedef TArray<InElementType, NavMeshMemory::FNavAllocator> Super;

#if PLATFORM_COMPILER_HAS_DEFAULTED_FUNCTIONS

	TNavStatArray() = default;
	TNavStatArray(const TNavStatArray&) = default;
	TNavStatArray& operator=(const TNavStatArray&) = default;

#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

	TNavStatArray(TNavStatArray&&) = default;
	TNavStatArray& operator=(TNavStatArray&&) = default;

#endif

#else

	FORCEINLINE TNavStatArray()
	{
	}

	FORCEINLINE TNavStatArray(const TNavStatArray& Other)
		: Super((const Super&)Other)
	{
	}

	FORCEINLINE TNavStatArray& operator=(const TNavStatArray& Other)
	{
		(Super&)*this = (const Super&)Other;
		return *this;
	}

#if PLATFORM_COMPILER_HAS_RVALUE_REFERENCES

	FORCEINLINE TNavStatArray(TNavStatArray&& Other)
		: Super((Super&&)Other)
	{
	}

	FORCEINLINE TNavStatArray& operator=(TNavStatArray&& Other)
	{
		(Super&)*this = (Super&&)Other;
		return *this;
	}

#endif

#endif
};

template<typename InElementType>
struct TContainerTraits<TNavStatArray<InElementType> > : public TContainerTraitsBase<TNavStatArray<InElementType> >
{
	enum {
		MoveWillEmptyContainer =
		PLATFORM_COMPILER_HAS_RVALUE_REFERENCES &&
		TContainerTraits<typename TNavStatArray<InElementType>::Super>::MoveWillEmptyContainer
	};
};