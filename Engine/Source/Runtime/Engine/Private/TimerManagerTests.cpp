// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "TimerManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTimerManagerTest, "Engine.TimerManager", EAutomationTestFlags::ATF_Editor)

#define TIMER_TEST_TEXT( Format, ... ) FString::Printf(TEXT("%s - %d: %s"), TEXT(__FILE__) , __LINE__ , *FString::Printf(TEXT(Format), ##__VA_ARGS__) )

void TimerTest_TickWorld(UWorld* World, float Time = 1.f)
{
	const float Step = 0.1f;
	while(Time > 0.f)
	{
		World->Tick(ELevelTick::LEVELTICK_All, FMath::Min(Time, Step) );
		Time-=Step;

		// This is terrible but required for subticking like this.
		// we could always cache the real GFrameCounter at the start of our tests and restore it when finished.
		GFrameCounter++;
	}
}

class FDummy
{
public:
	FDummy() { Count = 0; }

	void Callback() { ++Count; }
	void Reset() { Count = 0; }

	uint8 Count;
};

// Make sure that the timer manager works as expected when given invalid delegates and handles
bool TimerManagerTest_InvalidTimers(UWorld* World, FAutomationTestBase* Test)
{
	FTimerManager& TimerManager = World->GetTimerManager();
	FTimerDelegate Delegate;
	FTimerHandle Handle;

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with an invalid delegate"), TimerManager.TimerExists(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with an invalid handle"), TimerManager.TimerExists(Handle));

	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with an invalid delegate"), TimerManager.IsTimerActive(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with an invalid handle"), TimerManager.IsTimerActive(Handle));

	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with an invalid delegate"), TimerManager.IsTimerPaused(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with an invalid handle"), TimerManager.IsTimerPaused(Handle));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRate called with an invalid delegate"), (TimerManager.GetTimerRate(Delegate) == -1.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRate called with an invalid handle"), (TimerManager.GetTimerRate(Handle) == -1.f));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with an invalid delegate"), (TimerManager.GetTimerElapsed(Delegate) == -1.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with an invalid handle"), (TimerManager.GetTimerElapsed(Handle) == -1.f));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with an invalid delegate"), (TimerManager.GetTimerRemaining(Delegate) == -1.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with an invalid handle"), (TimerManager.GetTimerRemaining(Handle) == -1.f));

	// these don't return values but we should run them to make sure they don't do something horrible like crash
	TimerManager.PauseTimer(Delegate);
	TimerManager.PauseTimer(Handle);

	TimerManager.UnPauseTimer(Delegate);
	TimerManager.UnPauseTimer(Handle);

	TimerManager.ClearTimer(Delegate);
	TimerManager.ClearTimer(Handle);
	
	return true;
}

// Make sure that the timer manager works as expected when given delegates and handles that aren't in the timer manager
bool TimerManagerTest_MissingTimers(UWorld* World, FAutomationTestBase* Test)
{
	FTimerManager& TimerManager = World->GetTimerManager();
	FTimerDelegate Delegate;
	FTimerHandle Handle;

	FDummy Dummy;
	Delegate.BindRaw(&Dummy, &FDummy::Callback);
	Handle.MakeValid();

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with an invalid delegate"), TimerManager.TimerExists(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with an invalid handle"), TimerManager.TimerExists(Handle));

	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with an invalid delegate"), TimerManager.IsTimerActive(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with an invalid handle"), TimerManager.IsTimerActive(Handle));

	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with an invalid delegate"), TimerManager.IsTimerPaused(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with an invalid handle"), TimerManager.IsTimerPaused(Handle));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRate called with an invalid delegate"), (TimerManager.GetTimerRate(Delegate) == -1.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRate called with an invalid handle"), (TimerManager.GetTimerRate(Handle) == -1.f));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with an invalid delegate"), (TimerManager.GetTimerElapsed(Delegate) == -1.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with an invalid handle"), (TimerManager.GetTimerElapsed(Handle) == -1.f));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with an invalid delegate"), (TimerManager.GetTimerRemaining(Delegate) == -1.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with an invalid handle"), (TimerManager.GetTimerRemaining(Handle) == -1.f));

	// these don't return values but we should run them to make sure they don't do something horrible like crash
	TimerManager.PauseTimer(Delegate);
	TimerManager.PauseTimer(Handle);

	TimerManager.UnPauseTimer(Delegate);
	TimerManager.UnPauseTimer(Handle);

	TimerManager.ClearTimer(Delegate);
	TimerManager.ClearTimer(Handle);

	return true;
}

