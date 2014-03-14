// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 *	This file is for shared structs and enums that need to be declared before the rest of Engine.
 *  The typical use case is for structs used in the renderer and also in script code.
 */

#include "EngineBaseTypes.generated.h"

//
//	EInputEvent
//
UENUM()
enum EInputEvent
{
	IE_Pressed              =0,
	IE_Released             =1,
	IE_Repeat               =2,
	IE_DoubleClick          =3,
	IE_Axis                 =4,
	IE_MAX                  =5,
};

/**
 * Determines which ticking group an Actor/Component belongs to
 */
UENUM()
enum ETickingGroup
{
	/**
	 * Any item that needs to be updated before asynchronous work is done
	 */
	TG_PrePhysics,
	/**
	 * Temp while we transition away from tick groups, this will only have one task, the one that starts physics
	 */
#if EXPERIMENTAL_PARALLEL_CODE
	/**
	 * Add a specific group for parallel animation work
	 */
	TG_ParallelAnimWork,
	/**
	 * Add a specific group for parallel post animation work
	 */
	TG_ParallelPostAnimWork,
#endif								
	TG_StartPhysics,
	/**
	 * Any item that can be run in parallel of our async work
	 */
	TG_DuringPhysics,
	/**
	 * Temp while we transition away from tick groups, this will only have one task, the one is not completed until physics is done
	 */
	TG_EndPhysics,
	/**
	 * Any item that needs the async work to be done before being updated
	 */
	TG_PostPhysics,
	/**
	 * Any item that needs the update work to be done before being ticked
	 */
	TG_PostUpdateWork,
	/**
	 * Special tick group that is not actually a tick group. After every tick group this is repeatedly re-run until there are no more newly spawned items to run
	 */
	TG_NewlySpawned,
	TG_MAX,
};

/**
 * This is small structure to hold prerequisite tick functions
 */
USTRUCT(transient)
struct FTickPrerequisite
{
	GENERATED_USTRUCT_BODY()

	/** Tick functions live inside of UObjects, so we need a separate weak pointer to the UObject solely for the purpose of determining if PrerequisiteTickFunction is still valid **/
	TWeakObjectPtr<class UObject> PrerequisiteObject;



		/** Pointer to the actual tick function and must be completed prior to our tick running **/
		struct FTickFunction*		PrerequisiteTickFunction;

		/** Noop constructor **/
		FTickPrerequisite()
		: PrerequisiteTickFunction(NULL)
		{
		}
		/** 
		 * Constructor
		 * @param TargetObject - UObject containing this tick function. Only used to verify that the other pointer is still usable
		 * @param TargetTickFunction - Actual tick function to use as a prerequisite
		**/
		FTickPrerequisite(UObject* TargetObject, struct FTickFunction& TargetTickFunction)
		: PrerequisiteObject(TargetObject)
		, PrerequisiteTickFunction(&TargetTickFunction)
		{
			check(PrerequisiteTickFunction);
		}
		/** Equality operator, used to prevent duplicates and allow removal by value **/
		bool operator==(const FTickPrerequisite& Other) const
		{
			return PrerequisiteObject == Other.PrerequisiteObject &&
				PrerequisiteTickFunction == Other.PrerequisiteTickFunction;
		}
		/** Return the tick function, if it is still valid. Can be null if the tick function was null or the containing UObject has been garbage collected. **/
		struct FTickFunction* Get()
		{
			if (PrerequisiteObject.IsValid(true))
			{
				return PrerequisiteTickFunction;
			}
			return NULL;
		}
	
};

/** 
* Abstract Base class for all tick functions.
**/
USTRUCT()
struct ENGINE_API FTickFunction
{
	GENERATED_USTRUCT_BODY()

public:
	// The following UPROPERTYs are for configuration and inherited from the CDO/archetype/blueprint etc

	/** Defines the minimum tick group for this tick function; given prerequisites, it can be delayed. **/
	UPROPERTY()
	TEnumAsByte<enum ETickingGroup> TickGroup;

	/** Bool indicating that this function should execute even if the game is paused. Pause ticks are very limited in capabilities. **/
	UPROPERTY()
	uint32 bTickEvenWhenPaused:1;

