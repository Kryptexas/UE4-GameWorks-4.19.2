// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/*----------------------------------------------------------------------------
	Low level includes.
----------------------------------------------------------------------------*/

#include "Platform.h"							// Set up base types, etc for the platform
#include "Build.h"								// Build options.
#include "ProfilingDebugging/UMemoryDefines.h"	// Memory build options.
#include "CoreMiscDefines.h"					// Misc defines and enumerations
class FString;									// @todo Figure out better include order in Core.h
inline uint32 GetTypeHash (const int64 A);		// @todo Figure out better include order in Core.h
class FDefaultAllocator;						// @todo Figure out better include order in Core.h
template<typename T,typename Allocator = FDefaultAllocator> class TArray; // @todo Figure out better include order in Core.h
#include "Timespan.h"							// Time span definition
#include "DateTime.h"							// Date and time handling
#include "PlatformIncludes.h"					// Include the main and misc platform headers
#include "PlatformFileManager.h"				// Platform file manager.
#include "AssertionMacros.h"					// Various assertion macros
#include "UObject/UnrealNames.h"				// EName definition.
#include "OutputDevice.h"						// Output devices, logf, debugf, etc
#include "NumericLimits.h"						// Numeric limits
#include "UnrealMathUtility.h"					// FMath
#include "UnrealTypeTraits.h"					// Type Traits
#include "UnrealTemplate.h"						// Common template definitions.
#include "MemoryOps.h"							// Functions for efficient handling of object arrays.


/*----------------------------------------------------------------------------
	Forward declarations.
----------------------------------------------------------------------------*/

// Container forward declarations
#include "Containers/ContainerAllocationPolicies.h"

template<typename ElementType,typename Allocator = FDefaultAllocator> class TBaseArray;
template<typename T> class TTransArray;
template<typename KeyType,typename ValueType,bool bInAllowDuplicateKeys> struct TDefaultMapKeyFuncs;
template<typename KeyType,typename ValueType,typename SetAllocator = FDefaultSetAllocator, typename KeyFuncs = TDefaultMapKeyFuncs<KeyType,ValueType,false> > class TMap;
template<typename KeyType,typename ValueType,typename SetAllocator = FDefaultSetAllocator, typename KeyFuncs = TDefaultMapKeyFuncs<KeyType,ValueType,true > > class TMultiMap;


// Objects
class UObjectBase;
class UObjectBaseUtility;
class	UObject;
class		UField;
class			UEnum;
class			UProperty;
class				UByteProperty;
class				UUInt16Property;
class				UUInt32Property;
class				UUInt64Property;
class				UInt8Property;
class				UInt16Property;
class				UIntProperty;
class				UInt64Property;
class				UBoolProperty;
class				UFloatProperty;
class				UDoubleProperty;
class				UObjectPropertyBase;
class				UObjectProperty;
class					UClassProperty;
class					UInterfaceProperty;
class					UWeakObjectProperty;
class					ULazyObjectProperty;
class					UAssetObjectProperty;
class				UNameProperty;
class				UStructProperty;
class               UStrProperty;
class               UTextProperty;
class               UArrayProperty;
class				UDelegateProperty;
class				UMulticastDelegateProperty;
class			UStruct;
class				UFunction;
class				UClass;
class				UScriptStruct;
class		ULinker;
class			ULinkerLoad;
class			ULinkerSave;
class		UPackage;
class		USystem;
class		UTextBuffer;
class		UPackageMap;
class		UObjectRedirector;


// Structures
class FName;
class FArchive;
class FCompactIndex;
class FExec;
class FGuid;
class FMemStack;
class FPackageInfo;
class ITransaction;
class FUnknown;
class FRepLink;
class FString;
class FText;
class FMalloc;
class FString;
class FConfigCacheIni;
class FConfigFile;
class IFileManager;
class FOutputDeviceRedirector;
class FOutputDeviceConsole;