bool TimerManagerTest_ValidTimer_Delegate(UWorld* World, FAutomationTestBase* Test)
{
	FTimerManager& TimerManager = World->GetTimerManager();
	FTimerDelegate Delegate;
	const float Rate = 1.5f;

	FDummy Dummy;

	Delegate.BindRaw(&Dummy, &FDummy::Callback);

	TimerManager.SetTimer(Delegate, Rate, false);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a pending timer"), TimerManager.IsTimerActive(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with a pending timer"), TimerManager.IsTimerPaused(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRate called with a pending timer"), (TimerManager.GetTimerRate(Delegate) == Rate));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a pending timer"), (TimerManager.GetTimerElapsed(Delegate) == 0.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a pending timer"), (TimerManager.GetTimerRemaining(Delegate) == Rate));

	// small tick to move the timer from the pending list to the active list, the timer will start counting time after this tick
	TimerTest_TickWorld(World, KINDA_SMALL_NUMBER);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with an active timer"), TimerManager.IsTimerActive(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with an active timer"), TimerManager.IsTimerPaused(Delegate));

	TimerTest_TickWorld(World);

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with an active timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Delegate), 1.f, KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with an active timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Delegate), Rate - 1.f, KINDA_SMALL_NUMBER)));

	TimerManager.PauseTimer(Delegate);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a paused timer"), TimerManager.TimerExists(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with a paused timer"), TimerManager.IsTimerActive(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerPaused called with a paused timer"), TimerManager.IsTimerPaused(Delegate));

	TimerTest_TickWorld(World);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a paused timer"), TimerManager.TimerExists(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with a paused timer"), TimerManager.IsTimerActive(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerPaused called with a paused timer"), TimerManager.IsTimerPaused(Delegate));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a paused timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Delegate), 1.f, KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a paused timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Delegate), Rate - 1.f, KINDA_SMALL_NUMBER)));

	TimerManager.UnPauseTimer(Delegate);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a pending timer"), TimerManager.IsTimerActive(Delegate));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with a pending timer"), TimerManager.IsTimerPaused(Delegate));

	TimerTest_TickWorld(World);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a completed timer"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("Count of callback executions"), Dummy.Count == 1);


	// Test resetting the timer
	TimerManager.SetTimer(Delegate, Rate, false);
	TimerManager.SetTimer(Delegate, 0.f, false);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a reset timer"), TimerManager.TimerExists(Delegate));

	// Test looping timers
	Dummy.Reset();
	TimerManager.SetTimer(Delegate, Rate, true);
	TimerTest_TickWorld(World, KINDA_SMALL_NUMBER);

	TimerTest_TickWorld(World, 2.f);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a looping timer"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a looping timer"), TimerManager.IsTimerActive(Delegate));

	Test->TestTrue(TIMER_TEST_TEXT("Count of callback executions"), Dummy.Count == 1);
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Delegate), 2.f - (Rate * Dummy.Count), KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Delegate), Rate * (Dummy.Count + 1) - 2.f, KINDA_SMALL_NUMBER)));

	TimerTest_TickWorld(World, 2.f);

	Test->TestTrue(TIMER_TEST_TEXT("Count of callback executions"), Dummy.Count == 2);
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Delegate), 4.f - (Rate * Dummy.Count), KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Delegate), Rate * (Dummy.Count + 1) - 4.f, KINDA_SMALL_NUMBER)));

	TimerManager.SetTimer(Delegate, 0.f, false);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a reset looping timer"), TimerManager.TimerExists(Delegate));

	Dummy.Reset();
	TimerManager.SetTimerForNextTick(Delegate);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a timer executing on the next tick"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a timer executing on the next tick"), TimerManager.IsTimerActive(Delegate));

	TimerTest_TickWorld(World, SMALL_NUMBER);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a timer executing on the previous tick"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("Count of callback executions"), Dummy.Count == 1);

	return true;
}