	/** If false, this tick function will never be registered and will never tick. Only settable in defaults. */
	UPROPERTY()
	uint32 bCanEverTick:1;

	/** If true, this tick function will start enabled, but can be disabled later on. */
	UPROPERTY()
	uint32 bStartWithTickEnabled:1;

	/** If we allow this tick to run on a dedicated server */
	UPROPERTY()
	uint32 bAllowTickOnDedicatedServer:1;

	/** If false, this tick will run on the game thread, otherwise it will run on any thread in parallel with the game thread and in parallel with other "async ticks" **/
	uint32 bRunOnAnyThread:1;

private:
	/** If true, means that this tick function is in the master array of tick functions **/
	uint32 bRegistered:1;

	/** 
	 * If false, this tick will not fire, nor will any other tick that has this tick function as an EnableParent 
	 * CAUTION: Do not set this directly
	 **/
	uint32 bTickEnabled:1;

	/** Internal data to track if we have started visiting this tick function yet this frame **/
	int32 TickVisitedGFrameCounter;

	/** Internal data to track if we have finshed visiting this tick function yet this frame **/
	int32 TickQueuedGFrameCounter;

protected:
	/** Internal data that indicates the tick group we actually executed in (it may have been delayed due to prerequisites) **/
	TEnumAsByte<enum ETickingGroup> ActualTickGroup;

private:
	/** Completion handle for the task that will run this tick. Caution, this is no reset to NULL until an unspecified future time **/
	FGraphEventRef CompletionHandle;

	/** Prerequisites for this tick function **/
	TArray<struct FTickPrerequisite> Prerequisites;

public:
	/** 
	 * If the "EnableParent" is not enabled, then this tick function is implicitly disabled as well. 
	 * Caution, there is no protection on this strong pointer. It is assumed your enable parent will not be destroyed before you are.
	 **/
	FTickFunction*								EnableParent;
	/** Back pointer to the FTickTaskLevel containing this tick function if it is registered **/
	class FTickTaskLevel*						TickTaskLevel;

	/** Default constructor, intitalizes to reasonable defaults **/
	FTickFunction();
	/** Destructor, unregisters the tick function **/
	virtual ~FTickFunction();

	/** 
	 * Adds the tick function to the master list of tick functions. 
	 * @param Level - level to place this tick function in
	 **/
	void RegisterTickFunction(class ULevel* Level);
	/** Removes the tick function from the master list of tick functions. **/
	void UnRegisterTickFunction();
	/** See if the tick function is currently registered */
	bool IsTickFunctionRegistered() const { return bRegistered; }

	/** Enables or disables this tick function. **/
	void SetTickFunctionEnable(bool bInEnabled);
	/** Returns whether the tick function is currently enabled */
	bool IsTickFunctionEnabled() const { return bTickEnabled; }

	/**
	* Gets the current completion handle of this tick function, so it can be delayed until a later point when some additional
	* tasks have been completed.  Only valid after TG_PreAsyncWork has started and then only until the TickFunction itself has
	* started to run
	**/
	FGraphEventRef GetCompletionHandle() const
	{
		return CompletionHandle;
	}
	/** 
	* Gets the action tick group that this function will execute in this frame.
	* Only valid after TG_PreAsyncWork has started through the end of the frame.
	**/
	TEnumAsByte<enum ETickingGroup> GetActualTickGroup() const
	{
		return ActualTickGroup;
	}

	/** 
	 * Adds a tick function to the list of prerequisites...in other words, adds the requirement that TargetTickFunction is called before this tick function is 
	 * @param TargetObject - UObject containing this tick function. Only used to verify that the other pointer is still usable
	 * @param TargetTickFunction - Actual tick function to use as a prerequisite
	 **/
	void AddPrerequisite(UObject* TargetObject, struct FTickFunction& TargetTickFunction);
	/** 
	 * Removes a prerequisite that was previously added.
	 * @param TargetObject - UObject containing this tick function. Only used to verify that the other pointer is still usable
	 * @param TargetTickFunction - Actual tick function to use as a prerequisite
	 **/
	void RemovePrerequisite(UObject* TargetObject, struct FTickFunction& TargetTickFunction);

	/**
	 * @return a reference to prerequisites for this tick function.
	 */
	TArray<struct FTickPrerequisite>& GetPrerequisites()
	{
		return Prerequisites;
	}