// Core globals
extern CORE_API FConfigCacheIni* GConfig;
extern CORE_API ITransaction* GUndo;
extern CORE_API FOutputDeviceConsole* GLogConsole;

extern CORE_API TCHAR GErrorHist[16384];
extern CORE_API TCHAR GErrorExceptionDescription[1024];

extern CORE_API const FText GTrue, GFalse, GYes, GNo, GNone;

/** If true, this executable is able to run all games (which are loaded as DLL's). */
extern CORE_API bool GIsGameAgnosticExe;	

/** When saving out of the game, this override allows the game to load editor only properties. */
extern CORE_API bool GForceLoadEditorOnly;

/** Disable loading of objects not contained within script files; used during script compilation */
extern CORE_API bool GVerifyObjectReferencesOnly;

/** when constructing objects, use the fast path on consoles... */
extern CORE_API bool GFastPathUniqueNameGeneration;

/** allow AActor object to execute script in the editor from specific entry points, such as when running a construction script */
extern CORE_API bool GAllowActorScriptExecutionInEditor;

/** Forces use of template names for newly instanced components in a CDO. */
extern CORE_API bool GCompilingBlueprint;

/** Force blueprints to not compile on load */
extern CORE_API bool GForceDisableBlueprintCompileOnLoad;


/** Helper function to flush resource streaming. */
extern CORE_API void (*GFlushStreamingFunc)(void);

extern CORE_API float GVolumeMultiplier;


#if WITH_EDITORONLY_DATA

	/** 
	 *	True if we are in the editor. 
	 *	Note that this is still true when using Play In Editor. You may want to use GWorld->HasBegunPlay in that case.
	 */
	extern CORE_API bool GIsEditor;
	extern CORE_API bool GIsImportingT3D;
	extern CORE_API bool PRIVATE_GIsRunningCommandlet;
	extern CORE_API bool GIsUCCMakeStandaloneHeaderGenerator;
	extern CORE_API bool GIsTransacting;

	/** Indicates that the game thread is currently paused deep in a call stack,
		while some subset of the editor user interface is pumped.  No game
		thread work can be done until the UI pumping loop returns naturally. */
	extern CORE_API bool			GIntraFrameDebuggingGameThread;
	/** True if this is the first time through the UI message pumping loop. */
	extern CORE_API bool			GFirstFrameIntraFrameDebugging;

#else

	#define GIsEditor								false
	#define PRIVATE_GIsRunningCommandlet			false
	#define GIsUCCMakeStandaloneHeaderGenerator		false
	#define GIntraFrameDebuggingGameThread			false
	#define GFirstFrameIntraFrameDebugging			false

#endif // WITH_EDITORONLY_DATA


/**
 * Check to see if this executable is running a commandlet (custom command-line processing code in an editor-like environment)
 */
FORCEINLINE bool IsRunningCommandlet()
{
#if WITH_EDITORONLY_DATA
	return PRIVATE_GIsRunningCommandlet;
#else
	return false;
#endif
}

extern CORE_API bool GEdSelectionLock;
extern CORE_API bool GIsClient;
extern CORE_API bool GIsServer;
extern CORE_API bool GIsCriticalError;
extern CORE_API bool GIsRunning;
extern CORE_API bool GIsGarbageCollecting;
extern CORE_API bool GIsDuplicatingClassForReinstancing;

/**
 * These are set when the engine first starts up.
 */

/**
 * This specifies whether the engine was launched as a build machine process.
 */
extern CORE_API bool GIsBuildMachine;

/**
 * This determines if we should output any log text.  If Yes then no log text should be emitted.
 */
extern CORE_API bool GIsSilent;

extern CORE_API bool GIsSlowTask;
extern CORE_API bool GSlowTaskOccurred;
extern CORE_API bool GIsGuarded;
extern CORE_API bool GIsRequestingExit;