bool TimerManagerTest_ValidTimer_HandleWithDelegate(UWorld* World, FAutomationTestBase* Test)
{
	FTimerManager& TimerManager = World->GetTimerManager();
	FTimerDelegate Delegate;
	FTimerHandle Handle;
	const float Rate = 1.5f;

	FDummy Dummy;
	Delegate.BindRaw(&Dummy, &FDummy::Callback);

	TimerManager.SetTimer(Handle, Delegate, Rate, false);

	Test->TestTrue(TIMER_TEST_TEXT("Handle should be valid after calling SetTimer"), Handle.IsValid());
	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called by delegate with a pending timer"), TimerManager.TimerExists(Delegate));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a pending timer"), TimerManager.IsTimerActive(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with a pending timer"), TimerManager.IsTimerPaused(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRate called with a pending timer"), (TimerManager.GetTimerRate(Handle) == Rate));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a pending timer"), (TimerManager.GetTimerElapsed(Handle) == 0.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a pending timer"), (TimerManager.GetTimerRemaining(Handle) == Rate));

	// small tick to move the timer from the pending list to the active list, the timer will start counting time after this tick
	TimerTest_TickWorld(World, KINDA_SMALL_NUMBER);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with an active timer"), TimerManager.IsTimerActive(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with an active timer"), TimerManager.IsTimerPaused(Handle));

	TimerTest_TickWorld(World);

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with an active timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 1.f, KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with an active timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate - 1.f, KINDA_SMALL_NUMBER)));

	TimerManager.PauseTimer(Handle);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a paused timer"), TimerManager.TimerExists(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with a paused timer"), TimerManager.IsTimerActive(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerPaused called with a paused timer"), TimerManager.IsTimerPaused(Handle));

	TimerTest_TickWorld(World);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a paused timer"), TimerManager.TimerExists(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with a paused timer"), TimerManager.IsTimerActive(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerPaused called with a paused timer"), TimerManager.IsTimerPaused(Handle));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a paused timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 1.f, KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a paused timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate - 1.f, KINDA_SMALL_NUMBER)));

	TimerManager.UnPauseTimer(Handle);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a pending timer"), TimerManager.IsTimerActive(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with a pending timer"), TimerManager.IsTimerPaused(Handle));

	TimerTest_TickWorld(World);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a completed timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("Count of callback executions"), Dummy.Count == 1);


	// Test resetting the timer
	TimerManager.SetTimer(Handle, Delegate, Rate, false);
	TimerManager.SetTimer(Handle, 0.f, false);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a reset timer"), TimerManager.TimerExists(Handle));

	// Test looping timers
	Dummy.Reset();
	TimerManager.SetTimer(Handle, Delegate, Rate, true);
	TimerTest_TickWorld(World, KINDA_SMALL_NUMBER);

	TimerTest_TickWorld(World, 2.f);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a looping timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a looping timer"), TimerManager.IsTimerActive(Handle));

	Test->TestTrue(TIMER_TEST_TEXT("Count of callback executions"), Dummy.Count == 1);
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 2.f - (Rate * Dummy.Count), KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate * (Dummy.Count + 1) - 2.f, KINDA_SMALL_NUMBER)));

	TimerTest_TickWorld(World, 2.f);

	Test->TestTrue(TIMER_TEST_TEXT("Count of callback executions"), Dummy.Count == 2);
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 4.f - (Rate * Dummy.Count), KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate * (Dummy.Count + 1) - 4.f, KINDA_SMALL_NUMBER)));

	TimerManager.SetTimer(Handle, 0.f, false);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a reset looping timer"), TimerManager.TimerExists(Handle));

	return true;
}

