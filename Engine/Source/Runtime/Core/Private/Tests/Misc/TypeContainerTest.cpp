// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#if PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES


#include "CorePrivatePCH.h"
#include "Function.h"
#include "Async.h"
#include "Future.h"
#include "TypeContainer.h"
#include "AutomationTest.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTypeContainerTest, "Core.Misc.TypeContainer", EAutomationTestFlags::ATF_SmokeTest)


/* Helpers
 *****************************************************************************/

struct IFruit
{
	DECLARE_REGISTRABLE_TYPE(IFruit)
	virtual ~IFruit() { }
	virtual FString Name() = 0;
};

struct IBerry : public IFruit
{
	DECLARE_REGISTRABLE_TYPE(IBerry)
	virtual ~IBerry() { }
};

struct FBanana : public IFruit
{
	virtual ~FBanana() { }
	virtual FString Name() override { return TEXT("Cherry"); }
};

struct FStrawberry : public IBerry
{
	virtual ~FStrawberry() { }
	virtual FString Name() override { return TEXT("Strawberry"); }
};

struct ISmoothie
{
	DECLARE_REGISTRABLE_TYPE(ISmoothie)
	virtual ~ISmoothie() { }
	virtual TSharedRef<IBerry> GetBerry() = 0;
	virtual TSharedRef<IFruit> GetFruit() = 0;
};

struct FSmoothie : public ISmoothie
{
	FSmoothie(TSharedRef<IFruit> InFruit, TSharedRef<IBerry> InBerry) : Berry(InBerry), Fruit(InFruit) { }
	virtual ~FSmoothie() { }
	virtual TSharedRef<IBerry> GetBerry() override { return Berry; }
	virtual TSharedRef<IFruit> GetFruit() override { return Fruit; }
	TSharedRef<IBerry> Berry;
	TSharedRef<IFruit> Fruit;
};

struct FTwoSmoothies
{
	TSharedPtr<ISmoothie> One;
	TSharedPtr<ISmoothie> Two;
};


/* Tests
 *****************************************************************************/

