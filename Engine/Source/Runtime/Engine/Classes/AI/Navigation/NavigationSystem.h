// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NavigationTypes.h"
#include "NavigationSystem.generated.h"

#define NAVSYS_DEBUG (0 && UE_BUILD_DEBUG)
/** Whether to compile in navigation data generation - should be compiled at least when WITH_EDITOR is true */
#define WITH_NAVIGATION_GENERATOR (WITH_RECAST || WITH_EDITOR)
// if we'll be rebuilding navigation at runtime
#define WITH_RUNTIME_NAVIGATION_BUILDING (1 && WITH_NAVIGATION_GENERATOR)

#define NAVOCTREE_CONTAINS_COLLISION_DATA (1 && WITH_RECAST)

#define DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL 50.f
#define DEFAULT_NAV_QUERY_EXTENT_VERTICAL 100.f

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

DECLARE_LOG_CATEGORY_EXTERN(LogNavigation, Warning, All);

/** 
 * 
 */
DECLARE_DELEGATE_RetVal_OneParam(class ANavigationData*, FCreateNavigationDataInstanceDelegate, class UNavigationSystem*);

/** delegate to let interested parties know that new nav area class has been registered */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNavAreaChanged, const class UClass* /*AreaClass*/);

/** Delegate to let interested parties know that Nav Data has been registered */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNavDataRegistered, class ANavigationData*, NavData);

namespace NavigationDebugDrawing
{
	extern const ENGINE_API float PathLineThickness;
	extern const ENGINE_API FVector PathOffeset;
	extern const ENGINE_API FVector PathNodeBoxExtent;
}

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
					DEC_DWORD_STAT_BY( STAT_NavigationMemory, AllocatedSize );
				}
			}

			void ResizeAllocation(int32 PreviousNumElements,int32 NumElements,int32 NumBytesPerElement)
			{
				const int32 NewSize = NumElements * NumBytesPerElement;
				INC_DWORD_STAT_BY( STAT_NavigationMemory, NewSize - AllocatedSize );
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
	enum { MoveWillEmptyContainer =
		PLATFORM_COMPILER_HAS_RVALUE_REFERENCES &&
		TContainerTraits<typename TNavStatArray<InElementType>::Super>::MoveWillEmptyContainer };
};

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

/** 
 *	Delegate used to communicate that path finding query has been finished. 
 *	@param uint32 unique Query ID of given query 
 *	@param ENavigationQueryResult enum expressed query result. 
 *	@param FNavPathSharedPtr resulting path. Valid only for ENavigationQueryResult == ENavigationQueryResult::Fail 
 *		(may contain path leading as close to destination as possible) 
 *		and ENavigationQueryResult == ENavigationQueryResult::Success
 */
DECLARE_DELEGATE_ThreeParams( FNavPathQueryDelegate, uint32, ENavigationQueryResult::Type, FNavPathSharedPtr );

struct FPathFindingResult
{
	FNavPathSharedPtr Path;
	ENavigationQueryResult::Type Result;

	FPathFindingResult(ENavigationQueryResult::Type InResult = ENavigationQueryResult::Invalid) : Result(InResult)
	{}

	FORCEINLINE bool IsSuccessful() const { return Result == ENavigationQueryResult::Success; }
	FORCEINLINE bool IsPartial() const { return Result == ENavigationQueryResult::Fail && Path.IsValid() && Path->IsPartial(); }
};

namespace EPathFindingMode
{
	enum Type
	{
		Regular,
		Hierarchical,
	};
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementProperties)
	float AgentRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=MovementProperties)
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
		return FVector(AgentRadius,AgentRadius,AgentHeight/2);
	}
};

inline uint32 GetTypeHash( const FNavAgentProperties& A )
{
	return (int16(A.AgentRadius) << 16) | int16(A.AgentHeight);
}

namespace NavigationSystem
{
	enum ECreateIfEmpty 
	{
		Invalid = -1,
		DontCreate = 0,
		Create = 1,
	};

	enum EMode
	{
		InvalidMode = -1,
		GameMode,
		EditorMode,
		SimulationMode,
		PIEMode,
	};

	/** used as a fallback value for navigation agent radius, when none specified via UNavigationSystem::SupportedAgents */
	extern const float FallbackAgentRadius;

	/** used as a fallback value for navigation agent height, when none specified via UNavigationSystem::SupportedAgents */
	extern const float FallbackAgentHeight;