bool TimerManagerTest_ValidTimer_HandleNoDelegate(UWorld* World, FAutomationTestBase* Test)
{
	FTimerManager& TimerManager = World->GetTimerManager();
	FTimerHandle Handle;
	const float Rate = 1.5f;

	TimerManager.SetTimer(Handle, Rate, false);

	Test->TestTrue(TIMER_TEST_TEXT("Handle should be valid after calling SetTimer"), Handle.IsValid());
	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a pending timer"), TimerManager.IsTimerActive(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with a pending timer"), TimerManager.IsTimerPaused(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRate called with a pending timer"), (TimerManager.GetTimerRate(Handle) == Rate));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a pending timer"), (TimerManager.GetTimerElapsed(Handle) == 0.f));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a pending timer"), (TimerManager.GetTimerRemaining(Handle) == Rate));

	// small tick to move the timer from the pending list to the active list, the timer will start counting time after this tick
	TimerTest_TickWorld(World, KINDA_SMALL_NUMBER);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with an active timer"), TimerManager.IsTimerActive(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with an active timer"), TimerManager.IsTimerPaused(Handle));

	TimerTest_TickWorld(World);

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with an active timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 1.f, KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with an active timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate - 1.f, KINDA_SMALL_NUMBER)));

	TimerManager.PauseTimer(Handle);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a paused timer"), TimerManager.TimerExists(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with a paused timer"), TimerManager.IsTimerActive(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerPaused called with a paused timer"), TimerManager.IsTimerPaused(Handle));

	TimerTest_TickWorld(World);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a paused timer"), TimerManager.TimerExists(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerActive called with a paused timer"), TimerManager.IsTimerActive(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerPaused called with a paused timer"), TimerManager.IsTimerPaused(Handle));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a paused timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 1.f, KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a paused timer after one step"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate - 1.f, KINDA_SMALL_NUMBER)));

	TimerManager.UnPauseTimer(Handle);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a pending timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a pending timer"), TimerManager.IsTimerActive(Handle));
	Test->TestFalse(TIMER_TEST_TEXT("IsTimerPaused called with a pending timer"), TimerManager.IsTimerPaused(Handle));

	TimerTest_TickWorld(World);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a completed timer"), TimerManager.TimerExists(Handle));

	// Test resetting the timer
	TimerManager.SetTimer(Handle, Rate, false);
	TimerManager.SetTimer(Handle, 0.f, false);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a reset timer"), TimerManager.TimerExists(Handle));

	// Test looping timers
	TimerManager.SetTimer(Handle, Rate, true);
	TimerTest_TickWorld(World, KINDA_SMALL_NUMBER);

	TimerTest_TickWorld(World, 2.f);

	Test->TestTrue(TIMER_TEST_TEXT("TimerExists called with a looping timer"), TimerManager.TimerExists(Handle));
	Test->TestTrue(TIMER_TEST_TEXT("IsTimerActive called with a looping timer"), TimerManager.IsTimerActive(Handle));

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 2.f - Rate, KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate * 2 - 2.f, KINDA_SMALL_NUMBER)));

	TimerTest_TickWorld(World, 2.f);

	Test->TestTrue(TIMER_TEST_TEXT("GetTimerElapsed called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerElapsed(Handle), 4.f - (Rate * 2), KINDA_SMALL_NUMBER)));
	Test->TestTrue(TIMER_TEST_TEXT("GetTimerRemaining called with a looping timer"), (FMath::IsNearlyEqual(TimerManager.GetTimerRemaining(Handle), Rate * 3 - 4.f, KINDA_SMALL_NUMBER)));

	TimerManager.SetTimer(Handle, 0.f, false);

	Test->TestFalse(TIMER_TEST_TEXT("TimerExists called with a reset looping timer"), TimerManager.TimerExists(Handle));

	return true;
}

bool FTimerManagerTest::RunTest(const FString& Parameters)
{
	UWorld *World = UWorld::CreateWorld(EWorldType::Game, false);
	FWorldContext &WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	
	FURL URL;
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();

	TimerManagerTest_InvalidTimers(World, this);
	TimerManagerTest_MissingTimers(World, this);
	TimerManagerTest_ValidTimer_Delegate(World, this);
	TimerManagerTest_ValidTimer_HandleWithDelegate(World, this);
	TimerManagerTest_ValidTimer_HandleNoDelegate(World, this);

	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	return true;
}


