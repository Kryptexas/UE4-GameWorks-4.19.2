// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EQSTestingPawn.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

DEFINE_LOG_CATEGORY(LogEQS);

DEFINE_STAT(STAT_AI_EQS_Tick);
DEFINE_STAT(STAT_AI_EQS_LoadTime);
DEFINE_STAT(STAT_AI_EQS_GeneratorTime);
DEFINE_STAT(STAT_AI_EQS_TestTime);
DEFINE_STAT(STAT_AI_EQS_NumInstances);
DEFINE_STAT(STAT_AI_EQS_NumItems);
DEFINE_STAT(STAT_AI_EQS_InstanceMemory);

//////////////////////////////////////////////////////////////////////////
// FEnvQueryRequest

FEnvQueryRequest& FEnvQueryRequest::SetNamedParams(const TArray<struct FEnvNamedValue>& Params)
{
	for (int32 ParamIndex = 0; ParamIndex < Params.Num(); ParamIndex++)
	{
		NamedParams.Add(Params[ParamIndex].ParamName, Params[ParamIndex].Value);
	}

	return *this;
}

int32 FEnvQueryRequest::Execute(EEnvQueryRunMode::Type RunMode, FQueryFinishedSignature const& FinishDelegate)
{
	if (Owner == NULL)
	{
		Owner = FinishDelegate.GetUObject();
		if (Owner == NULL)
		{
			UE_LOG(LogEQS, Warning, TEXT("Unknown owner of request: %s"), *GetNameSafe(QueryTemplate));
			return INDEX_NONE;
		}
	}

	if (World == NULL)
	{
		World = GEngine->GetWorldFromContextObject(Owner);
		if (World == NULL)
		{
			UE_LOG(LogEQS, Warning, TEXT("Unable to access world with owner: %s"), *GetNameSafe(Owner));
			return INDEX_NONE;
		}
	}

	UEnvQueryManager* EnvQueryManager = World->GetEnvironmentQueryManager();
	if (EnvQueryManager == NULL)
	{
		UE_LOG(LogEQS, Warning, TEXT("Missing EQS manager!"));
		return INDEX_NONE;
	}

	return EnvQueryManager->RunQuery(*this, RunMode, FinishDelegate);
}


//////////////////////////////////////////////////////////////////////////
// UEnvQueryManager

TArray<TSubclassOf<UEnvQueryItemType> > UEnvQueryManager::RegisteredItemTypes;

UEnvQueryManager::UEnvQueryManager(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		FCoreDelegates::PreLoadMap.AddUObject(this, &UEnvQueryManager::OnPreLoadMap);
	}

	NextQueryID = 0;
}

void UEnvQueryManager::FinishDestroy()
{
	FCoreDelegates::PreLoadMap.RemoveAll(this);
	Super::FinishDestroy();
}

UEnvQueryManager* UEnvQueryManager::GetCurrent(UWorld* World)
{
	return World ? World->GetEnvironmentQueryManager() : NULL;
}

UEnvQueryManager* UEnvQueryManager::GetCurrent(class UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? GEngine->GetWorldFromContextObject(WorldContextObject) : NULL;
	return World ? World->GetEnvironmentQueryManager() : NULL;
}

#if WITH_EDITOR
void UEnvQueryManager::NotifyAssetUpdate(UEnvQuery* Query)
{
	if (GEditor == NULL)
	{
		return;
	}

	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (World)
	{
		UEnvQueryManager* EQS = World->GetEnvironmentQueryManager();
		if (EQS)
		{
			EQS->InstanceCache.Reset();
		}

		// was as follows, but got broken with changes to actor iterator (FActorIteratorBase::SpawnedActorArray)
		// for (TActorIterator<AEQSTestingPawn> It(World); It; ++It)
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AEQSTestingPawn* EQSPawn = Cast<AEQSTestingPawn>(*It);
			if (EQSPawn == NULL)
			{
				continue;
			}

			if (EQSPawn->QueryTemplate == Query || Query == NULL)
			{
				EQSPawn->RunEQSQuery();
			}
		}
	}
}
#endif // WITH_EDITOR

TStatId UEnvQueryManager::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UEnvQueryManager, STATGROUP_Tickables);
}

int32 UEnvQueryManager::RunQuery(const FEnvQueryRequest& Request, EEnvQueryRunMode::Type RunMode, FQueryFinishedSignature const& FinishDelegate)
{
	TSharedPtr<FEnvQueryInstance> QueryInstance = PrepareQueryInstance(Request, RunMode);
	if (QueryInstance.IsValid() == false)
	{
		return INDEX_NONE;
	}

	QueryInstance->FinishDelegate = FinishDelegate;
	RunningQueries.Add(QueryInstance);

	return QueryInstance->QueryID;
}