/** Archive for serializing arbitrary data to and from memory						*/
extern CORE_API class FReloadObjectArc* GMemoryArchive;
extern CORE_API bool GIsBenchmarking;

/**
 *	Global value indicating on-screen warnings/message should be displayed.
 *	Disabled via console command "DISABLEALLSCREENMESSAGES"
 *	Enabled via console command "ENABLEALLSCREENMESSAGES"
 *	Toggled via console command "TOGGLEALLSCREENMESSAGES"
 */
extern CORE_API bool GAreScreenMessagesEnabled;
extern CORE_API bool GScreenMessagesRestoreState;

/* Whether we are dumping screen shots */
extern CORE_API int32 GIsDumpingMovie;
extern CORE_API bool GIsHighResScreenshot;
extern CORE_API uint32 GScreenshotResolutionX;
extern CORE_API uint32 GScreenshotResolutionY;
extern CORE_API uint64 GMakeCacheIDIndex;
extern CORE_API FString GEditorKeyBindingsIni;
extern CORE_API FString GEngineIni;
extern CORE_API FString GEditorIni;
extern CORE_API FString GEditorUserSettingsIni;
extern CORE_API FString GEditorGameAgnosticIni;
extern CORE_API FString GCompatIni;
extern CORE_API FString GLightmassIni;
extern CORE_API FString GScalabilityIni;
extern CORE_API FString GInputIni;
extern CORE_API FString GGameIni;
extern CORE_API FString GGameUserSettingsIni;

extern CORE_API float GNearClippingPlane;

/** Timestep if a fixed delta time is wanted. */
extern CORE_API double GFixedDeltaTime;

/** Current delta time in seconds. */
extern CORE_API double GDeltaTime;

/** Unclamped delta time in seconds. */
extern CORE_API double GUnclampedDeltaTime;
extern CORE_API double GCurrentTime;
extern CORE_API double GLastTime;

extern CORE_API bool GExitPurge;
extern CORE_API TCHAR GGameName[64];

/** Exec handler for game debugging tool, allowing commands like "editactor" */
extern CORE_API FExec* GDebugToolExec;

/** Whether we're currently in the async loading code path or not */
extern CORE_API bool GIsAsyncLoading;

/** Whether the editor is currently loading a package or not */
extern CORE_API bool GIsEditorLoadingPackage;

/** Whether GWorld points to the play in editor world */
extern CORE_API bool GIsPlayInEditorWorld;

extern CORE_API int32 GPlayInEditorID;

/** Whether or not PIE was attempting to play from PlayerStart */
extern CORE_API bool GIsPIEUsingPlayerStart;

/** Proxy class that allows verification on FApp::IsGame() accesses. */
extern CORE_API bool IsInGameThread();

/** true if the runtime needs textures to be powers of two */
extern CORE_API bool GPlatformNeedsPowerOfTwoTextures;

/** Time at which FPlatformTime::Seconds() was first initialized (very early on) */
extern CORE_API double GStartTime;

/** System time at engine init. */
extern CORE_API FString GSystemStartTime;

/** Whether we are still in the initial loading process. */
extern CORE_API bool GIsInitialLoad;

/** true when we are routing ConditionalPostLoad/PostLoad to objects */
extern CORE_API bool GIsRoutingPostLoad;

/** Steadily increasing frame counter. */
extern CORE_API uint64 GFrameCounter;

/** Incremented once per frame before the scene is being rendered. In split screen mode this is incremented once for all views (not for each view). */
extern CORE_API uint32 GFrameNumber;

/** Render Thread copy of the frame number. */
extern CORE_API uint32 GFrameNumberRenderThread;

#if !(UE_BUILD_SHIPPING && WITH_EDITOR)
	// We cannot count on this variable to be accurate in a shipped game, so make sure no code tries to use it
	/** Whether we are the first instance of the game running. */
	extern CORE_API bool GIsFirstInstance;
#endif