	static const FBox InvalidBoundingBox(0);
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

	FNavDataConfig(float Radius = NavigationSystem::FallbackAgentRadius, float Height = NavigationSystem::FallbackAgentHeight)
		: FNavAgentProperties(Radius, Height)
		, Name(TEXT("Default"))
		, Color(140,255,0,164)
		, DefaultQueryExtent(DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_HORIZONTAL, DEFAULT_NAV_QUERY_EXTENT_VERTICAL)
	{
	}	
};

class INavigationQueryFilterInterface
{
public:
	virtual ~INavigationQueryFilterInterface(){}

	virtual void Reset() = 0;

	virtual void SetAreaCost(uint8 AreaType, float Cost) = 0;
	virtual void SetFixedAreaEnteringCost(uint8 AreaType, float Cost) = 0;
	virtual void SetAllAreaCosts(const float* CostArray, const int32 Count) = 0;
	virtual void GetAllAreaCosts(float* CostArray, float* FixedCostArray, const int32 Count) const = 0;
	virtual void SetBacktrackingEnabled(const bool bBacktracking) = 0;
	virtual bool IsBacktrackingEnabled() const = 0;
	virtual bool IsEqual(const INavigationQueryFilterInterface* Other) const = 0;

	virtual class INavigationQueryFilterInterface* CreateCopy() const = 0;
};

struct ENGINE_API FNavigationQueryFilter : public TSharedFromThis<FNavigationQueryFilter>
{
	FNavigationQueryFilter() : QueryFilterImpl(NULL), MaxSearchNodes(DefaultMaxSearchNodes) {}
private:
	FNavigationQueryFilter(const FNavigationQueryFilter& Source);
	FNavigationQueryFilter(const FNavigationQueryFilter* Source);
	FNavigationQueryFilter(const TSharedPtr<FNavigationQueryFilter> Source);
	FNavigationQueryFilter& operator=(const FNavigationQueryFilter& Source);
public:
	void SetAreaCost(uint8 AreaType, float Cost);

	void SetFixedAreaEnteringCost(uint8 AreaType, float Cost);
	
	void SetAllAreaCosts(const TArray<float>& CostArray);

	void SetAllAreaCosts(const float* CostArray, const int32 Count);

	void GetAllAreaCosts(float* CostArray, float* FixedCostArray, const int32 Count) const;

	void SetMaxSearchNodes(const uint32 MaxNodes) { MaxSearchNodes = MaxNodes; }
	FORCEINLINE uint32 GetMaxSearchNodes() const { return MaxSearchNodes; }

	void SetBacktrackingEnabled(const bool bBacktracking) {	QueryFilterImpl->SetBacktrackingEnabled(bBacktracking);	}
	bool IsBacktrackingEnabled() const { return QueryFilterImpl->IsBacktrackingEnabled(); }

	template<typename FilterType>
	void SetFilterType()
	{
		QueryFilterImpl = MakeShareable(new FilterType());
	}

	FORCEINLINE_DEBUGGABLE void SetFilterImplementation(const class INavigationQueryFilterInterface* InQueryFilterImpl)
	{
		QueryFilterImpl = MakeShareable(InQueryFilterImpl->CreateCopy());
	}

	FORCEINLINE const INavigationQueryFilterInterface* GetImplementation() const { return QueryFilterImpl.Get(); }
	FORCEINLINE INavigationQueryFilterInterface* GetImplementation() { return QueryFilterImpl.Get(); }
	void Reset() { GetImplementation()->Reset(); }

	TSharedPtr<FNavigationQueryFilter> GetCopy() const;

	FORCEINLINE bool operator==(const FNavigationQueryFilter& Other) const
	{
		const INavigationQueryFilterInterface* Impl0 = GetImplementation();
		const INavigationQueryFilterInterface* Impl1 = Other.GetImplementation();
		return Impl0 && Impl1 && Impl0->IsEqual(Impl1);
	}

	static const uint32 DefaultMaxSearchNodes;
	
protected:
	void Assign(const FNavigationQueryFilter& Source);

	TSharedPtr<INavigationQueryFilterInterface, ESPMode::ThreadSafe> QueryFilterImpl;
	uint32 MaxSearchNodes;
};

struct ENGINE_API FPathFindingQuery
{
	TWeakObjectPtr<const class ANavigationData> NavData;
	const FVector StartLocation;
	const FVector EndLocation;
	TSharedPtr<const FNavigationQueryFilter> QueryFilter;
	
