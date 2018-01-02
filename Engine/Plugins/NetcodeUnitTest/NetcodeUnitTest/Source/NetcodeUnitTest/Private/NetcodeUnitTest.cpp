// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "NetcodeUnitTest.h"

#include "Misc/CommandLine.h"
#include "Modules/ModuleManager.h"
#include "Engine/World.h"

#include "UI/LogWidgetCommands.h"

#include "INetcodeUnitTest.h"
#include "NUTUtil.h"
#include "NUTUtilNet.h"
#include "NUTUtilDebug.h"
#include "NUTUtilReflection.h"
#include "UnitTestEnvironment.h"


/**
 * Globals
 */

bool GIsInitializingActorChan = false;

/** Limits DumpRPCs to RPC's (partially) matching the specified names */
static TArray<FString> DumpRPCMatches;


/**
 * Definitions/implementations
 */

DEFINE_LOG_CATEGORY(LogUnitTest);
DEFINE_LOG_CATEGORY(NetCodeTestNone);


/**
 * Module implementation
 */
class FNetcodeUnitTest : public INetcodeUnitTest
{
private:
	static FWorldDelegates::FWorldInitializationEvent::FDelegate OnWorldCreatedDelegate;

#if TARGET_UE4_CL >= CL_DEPRECATEDEL
	static FDelegateHandle OnWorldCreatedDelegateHandle;
#endif

	static FDelegateHandle DumpRPCsHandle;

public:
	/**
	 * Called upon loading of the NetcodeUnitTest library
	 */
	virtual void StartupModule() override
	{
		static bool bSetDelegate = false;

		if (!bSetDelegate && FParse::Param(FCommandLine::Get(), TEXT("NUTServer")))
		{
			OnWorldCreatedDelegate = FWorldDelegates::FWorldInitializationEvent::FDelegate::CreateStatic(
										&FNetcodeUnitTest::OnWorldCreated);

#if TARGET_UE4_CL >= CL_DEPRECATEDEL
			OnWorldCreatedDelegateHandle = FWorldDelegates::OnPreWorldInitialization.Add(OnWorldCreatedDelegate);
#else
			FWorldDelegates::OnPreWorldInitialization.Add(OnWorldCreatedDelegate);
#endif

			bSetDelegate = true;
		}

		FLogWidgetCommands::Register();
		FUnitTestEnvironment::Register();


		/**
		 * Hooks all RPC calls, and dumps the function + full parameter list, to the log.
		 * Optionally, can specify string matches for filtering RPC's, delimited with ','.
		 *
		 * Examples:
		 *	-DumpRPCs
		 *
		 *	-DumpRPCs="RPC1,RPC2"
		 */
		FString MatchesStr;

		if (FParse::Param(FCommandLine::Get(), TEXT("DumpRPCs")) || FParse::Value(FCommandLine::Get(), TEXT("DumpRPCs="), MatchesStr))
		{
			if (MatchesStr.Len() > 0)
			{
				MatchesStr.ParseIntoArray(DumpRPCMatches, TEXT(","));
			}

			DumpRPCsHandle = FProcessEventHook::Get().AddGlobalRPCHook(FOnProcessNetEvent::CreateStatic(&FNetcodeUnitTest::DumpRPC));
		}

		// Hack-override the log category name
#if !NO_LOGGING
		class FLogOverride : public FLogCategoryBase
		{
		public:
			// Used to access protected CategoryFName
			void OverrideName(FName InName)
			{
				CategoryFName = InName;
			}
		};

		((FLogOverride&)NetCodeTestNone).OverrideName(TEXT("None"));
#endif
	}

	/**
	 * Called immediately prior to unloading of the NetcodeUnitTest library
	 */
	virtual void ShutdownModule() override
	{
		// Eliminate active global variables
		GUnitTestManager = nullptr;

		if (GTraceManager != nullptr)
		{
			delete GTraceManager;

			GTraceManager = nullptr;
		}

		if (GLogTraceManager != nullptr)
		{
			delete GLogTraceManager;

			GLogTraceManager = nullptr;
		}
		

		FLogWidgetCommands::Unregister();
		FUnitTestEnvironment::Unregister();

		if (DumpRPCsHandle.IsValid())
		{
			FProcessEventHook::Get().RemoveGlobalRPCHook(DumpRPCsHandle);
			DumpRPCsHandle.Reset();
		}
	}


	/**
	 * Delegate implementation, for adding NUTActor to ServerActors, early on in engine startup
	 */
	static void OnWorldCreated(UWorld* UnrealWorld, const UWorld::InitializationValues IVS)
	{
		// If NUTActor isn't already in RuntimeServerActors, add it now
		if (GEngine != nullptr)
		{
			bool bNUTActorPresent = GEngine->RuntimeServerActors.ContainsByPredicate(
				[](const FString& CurServerActor)
				{
					return CurServerActor == TEXT("/Script/NetcodeUnitTest.NUTActor");
				});

			if (!bNUTActorPresent)
			{
				UE_LOG(LogUnitTest, Log, TEXT("NUTActor not present in RuntimeServerActors - adding this"));

				GEngine->RuntimeServerActors.Add(TEXT("/Script/NetcodeUnitTest.NUTActor"));
			}
		}

		// Now remove it, so it's only called once
#if TARGET_UE4_CL >= CL_DEPRECATEDEL
		FWorldDelegates::OnPreWorldInitialization.Remove(OnWorldCreatedDelegateHandle);
#else
		FWorldDelegates::OnPreWorldInitialization.Remove(OnWorldCreatedDelegate);
#endif
	}

	// @todo #JohnB: Presently this has a 'bug' (feature?) where it logs not just received RPC's, but sent RPC's as well.
	static void DumpRPC(AActor* Actor, UFunction* Function, void* Parameters, bool& bBlockRPC)
	{
		FString FuncName = Function->GetName();
		bool bContainsMatch = DumpRPCMatches.Num() == 0 || DumpRPCMatches.ContainsByPredicate(
								[&FuncName](FString& CurEntry)
								{
									return FuncName.Contains(CurEntry);
								});

		if (bContainsMatch)
		{
			FString FuncParms = NUTUtilRefl::FunctionParmsToString(Function, Parameters);

			UE_LOG(LogUnitTest, Log, TEXT("Received RPC '%s' for actor '%s'"), *FuncName, *Actor->GetFullName());

			if (FuncParms.Len() > 0)
			{
				UE_LOG(LogUnitTest, Log, TEXT("     '%s' parameters: %s"), *FuncName, *FuncParms);
			}
		}
	}
};

FWorldDelegates::FWorldInitializationEvent::FDelegate FNetcodeUnitTest::OnWorldCreatedDelegate = nullptr;

#if TARGET_UE4_CL >= CL_DEPRECATEDEL
FDelegateHandle FNetcodeUnitTest::OnWorldCreatedDelegateHandle;
#endif

FDelegateHandle FNetcodeUnitTest::DumpRPCsHandle;


// Essential for getting the .dll to compile, and for the package to be loadable
IMPLEMENT_MODULE(FNetcodeUnitTest, NetcodeUnitTest);