/** Threshold for a frame to be considered a hitch (in seconds. */
extern CORE_API float GHitchThreshold;


/** Whether to forcefully enable capturing of stats due to profiler attached */
extern CORE_API bool GProfilerAttached;

/** Size to break up data into when saving compressed data */
extern CORE_API int32 GSavingCompressionChunkSize;

/** Whether we are using the seek-free/ cooked loading code path. */
extern CORE_API bool GUseSeekFreeLoading;

/** Thread ID of the main/game thread */
extern CORE_API uint32 GGameThreadId;

/** Thread ID of the render thread, if any */
extern CORE_API uint32 GRenderThreadId;

/** Thread ID of the slate thread, if any */
extern CORE_API uint32 GSlateLoadingThreadId;

/** Has GGameThreadId been set yet? */
extern CORE_API bool GIsGameThreadIdInitialized;

/** Whether to emit begin/ end draw events. */
extern CORE_API bool GEmitDrawEvents;

/** Whether we want the rendering thread to be suspended, used e.g. for tracing. */
extern CORE_API bool GShouldSuspendRenderingThread;

/** Whether we want to use a fixed time step or not. */
extern CORE_API bool GUseFixedTimeStep;

/** Determines what kind of trace should occur, NAME_None for none. */
extern CORE_API FName GCurrentTraceName;

/** How to print the time in log output. */
extern CORE_API ELogTimes::Type GPrintLogTimes;

/** Global screen shot index to avoid overwriting ScreenShots. */
extern CORE_API int32 GScreenshotBitmapIndex;

/** Whether stats should emit named events for e.g. PIX. */
extern CORE_API bool GCycleStatsShouldEmitNamedEvents;

/** Disables some warnings and minor features that would interrupt a demo presentation*/
extern CORE_API bool GIsDemoMode;

/** Name of the core package. */
//@Package name transition, remove the double checks 
extern CORE_API FName GLongCorePackageName;
//@Package name transition, remove the double checks 
extern CORE_API FName GLongCoreUObjectPackageName;

/**
 * Whether to show slate batches.
 * @todo UE4: This does not belong here but is needed at the core level so slate will compile
 */
#if STATS
	extern CORE_API bool GShowSlateBatches;
#endif

/** Whether or not a unit test is currently being run. */
extern CORE_API bool GIsAutomationTesting;

/** Constrain bandwidth if wanted. Value is in MByte/ sec. */
extern CORE_API float GAsyncIOBandwidthLimit;

/** Whether or not messages are being pumped outside of main loop */
extern CORE_API bool GPumpingMessagesOutsideOfMainLoop;

/** Total blueprint compile time. */
extern CORE_API double GBlueprintCompileTime;

/*----------------------------------------------------------------------------
	Metadata macros.
----------------------------------------------------------------------------*/

#define CPP       1
#define STRUCTCPP 1
#define DEFAULTS  0


/*-----------------------------------------------------------------------------
	Seek-free defines.
-----------------------------------------------------------------------------*/

#define STANDALONE_SEEKFREE_SUFFIX	TEXT("_SF")


/*----------------------------------------------------------------------------
	Includes.
----------------------------------------------------------------------------*/

