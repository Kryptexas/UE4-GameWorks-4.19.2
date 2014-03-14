// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#if WITH_EDITOR
#include "UnrealEd.h"
#endif

DEFINE_LOG_CATEGORY(LogEQS);

DEFINE_STAT(STAT_AI_EnvQuery_Tick);
DEFINE_STAT(STAT_AI_EnvQuery_LoadTime);
DEFINE_STAT(STAT_AI_EnvQuery_GeneratorTime);
DEFINE_STAT(STAT_AI_EnvQuery_TestTime);
DEFINE_STAT(STAT_AI_EnvQuery_NumInstances);
DEFINE_STAT(STAT_AI_EnvQuery_NumItems);
DEFINE_STAT(STAT_AI_EnvQuery_InstanceMemory);

//////////////////////////////////////////////////////////////////////////
// FEnvQueryRequest

FEnvQueryRequest& FEnvQueryRequest::SetNamedParams(const TArray<struct FEnvNamedValue>& Params)
{
	for (int32 i = 0; i < Params.Num(); i++)
	{
		NamedParams.Add(Params[i].ParamName, Params[i].Value);
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

#if WITH_EDITOR
	EQSDebugger.StoreQuery(QueryInstance);
#endif // WITH_EDITOR

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

	DEC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, QueryInstance->NamedParams.GetAllocatedSize());

	// @TODO: interface for providing default named params (like custom ranges in AI)
	QueryInstance->NamedParams = Request.NamedParams;

	INC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, QueryInstance->NamedParams.GetAllocatedSize());

	QueryInstance->QueryID = NextQueryID++;

	return QueryInstance;
}

bool UEnvQueryManager::AbortQuery(int32 RequestID)
{
	for (int32 i = 0; i < RunningQueries.Num(); i++)
	{
		TSharedPtr<FEnvQueryInstance>& QueryInstance = RunningQueries[i];
		if (QueryInstance->QueryID == RequestID &&
			QueryInstance->Status == EEnvQueryStatus::Processing)
		{
			QueryInstance->Status = EEnvQueryStatus::Aborted;
			QueryInstance->FinishDelegate.ExecuteIfBound(QueryInstance);
			
			RunningQueries.RemoveAt(i);
			return true;
		}
	}

	return false;
}

void UEnvQueryManager::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_AI_EnvQuery_Tick);
	SET_DWORD_STAT(STAT_AI_EnvQuery_NumInstances, RunningQueries.Num());
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
#if WITH_EDITOR
				EQSDebugger.StoreQuery(QueryInstance);
#endif // WITH_EDITOR

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
			const bool bConditionA = (TestA.Condition != EEnvTestCondition::NoCondition);
			const bool bConditionB = (TestB.Condition != EEnvTestCondition::NoCondition);
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

			const bool bConditionA = (TestA.Condition != EEnvTestCondition::NoCondition);
			const bool bConditionB = (TestB.Condition != EEnvTestCondition::NoCondition);
			if (TestA.Cost == HighestCost)
			{
				// highest cost: weights go first, conditions later (first match will return result)
				if (!bConditionA && bConditionB)
				{
					return true;
				}

				// conditions with weights before pure conditions
				const bool bNoWeightA = (TestA.WeightModifier == EEnvTestWeight::Flat);
				const bool bNoWeightB = (TestB.WeightModifier == EEnvTestWeight::Flat);
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

TSharedPtr<FEnvQueryInstance> UEnvQueryManager::CreateQueryInstance(class UEnvQuery* Template, EEnvQueryRunMode::Type RunMode)
{
	if (Template == NULL)
	{
		return NULL;
	}

	// try to find entry in cache
	FEnvQueryInstance* InstanceTemplate = NULL;
	for (int32 i = 0; i < InstanceCache.Num(); i++)
	{
		if (InstanceCache[i].Template == Template &&
			InstanceCache[i].Instance.Mode == RunMode)
		{
			InstanceTemplate = &InstanceCache[i].Instance;
			break;
		}
	}

	// and create one if can't be found
	if (InstanceTemplate == NULL)
	{
		SCOPE_CYCLE_COUNTER(STAT_AI_EnvQuery_LoadTime);
		
		{
			// memory stat tracking: temporary variable will exist only inside this section
			FEnvQueryInstanceCache NewCacheEntry;
			NewCacheEntry.Template = Template;
			NewCacheEntry.Instance.QueryName = Template->GetName();
			NewCacheEntry.Instance.Mode = RunMode;

			const int32 Idx = InstanceCache.Add(NewCacheEntry);
			InstanceTemplate = &InstanceCache[Idx].Instance;
		}

		for (int32 i = 0; i < Template->Options.Num(); i++)
		{
			UEnvQueryOption* MyOption = Template->Options[i];
			check(MyOption->Generator);

			EEnvTestCost::Type HighestCost(EEnvTestCost::Low);
			TArray<UEnvQueryTest*> SortedTests = MyOption->Tests;
			TSubclassOf<UEnvQueryItemType> GeneratedType = MyOption->Generator->ItemType;
			for (int32 iTest = SortedTests.Num() - 1; iTest >= 0; iTest--)
			{
				UEnvQueryTest* TestOb = SortedTests[iTest];
				if (!TestOb->IsSupportedItem(GeneratedType))
				{
					UE_LOG(LogEQS, Warning, TEXT("Query [%s] can't use test [%s] in option %d [%s], removing it"),
						*GetNameSafe(Template), *TestOb->GetName(), i, *MyOption->Generator->OptionName);

					SortedTests.RemoveAt(iTest);
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
	for (int32 iTest = 0; iTest < SortedTests.Num(); iTest++)
	{
		const UEnvQueryTest* TestOb = SortedTests[iTest];
		OptionInstance.TestDelegates[iTest] = TestOb->ExecuteDelegate;

		// always randomize when asking for single result
		// otherwise, can skip randomization if test wants to score every item
		if (TestOb->Condition == EEnvTestCondition::NoCondition &&
			Instance.Mode != EEnvQueryRunMode::SingleResult)
		{
			OptionInstance.bShuffleItems = false;
		}
	}

	DEC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, Instance.Options.GetAllocatedSize());

	const int32 AddedIdx = Instance.Options.Add(OptionInstance);

	INC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, Instance.Options.GetAllocatedSize() + Instance.Options[AddedIdx].GetAllocatedSize());
}

//----------------------------------------------------------------------//
// FEQSDebugger
//----------------------------------------------------------------------//
#if WITH_EDITOR

void FEQSDebugger::StoreQuery(TSharedPtr<FEnvQueryInstance>& Query)
{
	TSharedPtr<FEnvQueryInstance>& StoredQuery = StoredQueries.FindOrAdd(Query->Owner.Get());
	StoredQuery = Query;
}

TSharedPtr<FEnvQueryInstance> FEQSDebugger::GetQueryForOwner(const UObject* Owner)
{
	return StoredQueries.FindRef(Owner);
}

#endif // WITH_EDITOR