	/**
	 * @return a reference to prerequisites for this tick function (const).
	 */
	const TArray<struct FTickPrerequisite>& GetPrerequisites() const
	{
		return Prerequisites;
	}

private:
	/**
	 * Queues a tick function for execution from the game thread
	 * @param TickContext - context to tick in
	 */
	void QueueTickFunction(const struct FTickContext& TickContext);

	/**
	 * Queues a tick function for execution, assuming parallel queuing
	 * @param TickContext - context to tick in
	 * @param StackForCycleDetection - stack to detect cycles
	*/
	void QueueTickFunctionParallel(const FTickContext& TickContext, TArray<FTickFunction*, TInlineAllocator<4> >& StackForCycleDetection);

	/** 
	 * Abstract function actually execute the tick. 
	 * @param DeltaTime - frame time to advance, in seconds
	 * @param TickType - kind of tick for this frame
	 * @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
	 * @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completetion of this task until certain child tasks are complete.
	 **/
	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		check(0); // you cannot make this pure virtual in script because it wants to create constructors.
	}
	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage()
	{
		check(0); // you cannot make this pure virtual in script because it wants to create constructors.
		return FString(TEXT("invalid"));
	}
	
	friend class FTickTaskSequencer;
	friend class FTickTaskManager;
	friend class FTickTaskLevel;
};

/** 
* Tick function that calls AActor::TickActor
**/
USTRUCT(transient)
struct FActorTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	/**  AActor  that is the target of this tick **/
	class AActor*	Target;

	/** 
		* Abstract function actually execute the tick. 
		* @param DeltaTime - frame time to advance, in seconds
		* @param TickType - kind of tick for this frame
		* @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
		* @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completetion of this task until certain child tasks are complete.
	**/
	ENGINE_API virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) OVERRIDE;
	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	ENGINE_API virtual FString DiagnosticMessage();
};

/** 
* Tick function that calls UActorComponent::ConditionalTick
**/
USTRUCT(transient)
struct FActorComponentTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	/**  AActor  component that is the target of this tick **/
	class UActorComponent*	Target;

	/** 
		* Abstract function actually execute the tick. 
		* @param DeltaTime - frame time to advance, in seconds
		* @param TickType - kind of tick for this frame
		* @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
		* @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completetion of this task until certain child tasks are complete.
	**/
	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) OVERRIDE;
	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage();
};

/** 
* Tick function that calls UPrimitiveComponent::PostPhysicsTick
**/
USTRUCT(transient)
struct FPrimitiveComponentPostPhysicsTickFunction : public FTickFunction
{
	GENERATED_USTRUCT_BODY()

	/** PrimitiveComponent component that is the target of this tick **/
	class UPrimitiveComponent*	Target;

	/** 
		* Abstract function actually execute the tick. 
		* @param DeltaTime - frame time to advance, in seconds
		* @param TickType - kind of tick for this frame
		* @param CurrentThread - thread we are executing on, useful to pass along as new tasks are created
		* @param MyCompletionGraphEvent - completion event for this task. Useful for holding the completetion of this task until certain child tasks are complete.
	**/
	virtual void ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent) OVERRIDE;
	/** Abstract function to describe this tick. Used to print messages about illegal cycles in the dependency graph **/
	virtual FString DiagnosticMessage();
};

// Traveling from server to server.
UENUM()
enum ETravelType
{
	// Absolute URL.
	TRAVEL_Absolute,
	// Partial (carry name, reset server).
	TRAVEL_Partial,
	// Relative URL.
	TRAVEL_Relative,
	TRAVEL_MAX,
};