TSharedPtr<FEnvQueryResult> UEnvQueryManager::RunInstantQuery(const FEnvQueryRequest& Request, EEnvQueryRunMode::Type RunMode)
{
	TSharedPtr<FEnvQueryInstance> QueryInstance = PrepareQueryInstance(Request, RunMode);
	if (!QueryInstance.IsValid())
	{
		return NULL;
	}

	while (QueryInstance->Status == EEnvQueryStatus::Processing)
	{
		QueryInstance->ExecuteOneStep((double)FLT_MAX);
	}

#if USE_EQS_DEBUGGER
	EQSDebugger.StoreQuery(QueryInstance);
#endif // USE_EQS_DEBUGGER

	return QueryInstance;
}

TSharedPtr<struct FEnvQueryInstance> UEnvQueryManager::PrepareQueryInstance(const FEnvQueryRequest& Request, EEnvQueryRunMode::Type RunMode)
{
	TSharedPtr<FEnvQueryInstance> QueryInstance = CreateQueryInstance(Request.QueryTemplate, RunMode);
	if (!QueryInstance.IsValid())
	{
		return NULL;
	}

	QueryInstance->World = Cast<UWorld>(GetOuter());
	QueryInstance->Owner = Request.Owner;

	DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, QueryInstance->NamedParams.GetAllocatedSize());

	// @TODO: interface for providing default named params (like custom ranges in AI)
	QueryInstance->NamedParams = Request.NamedParams;

	INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, QueryInstance->NamedParams.GetAllocatedSize());

	QueryInstance->QueryID = NextQueryID++;

	return QueryInstance;
}

bool UEnvQueryManager::AbortQuery(int32 RequestID)
{
	for (int32 QueryIndex = 0; QueryIndex < RunningQueries.Num(); QueryIndex++)
	{
		TSharedPtr<FEnvQueryInstance>& QueryInstance = RunningQueries[QueryIndex];
		if (QueryInstance->QueryID == RequestID &&
			QueryInstance->Status == EEnvQueryStatus::Processing)
		{
			QueryInstance->Status = EEnvQueryStatus::Aborted;
			QueryInstance->FinishDelegate.ExecuteIfBound(QueryInstance);
			
			RunningQueries.RemoveAt(QueryIndex);
			return true;
		}
	}

	return false;
}

void UEnvQueryManager::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_EQS_Tick);
	SET_DWORD_STAT(STAT_AI_EQS_NumInstances, RunningQueries.Num());
	// @TODO: threads?

	const double StartTime = FPlatformTime::Seconds();
	const double MaxAllowedSeconds = 0.010;
	double TimeLeft = MaxAllowedSeconds;
		
	while (TimeLeft > 0.0 && RunningQueries.Num() > 0)
	{
		for (int32 Index = 0; Index < RunningQueries.Num() && TimeLeft > 0.0; Index++)
		{
			TSharedPtr<FEnvQueryInstance>& QueryInstance = RunningQueries[Index];
			QueryInstance->ExecuteOneStep(TimeLeft);
			
			if (QueryInstance->Status != EEnvQueryStatus::Processing)
			{
#if USE_EQS_DEBUGGER
				EQSDebugger.StoreQuery(QueryInstance);
#endif // USE_EQS_DEBUGGER

				QueryInstance->FinishDelegate.ExecuteIfBound(QueryInstance);

				RunningQueries.RemoveAt(Index);
				Index--;
			}

			TimeLeft -= FPlatformTime::Seconds() - StartTime;
		}
	}
}

void UEnvQueryManager::OnPreLoadMap()
{
	if (RunningQueries.Num() > 0)
	{
		for (int32 Index = 0; Index < RunningQueries.Num(); Index++)
		{
			TSharedPtr<FEnvQueryInstance>& QueryInstance = RunningQueries[Index];
			if (QueryInstance->Status == EEnvQueryStatus::Processing)
			{
				QueryInstance->Status = EEnvQueryStatus::Failed;
				QueryInstance->FinishDelegate.ExecuteIfBound(QueryInstance);
			}
		}

		RunningQueries.Reset();
	}
}