	FPathFindingQuery()
		: NavData(NULL)
		, StartLocation(FVector::ZeroVector)
		, EndLocation(FVector::ZeroVector)
	{
	}

	FPathFindingQuery(const FPathFindingQuery& Source);

	FPathFindingQuery(const class ANavigationData* InNavData, const FVector& Start, const FVector& End, TSharedPtr<const FNavigationQueryFilter> SourceQueryFilter = NULL);
};

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

	FAsyncPathFindingQuery(const class ANavigationData* InNavData, const FVector& Start, const FVector& End, const FNavPathQueryDelegate& Delegate, TSharedPtr<const FNavigationQueryFilter> SourceQueryFilter);
	FAsyncPathFindingQuery(const FPathFindingQuery& Query, const FNavPathQueryDelegate& Delegate, const EPathFindingMode::Type QueryMode);
	
protected:
	FORCEINLINE static uint32 GetUniqueID() 
	{
		return ++LastPathFindingUniqueID;
	}

	static uint32 LastPathFindingUniqueID;
};

struct FNavigationSystemExec: public FSelfRegisteringExec
{
	FNavigationSystemExec(class UNavigationSystem* InOwner)
	{
	}

	// Begin FExec Interface
	virtual bool Exec(UWorld* Inworld, const TCHAR* Cmd, FOutputDevice& Ar) OVERRIDE;
	// End FExec Interface
};

UCLASS(Within=World, config=Engine)
class ENGINE_API UNavigationSystem : public UBlueprintFunctionLibrary
{
	GENERATED_UCLASS_BODY()

	virtual ~UNavigationSystem();

	UPROPERTY()
	ANavigationData* MainNavData;

	/** Should navigation system spawn default Navigation Data when there's none and there are navigation bounds present? */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bAutoCreateNavigationData:1;

	/** Should navigation system rebuild navigation data at game/pie runtime? 
	 *	It also includes spawning navigation data instances if there are none present. */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bBuildNavigationAtRuntime:1;

	/** if set to true will result navigation system not rebuild navigation until 
	 *	a call to ReleaseInitialBuildingLock() is called. Does not influence 
	 *	editor-time generation (i.e. does influence PIE and Game).
	 *	Defaults to false.*/
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bInitialBuildingLocked:1;

	/** If set to true (default) navigation will be generated only within special navigation 
	 *	bounds volumes (like ANavMeshBoundsVolume). Set to false means navigation should be generated
	 *	everywhere.
	 */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bWholeWorldNavigable:1;

	/** If set to true (default) generation seeds will include locations of all player controlled pawns */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bAddPlayersToGenerationSeeds:1;

	/** false by default, if set to true will result in not caring about nav agent height 
	 *	when trying to match navigation data to passed in nav agent */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	uint32 bSkipAgentHeightCheckWhenPickingNavData:1;

	UPROPERTY(config, EditAnywhere, Category=Agents)
	TArray<FNavDataConfig> SupportedAgents;
	
	/** update frequency for dirty areas on navmesh */
	UPROPERTY(config, EditAnywhere, Category=NavigationSystem)
	float DirtyAreasUpdateFreq;

	UPROPERTY()
	TArray<class ANavigationData*> NavDataSet;

	UPROPERTY(transient)
	TArray<class ANavigationData*> NavDataRegistrationQueue;

	TSet<FNavigationDirtyElement> PendingOctreeUpdates;

 	UPROPERTY(/*BlueprintAssignable, */Transient)
	FOnNavDataRegistered OnNavDataRegisteredEvent;