#include "FileManager.h"				// File manager interface
#include "ScopedDebugInfo.h"			// Scoped debug info.
#include "ExternalProfiler.h"			// External profiler integration.
#include "MemoryBase.h"					// Base memory allocation
#include "ByteSwap.h"					// ByteSwapping utility code.
#include "ITransaction.h"				// Transaction handling
#include "Compression.h"				// Low level compression routines
#include "StringUtility.h"				// Low level string utility code.
#include "Parse.h"						// String parsing functions
#include "StringConv.h"					// Low level string conversion
#include "Crc.h"						// Crc functions
#include "ObjectVersion.h"				// Object version info.
#include "TypeHash.h"					// Type hashing functions
#include "EnumAsByte.h"					// Common template definitions.
#include "ArchiveBase.h"				// Archive class.
#include "ScopedPointer.h"				// Scoped pointer definitions.
#include "Sorting.h"					// Sorting definitions.
#include "Array.h"						// Dynamic array definitions.
#include "ArrayBuilder.h"				// Builder template for arrays.
#include "BitArray.h"					// Bit array definition.
#include "Bitstreams.h"					// Bit stream archiver.
#include "SparseArray.h"				// Sparse array definitions.
#include "UnrealString.h"				// Dynamic string definitions.
#include "CoreMisc.h"					// Low level utility code.
#include "Paths.h"						// Path helper functions
#include "StaticArray.h"                // Static array definition.
#include "StaticBitArray.h"             // Static bit array definition.
#include "Set.h"						// Set definitions.
#include "Map.h"						// Dynamic map definitions.
#include "MapBuilder.h"					// Builder template for maps.
#include "List.h"						// Dynamic list definitions.
#include "ResourceArray.h"				// Resource array definitions.
#include "RefCounting.h"				// Reference counting definitions.
#include "NameTypes.h"					// Global name subsystem.
#include "ScriptDelegates.h"
#include "Delegate.h"					// C++ delegate system
#include "ThreadingBase.h"				// Non-platform specific multi-threaded support.
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"
#include "Guid.h"						// FGuid class
#include "NetworkGuid.h"				// FNetworkGuid class
#include "AffinityManager.h"			// Non-platform Affinity information for threads.
#include "UnrealMath.h"					// Vector math functions.
#include "SHMath.h"						// SH math functions.
#include "RandomStream.h"				// Random stream definitions.
#include "OutputDevices.h"				// Output devices
#include "CoreStats.h"
#include "MemStack.h"					// Stack based memory management.
#include "AsyncWork.h"					// Async threaded work
#include "Archive.h"					// Utility archive classes
#include "IOBase.h"						// base IO declarations, FIOManager, FIOSystem
#include "Variant.h"
#include "WildcardString.h"
#include "CircularBuffer.h"
#include "CircularQueue.h"
#include "Queue.h"
#include "Ticker.h"						// Efficient scheduled delegate manager
#include "RocketSupport.h"				// Core support for launching in "Rocket" mode
#include "ProfilingHelpers.h"			// Misc profiling helpers.
#include "ConfigCacheIni.h"				// The configuration cache declarations
#include "IConsoleManager.h"
#include "FeedbackContext.h"
#include "AutomationTest.h"				// Automation testing functionality for game, editor, etc.
#include "CallbackDevice.h"				// Base class for callback devices.
#include "ObjectThumbnail.h"			// Object thumbnails
#include "LocalTimestampDirectoryVisitor.h"
#include "GenericWindowDefinition.h"
#include "GenericWindow.h"
#include "GenericApplicationMessageHandler.h"
#include "GenericApplication.h"
#include "ICursor.h"
#include "CustomVersion.h"				// Custom versioning system.
#include "App.h"
#include "OutputDeviceConsole.h"
#include "MonitoredProcess.h"

#ifdef TRUE
	#undef TRUE
#endif

#ifdef FALSE
	#undef FALSE
#endif


// Trick to prevent people from using old UE4 types (which are still defined in Windef.h on PC).
// If old type is really necessary please use the global scope operator to access it (i.e. ::INT).
// Windows header file includes can be wrapped in #include "AllowWindowsPlatformTypes.h" and
// #include "HideWindowsPlatformTypes.h"
namespace DoNotUseOldUE4Type
{
	class FUnusableType
	{
		FUnusableType();
		~FUnusableType();
	};

	typedef FUnusableType INT;
	typedef FUnusableType UINT;
	typedef FUnusableType DWORD;
	typedef FUnusableType FLOAT;

	typedef FUnusableType TRUE;
	typedef FUnusableType FALSE;
}



using namespace DoNotUseOldUE4Type;