namespace EnvQueryTestSort
{
	struct FAllMatching
	{
		FORCEINLINE bool operator()(const UEnvQueryTest& TestA, const UEnvQueryTest& TestB) const
		{
			// cheaper tests go first
			if (TestB.Cost > TestA.Cost)
			{
				return true;
			}

			// conditions go first
			const bool bConditionA = (TestA.TestPurpose != EEnvTestPurpose::Score); // Is Test A Filtering?
			const bool bConditionB = (TestB.TestPurpose != EEnvTestPurpose::Score); // Is Test B Filtering?
			if (bConditionA && !bConditionB)
			{
				return true;
			}

			// keep connection order (sort stability)
			return (TestB.TestOrder > TestA.TestOrder);
		}
	};

	struct FSingleResult
	{
		FSingleResult(EEnvTestCost::Type InHighestCost) : HighestCost(InHighestCost) {}

		FORCEINLINE bool operator()(const UEnvQueryTest& TestA, const UEnvQueryTest& TestB) const
		{
			// cheaper tests go first
			if (TestB.Cost > TestA.Cost)
			{
				return true;
			}

			const bool bConditionA = (TestA.TestPurpose != EEnvTestPurpose::Score); // Is Test A Filtering?
			const bool bConditionB = (TestB.TestPurpose != EEnvTestPurpose::Score); // Is Test B Filtering?
			if (TestA.Cost == HighestCost)
			{
				// highest cost: weights go first, conditions later (first match will return result)
				if (!bConditionA && bConditionB)
				{
					return true;
				}

				// conditions with weights before pure conditions
				const bool bNoWeightA = (TestA.WeightModifier >= EEnvTestWeight::Constant);
				const bool bNoWeightB = (TestB.WeightModifier >= EEnvTestWeight::Constant);
				if (!bNoWeightA && bNoWeightB)
				{
					return true;
				}
			}
			else
			{
				// lower costs: conditions go first to reduce amount of items
				if (bConditionA && !bConditionB)
				{
					return true;
				}
			}

			// keep connection order (sort stability)
			return (TestB.TestOrder > TestA.TestOrder);
		}

	protected:
		TEnumAsByte<EEnvTestCost::Type> HighestCost;
	};
}

UEnvQuery* UEnvQueryManager::FindQueryTemplate(const FString& QueryName) const
{
	for (int32 InstanceIndex = 0; InstanceIndex < InstanceCache.Num(); InstanceIndex++)
	{
		const UEnvQuery* QueryTemplate = InstanceCache[InstanceIndex].Template.Get();

		if (QueryTemplate != NULL && QueryTemplate->GetName() == QueryName)
		{
			return const_cast<UEnvQuery*>(QueryTemplate);
		}
	}

	for (FObjectIterator It(UEnvQuery::StaticClass()); It; ++It)
	{
		if (It->GetName() == QueryName)
		{
			return Cast<UEnvQuery>(*It);
		}
	}

	return NULL;
}