	//----------------------------------------------------------------------//
	// Kismet functions
	//----------------------------------------------------------------------//
	/** Project a point onto the NavigationData */
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static FVector ProjectPointToNavigation(UObject* WorldContext, const FVector& Point, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);
	
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static FVector GetRandomPoint(UObject* WorldContext, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static FVector GetRandomPointInRadius(UObject* WorldContext, const FVector& Origin, float Radius, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	/** Potentially expensive. Use with caution. Consider using UPathFollowingComponent::GetRemainingPathCost instead */
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static ENavigationQueryResult::Type GetPathCost(UObject* WorldContext, const FVector& PathStart, const FVector& PathEnd, float& PathCost, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	/** Potentially expensive. Use with caution */
	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static ENavigationQueryResult::Type GetPathLength(UObject* WorldContext, const FVector& PathStart, const FVector& PathEnd, float& PathLength, class ANavigationData* NavData = NULL, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL);

	UFUNCTION(BlueprintPure, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ) )
	static bool IsNavigationBeingBuilt(UObject* WorldContext);

	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	static void SimpleMoveToActor(AController* Controller, const AActor* Goal);

	UFUNCTION(BlueprintCallable, Category="AI|Navigation")
	static void SimpleMoveToLocation(AController* Controller, const FVector& Goal);

	/** Performs navigation raycast on NavigationData appropriate for given Querier.
	 *	@param Querier if not passed default navigation data will be used
	 *	@param HitLocation if line was obstructed this will be set to hit location. Otherwise it contains SegmentEnd
	 *	@return true if line from RayStart to RayEnd was obstructed. Also, true when no navigation data present */
	UFUNCTION(BlueprintCallable, Category="AI|Navigation", meta=(HidePin="WorldContext", DefaultToSelf="WorldContext" ))
	static bool NavigationRaycast(UObject* WorldContext, const FVector& RayStart, const FVector& RayEnd, FVector& HitLocation, TSubclassOf<class UNavigationQueryFilter> FilterClass = NULL, class AAIController* Querier = NULL);

	/** delegate type for events that dirty the navigation data ( Params: const FBox& DirtyBounds ) */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnNavigationDirty, const FBox&);
	/** called after navigation influencing event takes place*/
	static FOnNavigationDirty NavigationDirtyEvent;

	enum ERegistrationResult
	{
		RegistrationError,
		RegistrationFailed_DataPendingKill,			// means navigation data being registered is marked as pending kill
		RegistrationFailed_AgentAlreadySupported,	// this means that navigation agent supported by given nav data is already handled by some other, previously registered instance
		RegistrationFailed_AgentNotValid,			// given instance contains navmesh that doesn't support any of expected agent types, or instance doesn't specify any agent
		RegistrationSuccessful,
	};

	enum EOctreeUpdateMode
	{
		OctreeUpdate_Default = 0,						// regular update, mark dirty areas depending on exported content
		OctreeUpdate_Geometry = 1,						// full update, mark dirty areas for geometry rebuild
		OctreeUpdate_Modifiers = 2,						// quick update, mark dirty areas for modifier rebuild
		OctreeUpdate_Refresh = 4,						// update is used for refresh, don't invalidate pending queue
	};

	// Begin UObject Interface
	virtual void PostInitProperties() OVERRIDE;
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// End UObject Interface

	virtual void Tick(float DeltaSeconds);

	UWorld* GetWorld() const { return GetOuterUWorld(); }

	//----------------------------------------------------------------------//
	// Public querying interface                                                                
	//----------------------------------------------------------------------//
	/** 
	 *	Synchronously looks for a path from @fLocation to @EndLocation for agent with properties @AgentProperties. NavData actor appropriate for specified 
	 *	FNavAgentProperties will be found automatically
	 *	@param ResultPath results are put here
	 *	@param NavData optional navigation data that will be used instead of the one that would be deducted from AgentProperties
	 *  @param Mode switch between normal and hierarchical path finding algorithms
	 */
	FPathFindingResult FindPathSync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular);

