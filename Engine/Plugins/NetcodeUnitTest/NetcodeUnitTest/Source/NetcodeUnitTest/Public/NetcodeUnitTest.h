// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Compatibility defines
 */

// @todo #JohnB: Consider discarding the compatibility defines. Or at least, removing the ones made redundant by refactoring.

// @todo #JohnBRefactor: Perhaps move compatibility defines to its own header

// @todo #JohnB: I need accurate mainline CL numbers for UT integrations (they aren't included in the merge changelist descriptions)


/** List of mainline UE4 changelists, representing UE4 version changes (as judged by changes to Version.h) */
#define CL_4_10	2626674
#define CL_4_9	2526821
#define CL_4_8	2388573
#define CL_4_7	2347015
#define CL_4_6	2308471

/** List of UnrealTournament mainline merges (number represents merged mainline CL) NOTE: Estimated. No integrated CL number in merge */
#define CL_UT_4_8 CL_4_9	// Integrated with final 4.8 release, which is approximately same time as main branch started 4.9
#define CL_UT_4_7 CL_4_7
#define CL_UT_4_6 CL_4_6

/** List of Fortnite mainline merges (number represents merged mainline CL) */
#define CL_FORT_4_8_APRIL 2509925
#define CL_FORT_4_8 2415178
#define CL_FORT_4_7 2349525


/**
 * List of mainline UE4 engine changelists, that required a NetcodeUnitTest compatibility adjustment (newest first)
 * Every time you update NetcodeUnitTest code, to toggle based on a changelist, add a define for it with the CL number here
 */
#define CL_HELLOENCRYPTION	3578855
#define CL_PREVERSIONCHANGE	2960134	// Not an accurate CL, just refers to the first version of UE4 code I can be sure changes work on
#define CL_STATELESSCONNECT	2866629	// @todo #JohnB: Refers to Dev-Networking, update so it refers to /UE4//Main merge CL
#define CL_FENGINEVERSION	2655102
#define CL_INITCONNPARAM	2567692
#define CL_CONSTUNIQUEID	2540329
#define CL_CONSTNETCONN		2501704
#define CL_INPUTCHORD		2481648
#define CL_CLOSEPROC		2476050
#define CL_STRINGPARSEARRAY	2466824
#define CL_BEACONHOST		2456855
#define CL_GETSELECTIONMODE	2425976
#define CL_DEPRECATENEW		2425600
#define CL_DEPRECATEDEL		2400883
#define CL_FNETWORKVERSION	2384479



/**
 * The changelist number for //depot/UE4/... that NetcodeUnitTest should adjust compatibility for. Set to the CL your workspace uses.
 *
 * If using NetcodeUnitTest with a different branch (e.g. UnrealTournament/Fortnite), target the last-merged CL from //depot/UE4/...
 *
 * If in doubt, set to the top CL from list above ('List of mainline UE4 engine changelists'), and work your way down until it compiles.
 */
#define TARGET_UE4_CL 3645298	// IMPORTANT: If you're hovering over this because compiling failed, you need to adjust this value.


/**
 * Forward declarations
 */

class UUnitTestManager;



/**
 * Globals/externs
 */

/** Holds a reference to the object in charge of managing unit tests */
extern UUnitTestManager* GUnitTestManager;

/** Whether not an actor channel, is in the process of initializing the remote actor */
extern bool GIsInitializingActorChan;


/**
 * Declarations
 */

NETCODEUNITTEST_API DECLARE_LOG_CATEGORY_EXTERN(LogUnitTest, Log, All);

// Hack to allow log entries to print without the category (specify log type of 'none')
DECLARE_LOG_CATEGORY_EXTERN(NetCodeTestNone, Log, All);


/**
 * Defines
 */

/**
 * The IPC pipe used for resuming a suspended server
 * NOTE: You must append the process ID of the server to this string
 */
#define NUT_SUSPEND_PIPE TEXT("\\\\.\\Pipe\\NetcodeUnitTest_SuspendResume")


/**
 * Macro defines
 */

// Actual asserts (not optimized out, like almost all other engine assert macros)
// @todo #JohnBLowPri: Try to get this to show up a message box, or some other notification, rather than just exiting immediately
#define UNIT_ASSERT(Condition) \
	if (!(Condition)) \
	{ \
		UE_LOG(LogUnitTest, Error, TEXT(PREPROCESSOR_TO_STRING(__FILE__)) TEXT(": Line: ") TEXT(PREPROCESSOR_TO_STRING(__LINE__)) \
				TEXT(":")); \
		UE_LOG(LogUnitTest, Error, TEXT("Condition '(") TEXT(PREPROCESSOR_TO_STRING(Condition)) TEXT(")' failed")); \
		check(false); /* Try to get a meaningful stack trace. */ \
		FPlatformMisc::RequestExit(true); \
		CA_ASSUME(false); \
	}