TSharedPtr<FEnvQueryInstance> UEnvQueryManager::CreateQueryInstance(const class UEnvQuery* Template, EEnvQueryRunMode::Type RunMode)
{
	if (Template == NULL)
	{
		return NULL;
	}

	// try to find entry in cache
	FEnvQueryInstance* InstanceTemplate = NULL;
	for (int32 InstanceIndex = 0; InstanceIndex < InstanceCache.Num(); InstanceIndex++)
	{
		if (InstanceCache[InstanceIndex].Template == Template &&
			InstanceCache[InstanceIndex].Instance.Mode == RunMode)
		{
			InstanceTemplate = &InstanceCache[InstanceIndex].Instance;
			break;
		}
	}

	// and create one if can't be found
	if (InstanceTemplate == NULL)
	{
		SCOPE_CYCLE_COUNTER(STAT_AI_EQS_LoadTime);
		
		{
			// memory stat tracking: temporary variable will exist only inside this section
			FEnvQueryInstanceCache NewCacheEntry;
			NewCacheEntry.Template = Template;
			NewCacheEntry.Instance.QueryName = Template->GetName();
			NewCacheEntry.Instance.Mode = RunMode;

			const int32 Idx = InstanceCache.Add(NewCacheEntry);
			InstanceTemplate = &InstanceCache[Idx].Instance;
		}

		for (int32 OptionIndex = 0; OptionIndex < Template->Options.Num(); OptionIndex++)
		{
			UEnvQueryOption* MyOption = Template->Options[OptionIndex];
			if (MyOption == NULL || MyOption->Generator == NULL)
			{
				UE_LOG(LogEQS, Error, TEXT("Trying to spawn a query with broken Template (empty generator): %s, option %d"),
					*GetNameSafe(Template), OptionIndex);

				continue;
			}

			EEnvTestCost::Type HighestCost(EEnvTestCost::Low);
			TArray<UEnvQueryTest*> SortedTests = MyOption->Tests;
			TSubclassOf<UEnvQueryItemType> GeneratedType = MyOption->Generator->ItemType;
			for (int32 TestIndex = SortedTests.Num() - 1; TestIndex >= 0; TestIndex--)
			{
				UEnvQueryTest* TestOb = SortedTests[TestIndex];
				if (TestOb == NULL || !TestOb->IsSupportedItem(GeneratedType))
				{
					UE_LOG(LogEQS, Warning, TEXT("Query [%s] can't use test [%s] in option %d [%s], removing it"),
						*GetNameSafe(Template), *GetNameSafe(TestOb), OptionIndex, *MyOption->Generator->OptionName);

					SortedTests.RemoveAt(TestIndex);
				}
				else if (HighestCost < TestOb->Cost)
				{
					HighestCost = TestOb->Cost;
				}
			}

			if (SortedTests.Num())
			{
				switch (RunMode)
				{
				case EEnvQueryRunMode::SingleResult:
					SortedTests.Sort(EnvQueryTestSort::FSingleResult(HighestCost));
					break;

				case EEnvQueryRunMode::AllMatching:
					SortedTests.Sort(EnvQueryTestSort::FAllMatching());
					break;

				default:
					{
						UEnum* RunModeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEnvQueryRunMode"));
						UE_LOG(LogEQS, Warning, TEXT("Query [%s] can't be sorted for RunMode: %d [%s]"),
							*GetNameSafe(Template), (int32)RunMode, RunModeEnum ? *RunModeEnum->GetEnumName(RunMode) : TEXT("??"));
					}
				}
			}

			CreateOptionInstance(MyOption, SortedTests, *InstanceTemplate);
		}

		if (InstanceTemplate->Options.Num() == 0)
		{
			UE_LOG(LogEQS, Warning, TEXT("Query [%s] doesn't have any valid options!"), *GetNameSafe(Template));
			return NULL;
		}
	}

	// create new instance
	TSharedPtr<FEnvQueryInstance> NewInstance(new FEnvQueryInstance(*InstanceTemplate));
	return NewInstance;
}

void UEnvQueryManager::CreateOptionInstance(class UEnvQueryOption* OptionTemplate, const TArray<UEnvQueryTest*>& SortedTests, struct FEnvQueryInstance& Instance)
{
	FEnvQueryOptionInstance OptionInstance;
	OptionInstance.GenerateDelegate = OptionTemplate->Generator->GenerateDelegate;
	OptionInstance.ItemType = OptionTemplate->Generator->ItemType;
	OptionInstance.bShuffleItems = true;

	OptionInstance.TestDelegates.AddZeroed(SortedTests.Num());
	for (int32 TestIndex = 0; TestIndex < SortedTests.Num(); TestIndex++)
	{
		const UEnvQueryTest* TestOb = SortedTests[TestIndex];
		OptionInstance.TestDelegates[TestIndex] = TestOb->ExecuteDelegate;

		// HACK!  TODO: Is this the correct replacement here?  or should it check just if SCORING ONLY?
		// always randomize when asking for single result
		// otherwise, can skip randomization if test wants to score every item
		if ((TestOb->TestPurpose != EEnvTestPurpose::Filter) && // We ARE scoring, regardless of whether or not we're filtering
			(Instance.Mode != EEnvQueryRunMode::SingleResult))
		{
			OptionInstance.bShuffleItems = false;
		}
	}

	DEC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Instance.Options.GetAllocatedSize());

	const int32 AddedIdx = Instance.Options.Add(OptionInstance);

	INC_MEMORY_STAT_BY(STAT_AI_EQS_InstanceMemory, Instance.Options.GetAllocatedSize() + Instance.Options[AddedIdx].GetAllocatedSize());
}

//----------------------------------------------------------------------//
// FEQSDebugger
//----------------------------------------------------------------------//
#if USE_EQS_DEBUGGER

void FEQSDebugger::StoreQuery(TSharedPtr<FEnvQueryInstance>& Query)
{
	TSharedPtr<FEnvQueryInstance>& StoredQuery = StoredQueries.FindOrAdd(Query->Owner.Get());
	StoredQuery = Query;
}

TSharedPtr<FEnvQueryInstance> FEQSDebugger::GetQueryForOwner(const UObject* Owner)
{
	return StoredQueries.FindRef(Owner);
}

const TSharedPtr<FEnvQueryInstance> FEQSDebugger::GetQueryForOwner(const UObject* Owner) const
{
	return StoredQueries.FindRef(Owner);
}

#endif // USE_EQS_DEBUGGER