	/** 
	 *	Does a simple path finding from @StartLocation to @EndLocation on specified NavData. If none passed MainNavData will be used
	 *	Result gets placed in ResultPath
	 *	@param NavData optional navigation data that will be used instead main navigation data
	 *  @param Mode switch between normal and hierarchical path finding algorithms
	 */
	FPathFindingResult FindPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular);

	/** 
	 *	Asynchronously looks for a path from @StartLocation to @EndLocation for agent with properties @AgentProperties. NavData actor appropriate for specified 
	 *	FNavAgentProperties will be found automatically
	 *	@param ResultDelegate delegate that will be called once query has been processed and finished. Will be called even if query fails - in such case see comments for delegate's params
	 *	@param NavData optional navigation data that will be used instead of the one that would be deducted from AgentProperties
	 *	@param PathToFill if points to an actual navigation path instance than this instance will be filled with resulting path. Otherwise a new instance will be created and 
	 *		used in call to ResultDelegate
	 *  @param Mode switch between normal and hierarchical path finding algorithms
	 *	@return request ID
	 */
	uint32 FindPathAsync(const FNavAgentProperties& AgentProperties, FPathFindingQuery Query, const FNavPathQueryDelegate& ResultDelegate, EPathFindingMode::Type Mode = EPathFindingMode::Regular);

	/** Removes query indicated by given ID from queue of path finding requests to process. */
	void AbortAsyncFindPathRequest(uint32 AsynPathQueryID);
	
	/** 
	 *	Synchronously check if path between two points exists
	 *  Does not return path object, but will run faster (especially in hierarchical mode)
	 *  @param Mode switch between normal and hierarchical path finding algorithms. @note Hierarchical mode ignores QueryFilter
	 *	@return true if path exists
	 */
	bool TestPathSync(FPathFindingQuery Query, EPathFindingMode::Type Mode = EPathFindingMode::Regular) const;

	/** Finds random point in navigable space
	 *	@param ResultLocation Found point is put here
	 *	@param NavData If NavData == NULL then MainNavData is used.
	 *	@return true if any location found, false otherwise */
	bool GetRandomPoint(FNavLocation& ResultLocation, class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL);

	/** Finds random point in navigable space restricted to Radius around Origin
	 *	@param ResultLocation Found point is put here
	 *	@param NavData If NavData == NULL then MainNavData is used.
	 *	@return true if any location found, false otherwise */
	bool GetRandomPointInRadius(const FVector& Origin, float Radius, FNavLocation& ResultLocation, class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** Calculates a path from PathStart to PathEnd and retrieves its cost. 
	 *	@NOTE potentially expensive, so use it with caution */
	ENavigationQueryResult::Type GetPathCost(const FVector& PathStart, const FVector& PathEnd, float& PathCost, const class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** Calculates a path from PathStart to PathEnd and retrieves its overestimated length.
	 *	@NOTE potentially expensive, so use it with caution */
	ENavigationQueryResult::Type GetPathLength(const FVector& PathStart, const FVector& PathEnd, float& PathLength, const ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** Calculates a path from PathStart to PathEnd and retrieves its overestimated length and cost.
	 *	@NOTE potentially expensive, so use it with caution */
	ENavigationQueryResult::Type GetPathLengthAndCost(const FVector& PathStart, const FVector& PathEnd, float& PathLength, float& PathCost, const ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	// @todo document
	bool ProjectPointToNavigation(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent = INVALID_NAVEXTENT, const FNavAgentProperties* AgentProperties = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL)
	{
		return ProjectPointToNavigation(Point, OutLocation, Extent, AgentProperties != NULL ? GetNavDataForProps(*AgentProperties) : GetMainNavData(NavigationSystem::DontCreate), QueryFilter);
	}

	// @todo document
	bool ProjectPointToNavigation(const FVector& Point, FNavLocation& OutLocation, const FVector& Extent = INVALID_NAVEXTENT, const class ANavigationData* NavData = NULL, TSharedPtr<const FNavigationQueryFilter> QueryFilter = NULL) const;

	/** 
	 * Looks for NavData generated for specified movement properties and returns it. NULL if not found;
	 */
	class ANavigationData* GetNavDataForProps(const FNavAgentProperties& AgentProperties);

	/** 
	 * Looks for NavData generated for specified movement properties and returns it. NULL if not found; Const version.
	 */
	const class ANavigationData* GetNavDataForProps(const FNavAgentProperties& AgentProperties) const;

	/** Returns the world nav mesh object.  Creates one if it doesn't exist. */
	class ANavigationData* GetMainNavData(NavigationSystem::ECreateIfEmpty CreateNewIfNoneFound);
	/** Returns the world nav mesh object.  Creates one if it doesn't exist. */
	const class ANavigationData* GetMainNavData() const { return MainNavData; }

	/** Super-hacky safety feature for threaded navmesh building. Will be gone once figure out why keeping TSharedPointer to Navigation Generator doesn't 
	 *	guarantee its existence */
	bool ShouldGeneratorRun(const class FNavDataGenerator* Generator) const;

	bool IsNavigationBuilt(const class AWorldSettings* Settings) const;

	bool IsThereAnywhereToBuildNavigation() const;

	bool ShouldGenerateNavigationEverywhere() const { return bWholeWorldNavigable; }

	FBox GetWorldBounds() const;
	
	FBox GetLevelBounds(ULevel* InLevel) const;

	/** @return default walkable area class */
	FORCEINLINE static TSubclassOf<class UNavArea> GetDefaultWalkableArea() { return DefaultWalkableArea; }

	/** @return default obstacle area class */
	FORCEINLINE static TSubclassOf<class UNavArea> GetDefaultObstacleArea() { return DefaultObstacleArea; }

	void ApplyWorldOffset(const FVector& InOffset, bool bWorldShift);

	/** checks if navigation/navmesh is dirty and needs to be rebuilt */
	bool IsNavigationDirty();

	//----------------------------------------------------------------------//
	// Bookkeeping 
	//----------------------------------------------------------------------//
	
	// @todo document
	void UnregisterNavData(class ANavigationData* NavData);

	/** adds NavData to registration candidates queue - NavDataRegistrationQueue */
	void RequestRegistration(class ANavigationData* NavData, bool bTriggerRegistrationProcessing = true);

	/** Processes registration of candidates queues via RequestRegistration and stored in NavDataRegistrationQueue */
	void ProcessRegistrationCandidates();

	/** registers NavArea classes awaiting registration in PendingNavAreaRegistration */
	void ProcessNavAreaPendingRegistration();

	// @todo document
	static void RegisterNavigationDataClass(const FCreateNavigationDataInstanceDelegate& ResultDelegate);

	/** @return pointer to ANavigationData instance of given ID, or NULL if it was not found. Note it looks only through registered navigation data */
	class ANavigationData* GetNavDataWithID(const uint16 NavDataID) const;

	void ReleaseInitialBuildingLock();

	//----------------------------------------------------------------------//
	// navigation octree related functions
	//----------------------------------------------------------------------//
	FSetElementId RegisterNavigationRelevantActor(class AActor* Actor, int32 UpdateFlags = OctreeUpdate_Default);
	void UnregisterNavigationRelevantActor(class AActor* Actor, int32 UpdateFlags = OctreeUpdate_Default);

#if WITH_NAVIGATION_GENERATOR
	void AddDirtyArea(const FBox& NewArea, int32 Flags);
	void AddDirtyAreas(const TArray<FBox>& NewAreas, int32 Flags);
#endif // WITH_NAVIGATION_GENERATOR

	const class FNavigationOctree* GetNavOctree() const { return NavOctree; }

	void UpdateNavOctree(class AActor* Actor, int32 UpdateFlags = OctreeUpdate_Default);
	void UpdateNavOctree(class UActorComponent* ActorComp, int32 UpdateFlags = OctreeUpdate_Default);
	void UpdateNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds);

	/** called to gather navigation relevant actors that have been created while
	 *	Navigation System was not present */
	void PopulateNavOctree();

	FORCEINLINE void SetObjectsNavOctreeId(UObject* Object, FOctreeElementId Id) { ObjectToOctreeId.Add(Object, Id); }
	FORCEINLINE const FOctreeElementId* GetObjectsNavOctreeId(const UObject* Object) const { return ObjectToOctreeId.Find(Object); }
	FORCEINLINE	void RemoveObjectsNavOctreeId(UObject* Object) { ObjectToOctreeId.Remove(Object); }

	/** find all elements in navigation octree within given box (intersection) */
	void FindElementsInNavOctree(const FBox& QueryBox, const struct FNavigationOctreeFilter& Filter, TArray<struct FNavigationOctreeElement>& Elements);

	//----------------------------------------------------------------------//
	// Smart links
	//----------------------------------------------------------------------//
	void RegisterSmartLink(class USmartNavLinkComponent* LinkComp);
	void UnregisterSmartLink(class USmartNavLinkComponent* LinkComp);
	
	/** find first available smart link ID */
	uint32 FindFreeSmartLinkId() const;

	/** find smart link by link ID */
	class USmartNavLinkComponent* GetSmartLink(uint32 SmartLinkId) const;

	/** updates smart link for all active navigation data instances */
	void UpdateSmartLink(class USmartNavLinkComponent* LinkComp);

	//----------------------------------------------------------------------//
	// Areas
	//----------------------------------------------------------------------//
	static void RequestAreaRegistering(UClass* NavAreaClass);
	static void RequestAreaUnregistering(UClass* NavAreaClass);

	/** find index in SupportedAgents array for given navigation data */
	int32 GetSupportedAgentIndex(const ANavigationData* NavData) const;

	/** find index in SupportedAgents array for agent type */
	int32 GetSupportedAgentIndex(const FNavAgentProperties& NavAgent) const;

	//----------------------------------------------------------------------//
	// building
	//----------------------------------------------------------------------//
#if WITH_NAVIGATION_GENERATOR
	/** Triggers navigation building on all eligible navigation data. */
	virtual void Build();

	// @todo document
	void OnUpdateStreamingStarted();
	// @todo document
	void OnUpdateStreamingFinished();
	// @todo document
	void OnPIEStart();
	// @todo document
	void OnPIEEnd();
	
	// @todo document
	FORCEINLINE bool IsNavigationBuildingLocked() const { return bNavigationBuildingLocked || bInitialBuildingLockActive; }

	// @todo document
	void OnNavigationBoundsUpdated(class AVolume* NavVolume);

	/** Used to display "navigation building in progress" notify */
	bool IsNavigationBuildInProgress(bool bCheckDirtyToo = true);
#endif // WITH_NAVIGATION_GENERATOR

	/** Called upon UWorld destruction to release what needs to be released */
	void CleanUp();

	/** 
	 *	Called when owner-UWorld begins play. 
	 */
	virtual void OnBeginPlay();

	/** */
	virtual void OnWorldInitDone(NavigationSystem::EMode Mode);

#if WITH_EDITOR
	/** allow editor to toggle whether seamless navigation building is enabled */
	static void SetNavigationAutoUpdateEnabled(bool bNewEnable,UNavigationSystem* InNavigationsystem);

	/** check whether seamless navigation building is enabled*/
	FORCEINLINE static bool GetIsNavigationAutoUpdateEnabled() { return bNavigationAutoUpdateEnabled; }

	bool AreFakeComponentChangesBeingApplied() { return bFakeComponentChangesBeingApplied; }
	void BeginFakeComponentChanges() { bFakeComponentChangesBeingApplied = true; }
	void EndFakeComponentChanges() { bFakeComponentChangesBeingApplied = false; }

	void UpdateLevelCollision(ULevel* InLevel);
#endif // WITH_EDITOR

	/** register actor important for generation (navigation data will be build around them first) */
	void RegisterGenerationSeed(AActor* SeedActor);
	void UnregisterGenerationSeed(AActor* SeedActor);
	void GetGenerationSeeds(TArray<FVector>& SeedLocations) const;

	static UNavigationSystem* CreateNavigationSystem(class UWorld* WorldOwner);

	static UNavigationSystem* GetCurrent(class UWorld* World);
	static UNavigationSystem* GetCurrent(class UObject* WorldContextObject);

	// Fetch the array of all nav-agent properties.
	void GetNavAgentPropertiesArray(TArray<FNavAgentProperties>& OutNavAgentProperties) const;

	/** 
	 * Exec command handlers
	 */
	bool HandleCycleNavDrawnCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	bool HandleCountNavMemCommand( const TCHAR* Cmd, FOutputDevice& Ar );
	
	//----------------------------------------------------------------------//
	// debug
	//----------------------------------------------------------------------//
	void CycleNavigationDataDrawn();

protected:
#if WITH_EDITOR
	bool bFakeComponentChangesBeingApplied;
#endif

	NavigationSystem::EMode OperationMode;

	class FNavigationOctree* NavOctree;

	TArray<FAsyncPathFindingQuery> AsyncPathFindingQueries;

	FCriticalSection NavDataRegistration;

	TMap<FNavAgentProperties, ANavigationData*> AgentToNavDataMap;
	
	TMap<const UObject*, FOctreeElementId> ObjectToOctreeId;

	/** Map of all custom navigation links, that are relevant for path following */
	TMap<uint32, class USmartNavLinkComponent*> SmartLinksMap;

	/** List of actors relevant to generation of navigation data */
	TArray<TWeakObjectPtr<AActor> > GenerationSeeds;

#if WITH_NAVIGATION_GENERATOR
	/** stores areas marked as dirty throughout the frame, processes them 
	 *	once a frame in Tick function */
	TArray<FNavigationDirtyArea> DirtyAreas;
#endif // WITH_NAVIGATION_GENERATOR	

	// async queries
	FCriticalSection NavDataRegistrationSection;
	
	uint32 bNavigationBuildingLocked:1;
	uint32 bInitialBuildingLockActive:1;

	/** cached navigable world bounding box*/
	mutable FBox NavigableWorldBounds;

	int32 CurrentlyDrawnNavDataIndex;

	/** temporary cumulative time to calculate when we need to update dirty areas */
	float DirtyAreasUpdateTime;

	/** collection of delegates to create all available navigation data types */
	static TArray<FCreateNavigationDataInstanceDelegate>* NavigationDataCreators;

	/** whether seamless navigation building is enabled */
	static bool bNavigationAutoUpdateEnabled;

	static TArray<UClass*> PendingNavAreaRegistration;
	static TArray<const UClass*> NavAreaClasses;
	static TSubclassOf<class UNavArea> DefaultWalkableArea;
	static TSubclassOf<class UNavArea> DefaultObstacleArea;

	static FOnNavAreaChanged NavAreaAddedObservers;
	static FOnNavAreaChanged NavAreaRemovedObservers;
	 
	/** delegate handler for PostLoadMap event */
	void OnPostLoadMap();
	/** delegate handler for ActorMoved events */
	void OnActorMoved(AActor* Actor);
	/** delegate handler called when navigation is dirtied*/
	void OnNavigationDirtied(const FBox& Bounds);
	
	/** Registers given navigation data with this Navigation System.
	 *	@return RegistrationSuccessful if registration was successful, other results mean it failed
	 *	@see ERegistrationResult
	 */
	ERegistrationResult RegisterNavData(class ANavigationData* NavData);

	/** tries to register navigation area */
	void RegisterNavAreaClass(UClass* NavAreaClass);
	
 	FSetElementId RegisterNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds);
	void UnregisterNavOctreeElement(class UObject* ElementOwner, int32 UpdateFlags, const FBox& Bounds);
	
	/** read element data from navigation octree */
	bool GetNavOctreeElementData(class UObject* NodeOwner, int32& DirtyFlags, FBox& DirtyBounds);

	/** Adds given element to NavOctree. No check for owner's validity are performed, 
	 *	nor its presence in NavOctree - function assumes callee responsibility 
	 *	in this regard **/
	void AddElementToNavOctree(const FNavigationDirtyElement& DirtyElement);

	void AddActorElementToNavOctree(class AActor* Actor, const FNavigationDirtyElement& DirtyElement);
	void AddComponentElementToNavOctree(class UActorComponent* ActorComp, const FNavigationDirtyElement& DirtyElement, const FBox& Bounds);

#if WITH_NAVIGATION_GENERATOR

	/** Add BSP collision data to navigation octree */
	void AddLevelCollisionToOctree(ULevel* Level);
	
	/** Remove BSP collision data from navigation octree */
	void RemoveLevelCollisionFromOctree(ULevel* Level);

	/** registers or unregisters all generators from rebuilding callbacks */
	void EnableAllGenerators(bool bEnable, bool bForce = false);
 
private:
	// @todo document
	void NavigationBuildingLock();
	// @todo document
	void NavigationBuildingUnlock(bool bForce = false);

	/** Triggers navigation building on all eligible navigation data. */
	void SpawnNavigationData();
	void RebuildAll();
		 
	/** Handler for FWorldDelegates::LevelAddedToWorld event */
	 void OnLevelAddedToWorld(ULevel* InLevel, UWorld* InWorld);
	 
	/** Handler for FWorldDelegates::LevelRemovedFromWorld event */
	void OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld);
#endif // WITH_NAVIGATION_GENERATOR

	/** Adds given request to requests queue. Note it's to be called only on game thread only */
	void AddAsyncQuery(const FAsyncPathFindingQuery& Query);
		 
	/** spawns a non-game-thread task to process requests given in PathFindingQueries.
	 *	In the process PathFindingQueries gets copied. */
	void TriggerAsyncQueries(TArray<FAsyncPathFindingQuery>& PathFindingQueries);

	/** Processes pathfinding requests given in PathFindingQueries.*/
	void PerformAsyncQueries(TArray<FAsyncPathFindingQuery> PathFindingQueries);
};

struct FNavigationTypeCreator
{
	FCreateNavigationDataInstanceDelegate CreatorDelegate;

	FNavigationTypeCreator(const FCreateNavigationDataInstanceDelegate& InCreatorDelegate)
	: CreatorDelegate(InCreatorDelegate)
	{
		UNavigationSystem::RegisterNavigationDataClass(CreatorDelegate);
	}

	class ANavigationData* Create(class UNavigationSystem* NavSys)
	{
		return CreatorDelegate.IsBound() ? CreatorDelegate.Execute(NavSys) : NULL;
	}
};
