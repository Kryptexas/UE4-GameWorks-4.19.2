// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "CorePrivatePCH.h"
#include "Function.h"
#include "Async.h"
#include "AutomationTest.h"
#include "Future.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAsyncTest, "Core.Async.Async", EAutomationTestFlags::ATF_Editor)


bool FAsyncTest::RunTest(const FString& Parameters)
{
	auto Task = []()
	{
		return 123;
	};

	// task graph task
	{
		auto Future = Async<int>(EAsyncExecution::TaskGraph, Task);
		int Result = Future.Get();

		TestEqual(TEXT("Task graph task must return expected value"), Result, 123);
	}

	// thread task
	{
		auto Future = Async<int>(EAsyncExecution::Thread, Task);
		int Result = Future.Get();

		TestEqual(TEXT("Task graph task must return expected value"), Result, 123);
	}

	return true;
}