bool FTypeContainerTest::RunTest(const FString& Parameters)
{
	// existing instance test
	{
		TSharedRef<IFruit> Fruit = MakeShareable(new FBanana());
		TSharedRef<IBerry> Berry = MakeShareable(new FStrawberry());

		FTypeContainer Container;
		{
			Container.RegisterInstance<IFruit>(Fruit);
			Container.RegisterInstance<IBerry>(Berry);
		}

		auto Instance = Container.GetInstance<IFruit>();

		TestTrue(TEXT("Existing registered instance must be returned"), Instance.IsValid());

		if (Instance.IsValid())
		{
			TestEqual(TEXT("Correct instance must be returned"), Instance.ToSharedRef(), Fruit);
			TestNotEqual(TEXT("Incorrect instance must not be returned"), Instance.ToSharedRef(), StaticCastSharedRef<IFruit>(Berry));
		}
	}

	// per instance test
	{
		FTypeContainer Container;
		{
			Container.RegisterClass<IFruit, FBanana>(ETypeContainerScope::Instance);
			Container.RegisterClass<IBerry, FStrawberry>(ETypeContainerScope::Instance);
			Container.RegisterClass<ISmoothie, FSmoothie, IFruit, IBerry>(ETypeContainerScope::Instance); // !!!
		}

		auto Smoothie1 = Container.GetInstance<ISmoothie>();
		auto Smoothie2 = Container.GetInstance<ISmoothie>();

		TestTrue(TEXT("For per-instance classes, an instance must be returned [1]"), Smoothie1.IsValid());
		TestTrue(TEXT("For per-instance classes, an instance must be returned [2]"), Smoothie2.IsValid());
		TestNotEqual(TEXT("For per-instances classes, a unique instance must be returned each time"), Smoothie1, Smoothie2);
		TestNotEqual(TEXT("For per-instances dependencies, a unique instance must be returned each time [1]"), Smoothie1->GetBerry(), Smoothie2->GetBerry());
		TestNotEqual(TEXT("For per-instances dependencies, a unique instance must be returned each time [2]"), Smoothie1->GetFruit(), Smoothie2->GetFruit());
	}

	// per thread test
	{
		FTypeContainer Container;
		{
			Container.RegisterClass<IFruit, FBanana>(ETypeContainerScope::Thread);
			Container.RegisterClass<IBerry, FStrawberry>(ETypeContainerScope::Instance);
			Container.RegisterClass<ISmoothie, FSmoothie, IFruit, IBerry>(ETypeContainerScope::Thread); // !!!
		}

		TFunction<FTwoSmoothies()> MakeSmoothies = [&]()
		{
			FTwoSmoothies Smoothies;
			Smoothies.One = Container.GetInstance<ISmoothie>();
			Smoothies.Two = Container.GetInstance<ISmoothie>();
			return Smoothies;
		};

		auto Smoothies1 = Async(EAsyncExecution::Thread, MakeSmoothies);
		auto Smoothies2 = Async(EAsyncExecution::Thread, MakeSmoothies);

		TSharedPtr<ISmoothie> One1 = Smoothies1.Get().One;
		TSharedPtr<ISmoothie> Two1 = Smoothies1.Get().Two;
		TSharedPtr<ISmoothie> One2 = Smoothies2.Get().One;
		TSharedPtr<ISmoothie> Two2 = Smoothies2.Get().Two;

		TestTrue(TEXT("For per-thread classes, an instance must be returned [1]"), One1.IsValid());
		TestTrue(TEXT("For per-thread classes, an instance must be returned [2]"), Two1.IsValid());
		TestTrue(TEXT("For per-thread classes, an instance must be returned [3]"), One2.IsValid());
		TestTrue(TEXT("For per-thread classes, an instance must be returned [4]"), Two2.IsValid());
		TestEqual(TEXT("For per-thread classes, the same instance must be returned from the same thread [1]"), One1, Two1);
		TestEqual(TEXT("For per-thread classes, the same instance must be returned from the same thread [2]"), One2, Two2);
		TestNotEqual(TEXT("For per-thread classes, different instances must be returned from different threads [1]"), One1, One2);
		TestNotEqual(TEXT("For per-thread classes, different instances must be returned from different threads [2]"), Two1, Two2);
	}

	// per process test
	{
		FTypeContainer Container;
		{
			Container.RegisterClass<IFruit, FBanana>(ETypeContainerScope::Thread);
			Container.RegisterClass<IBerry, FStrawberry>(ETypeContainerScope::Instance);
			Container.RegisterClass<ISmoothie, FSmoothie, IFruit, IBerry>(ETypeContainerScope::Process); //!!!
		}

		TFunction<FTwoSmoothies()> MakeSmoothies = [&]()
		{
			FTwoSmoothies Smoothies;
			Smoothies.One = Container.GetInstance<ISmoothie>();
			Smoothies.Two = Container.GetInstance<ISmoothie>();
			return Smoothies;
		};

		auto Smoothies1 = Async(EAsyncExecution::Thread, MakeSmoothies);
		auto Smoothies2 = Async(EAsyncExecution::Thread, MakeSmoothies);

		TSharedPtr<ISmoothie> One1 = Smoothies1.Get().One;
		TSharedPtr<ISmoothie> Two1 = Smoothies1.Get().Two;
		TSharedPtr<ISmoothie> One2 = Smoothies2.Get().One;
		TSharedPtr<ISmoothie> Two2 = Smoothies2.Get().Two;

		TestTrue(TEXT("For per-process classes, an instance must be returned [1]"), One1.IsValid());
		TestTrue(TEXT("For per-process classes, an instance must be returned [2]"), Two1.IsValid());
		TestTrue(TEXT("For per-process classes, an instance must be returned [3]"), One2.IsValid());
		TestTrue(TEXT("For per-process classes, an instance must be returned [4]"), Two2.IsValid());
		TestEqual(TEXT("For per-process classes, the same instance must be returned from the same thread [1]"), One1, Two1);
		TestEqual(TEXT("For per-process classes, the same instance must be returned from the same thread [2]"), One2, Two2);
		TestEqual(TEXT("For per-process classes, the same instance must be returned from different threads [1]"), One1, One2);
		TestEqual(TEXT("For per-process classes, the same instance must be returned from different threads [1]"), Two1, Two2);
	}

	return true;
}


#endif //PLATFORM_COMPILER_HAS_VARIADIC_TEMPLATES