//URL structure.
USTRUCT(transient)
struct ENGINE_API FURL
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString Protocol;    // Protocol, i.e. "unreal" or "http".

	UPROPERTY()
	FString Host;    // Optional hostname, i.e. "204.157.115.40" or "unreal.epicgames.com", blank if local.

	UPROPERTY()
	int32 Port;    // Optional host port.

	UPROPERTY()
	FString Map;    // Map name, i.e. "SkyCity", default is "Entry".

	UPROPERTY()
	TArray<FString> Op;    // Options.

	UPROPERTY()
	FString Portal;    // Portal to enter through, default is "".

	UPROPERTY()
	int32 Valid;

	// Statics.
	static FUrlConfig UrlConfig;
	static bool bDefaultsInitialized;

	// Constructors.
	/* FURL() prevent default from being generated */
	explicit FURL( ENoInit ) { }
	FURL( const TCHAR* Filename=NULL );
	FURL( FURL* Base, const TCHAR* TextURL, ETravelType Type );
	static void StaticInit();
	static void StaticExit();

	/**
	 * Static: Removes any special URL characters from the specified string
	 *
	 * @param Str String to be filtered
	 */
	static void FilterURLString( FString& Str );

	// Functions.
	bool IsInternal() const;
	bool IsLocalInternal() const;
	bool HasOption( const TCHAR* Test ) const;
	const TCHAR* GetOption( const TCHAR* Match, const TCHAR* Default ) const;
	void LoadURLConfig( const TCHAR* Section, const FString& Filename=GGameIni );
	void SaveURLConfig( const TCHAR* Section, const TCHAR* Item, const FString& Filename=GGameIni ) const;
	void AddOption( const TCHAR* Str );
	void RemoveOption( const TCHAR* Key, const TCHAR* Section = NULL, const FString& Filename = GGameIni);
	FString ToString( bool FullyQualified=0 ) const;
	ENGINE_API friend FArchive& operator<<( FArchive& Ar, FURL& U );

	// Operators.
	bool operator==( const FURL& Other ) const;
};

enum ENetMode
{
	NM_Standalone,
	NM_DedicatedServer,
	NM_ListenServer,
	NM_Client,	// note that everything below this value is a kind of server
	NM_MAX,
};

/**
 * Define view modes to get specific show flag settings (some on, some off and some are not altered)
 * Don't change the order, the ID is serialized with the editor
 */
UENUM()
enum EViewModeIndex 
{
	// Wireframe w/ brushes
	VMI_BrushWireframe = 0,
	// Wireframe w/ BSP
	VMI_Wireframe = 1,
	// Unlit
	VMI_Unlit = 2,
	// Lit
	VMI_Lit = 3,
	VMI_Lit_DetailLighting = 4,
	// Lit wo/ materials
	VMI_LightingOnly = 5,
	// Colored according to light count.
	VMI_LightComplexity = 6,
	// Colored according to shader complexity.
	VMI_ShaderComplexity = 8,
	// Colored according to world-space LightMap texture density.
	VMI_LightmapDensity = 9,
	// Colored according to light count - showing lightmap texel density on texture mapped objects.
	VMI_LitLightmapDensity = 10,
	VMI_ReflectionOverride = 11,
	VMI_VisualizeBuffer = 12,
//	VMI_VoxelLighting = 13,
	// Colored according to stationary light overlap.
	VMI_StationaryLightOverlap = 14,
//	VMI_VertexDensity = 15,
	VMI_CollisionPawn = 15, 
	VMI_CollisionVisibility = 16, 
	VMI_Max,

	VMI_Unknown = 255,
};


/** Settings to allow designers to override the automatic expose */
USTRUCT()
struct FExposureSettings
{
	GENERATED_USTRUCT_BODY()

	FExposureSettings()
		: LogOffset(0), bFixed(false)
	{
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("%d,%d"), LogOffset, bFixed ? 1 : 0);
	}

	void SetFromString(const TCHAR *In)
	{
		// set to default
		*this = FExposureSettings();

		const TCHAR* Comma = FCString::Strchr(In, TEXT(','));
		check(Comma);

		const int32 BUFFER_SIZE = 128;
		TCHAR Buffer[BUFFER_SIZE];
		check((Comma-In)+1 < BUFFER_SIZE);
		
		FCString::Strncpy(Buffer, In, (Comma-In)+1);
		LogOffset = FCString::Atoi(Buffer);
		bFixed = !!FCString::Atoi(Comma+1);
	}

	// usually -4:/16 darker .. +4:16x brighter
	UPROPERTY()
	int32 LogOffset;
	// true: fixed exposure using the LogOffset value, false: automatic eye adaptation
	UPROPERTY()
	bool bFixed;
};


UCLASS(HeaderGroup=Base, abstract, config=Engine)
class UEngineBaseTypes : public UObject
{
	GENERATED_UCLASS_BODY()

};

