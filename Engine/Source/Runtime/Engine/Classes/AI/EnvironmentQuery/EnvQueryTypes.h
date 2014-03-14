// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogEQS, Log, All);

// If set, normalized partial weights will be stored
#define EQS_STORE_PARTIAL_WEIGHTS		0

DECLARE_STATS_GROUP(TEXT("EnvironmentQuery"),STATGROUP_AIEnvQuery);

DECLARE_CYCLE_STAT_EXTERN(TEXT("Tick"),STAT_AI_EnvQuery_Tick,STATGROUP_AIEnvQuery, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Load Time"),STAT_AI_EnvQuery_LoadTime,STATGROUP_AIEnvQuery, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Generator Time"),STAT_AI_EnvQuery_GeneratorTime,STATGROUP_AIEnvQuery, );
DECLARE_CYCLE_STAT_EXTERN(TEXT("Test Time"),STAT_AI_EnvQuery_TestTime,STATGROUP_AIEnvQuery, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Instances"),STAT_AI_EnvQuery_NumInstances,STATGROUP_AIEnvQuery, );
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Num Items"),STAT_AI_EnvQuery_NumItems,STATGROUP_AIEnvQuery, );
DECLARE_MEMORY_STAT_EXTERN(TEXT("Instance memory"),STAT_AI_EnvQuery_InstanceMemory,STATGROUP_AIEnvQuery, ENGINE_API);

UENUM()
namespace EEnvTestCondition
{
	enum Type
	{
		NoCondition		UMETA(DisplayName="Always pass"),
		AtLeast			UMETA(DisplayName="At least"),
		UpTo			UMETA(DisplayName="Up to"),
		Match,
	};
}

UENUM()
namespace EEnvTestWeight
{
	enum Type
	{
		None,
		Square,
		Inverse,
		Absolute,
		Flat,
	};
}

UENUM()
namespace EEnvTestCost
{
	enum Type
	{
		Low,				// reading data, math operations (e.g. distance)
		Medium,				// processing data from multiple sources (e.g. fire tickets)
		High,				// really expensive calls (e.g. visibility traces, pathfinding)
	};
}

UENUM()
namespace EEnvQueryStatus
{
	enum Type
	{
		Processing,
		Success,
		Failed,
		Aborted,
		OwnerLost,
		MissingParam,
	};
}

UENUM()
namespace EEnvQueryRunMode
{
	enum Type
	{
		SingleResult,		// weight scoring first, try conditions from best result and stop after first item pass
		AllMatching,		// conditions first (limit set of items), weight scoring later
	};
}

UENUM()
namespace EEnvQueryParam
{
	enum Type
	{
		Float,
		Int,
		Bool,
	};
}

UENUM()
namespace EEnvQueryTrace
{
	enum Type
	{
		None,
		Navigation,
		Geometry_NOT_IMPLEMENTED,
	};
}

USTRUCT()
struct ENGINE_API FEnvFloatParam
{
	GENERATED_USTRUCT_BODY();

	typedef float FValueType;

	/** default value */
	UPROPERTY(EditDefaultsOnly, Category=Param)
	float Value;

	/** name of parameter */
	UPROPERTY(EditDefaultsOnly, Category=Param)
	FName ParamName;

	bool IsNamedParam() const { return ParamName != NAME_None; }
};

USTRUCT()
struct ENGINE_API FEnvIntParam
{
	GENERATED_USTRUCT_BODY();

	typedef int32 FValueType;

	/** default value */
	UPROPERTY(EditDefaultsOnly, Category=Param)
	int32 Value;

	/** name of parameter */
	UPROPERTY(EditDefaultsOnly, Category=Param)
	FName ParamName;

	bool IsNamedParam() const { return ParamName != NAME_None; }
};

USTRUCT()
struct ENGINE_API FEnvBoolParam
{
	GENERATED_USTRUCT_BODY();

	typedef bool FValueType;

	/** default value */
	UPROPERTY(EditDefaultsOnly, Category=Param)
	bool Value;

	/** name of parameter */
	UPROPERTY(EditDefaultsOnly, Category=Param)
	FName ParamName;

	bool IsNamedParam() const { return ParamName != NAME_None; }
};

USTRUCT(BlueprintType)
struct ENGINE_API FEnvNamedValue
{
	GENERATED_USTRUCT_BODY();

	UPROPERTY(EditAnywhere, Category=Param)
	FName ParamName;

	UPROPERTY(EditAnywhere, Category=Param)
	TEnumAsByte<EEnvQueryParam::Type> ParamType;

	UPROPERTY(EditAnywhere, Category=Param)
	float Value;
};


//////////////////////////////////////////////////////////////////////////
// Returned results

struct ENGINE_API FEnvQueryItem
{
	/** total score of item */
	float Score;

	/** raw data offset */
	int32 DataOffset:31;

	/** has this item been discarded */
	int32 bIsDiscarted:1;

	FORCEINLINE bool IsValid() const { return DataOffset >= 0 && !bIsDiscarted; }
	FORCEINLINE void Discard() { bIsDiscarted = true; }

	bool operator<(const FEnvQueryItem& Other) const
	{
		// sort by validity
		if (IsValid() != Other.IsValid())
		{
			// self not valid = less important
			return !IsValid();
		}

		// sort by score
		return Score < Other.Score;
	}

	FEnvQueryItem() : Score(0.0f), DataOffset(-1), bIsDiscarted(false) {}
	FEnvQueryItem(int32 InOffset) : Score(0.0f), DataOffset(InOffset), bIsDiscarted(false) {}
};

template <> struct TIsZeroConstructType<FEnvQueryItem> { enum { Value = true }; };

struct ENGINE_API FEnvQueryResult
{
	TArray<FEnvQueryItem> Items;

	/** type of generated items */
	TSubclassOf<class UEnvQueryItemType> ItemType;

	/** query status */
	TEnumAsByte<EEnvQueryStatus::Type> Status;

	/** raw data of items */
	TArray<uint8> RawData;

	/** index of query option, that generated items */
	int32 OptionIndex;

	/** instance ID */
	int32 QueryID;

	/** instance owner */
	TWeakObjectPtr<UObject> Owner;

	FORCEINLINE float GetItemScore(int32 Index) const { return Items.IsValidIndex(Index) ? Items[Index].Score : 0.0f; }

	/** item accessors for basic types */
	AActor* GetItemAsActor(int32 Index) const;
	FVector GetItemAsLocation(int32 Index) const;

	FEnvQueryResult() : ItemType(NULL), Status(EEnvQueryStatus::Processing), OptionIndex(0) {}
	FEnvQueryResult(const EEnvQueryStatus::Type& InStatus) : ItemType(NULL), Status(InStatus), OptionIndex(0) {}
};


//////////////////////////////////////////////////////////////////////////
// Runtime processing structures

DECLARE_DELEGATE_OneParam(FQueryFinishedSignature, TSharedPtr<struct FEnvQueryResult>);
DECLARE_DELEGATE_OneParam(FGenerateItemsSignature, struct FEnvQueryInstance&);
DECLARE_DELEGATE_OneParam(FExecuteTestSignature, struct FEnvQueryInstance&);

struct ENGINE_API FEnvQuerySpatialData
{
	FVector Location;
	FRotator Rotation;
};

/** Detailed information about item, used by tests */
struct ENGINE_API FEnvQueryItemDetails
{
	/** Results assigned by option's tests, before any modifications */
	TArray<float> TestResults;

#if EQS_STORE_PARTIAL_WEIGHTS
	/** Results assigned by option's tests, after applying modifiers, normalization and weight */
	TArray<float> TestWeightedScores;
#endif

#if WITH_EDITOR
	int32 FailedTestIndex;
#endif // WITH_EDITOR

	FEnvQueryItemDetails() {}
	FEnvQueryItemDetails(int32 NumTests)
	{
		TestResults.AddZeroed(NumTests);
#if EQS_STORE_PARTIAL_WEIGHTS
		TestWeightedScores.AddZeroed(NumTests);
#endif
	}

	FORCEINLINE uint32 GetAllocatedSize() const
	{
		return sizeof(*this) +
#if EQS_STORE_PARTIAL_WEIGHTS
			TestWeightedScores.GetAllocatedSize() +
#endif
			TestResults.GetAllocatedSize();
	}
};

struct ENGINE_API FEnvQueryContextData
{
	/** type of context values */
	TSubclassOf<class UEnvQueryItemType> ValueType;

	/** number of stored values */
	int32 NumValues;

	/** data of stored values */
	TArray<uint8> RawData;

	FORCEINLINE uint32 GetAllocatedSize() const { return sizeof(*this) + RawData.GetAllocatedSize(); }
};

struct ENGINE_API FEnvQueryOptionInstance
{
	/** generator's delegate */
	FGenerateItemsSignature GenerateDelegate;

	/** tests' delegates */
	TArray<FExecuteTestSignature> TestDelegates;

	/** type of generated items */
	TSubclassOf<class UEnvQueryItemType> ItemType;

	/** is set, items will be shuffled after tests */
	bool bShuffleItems;

	FORCEINLINE uint32 GetAllocatedSize() const { return sizeof(*this) + TestDelegates.GetAllocatedSize(); }
};

#if NO_LOGGING
#define EQSHEADERLOG(...)
#else
#define EQSHEADERLOG(msg) Log(msg)
#endif // NO_LOGGING

struct FEQSQueryDebugData
{
	TArray<FEnvQueryItem> DebugItems;
	TArray<FEnvQueryItemDetails> DebugItemDetails;
	TArray<uint8> RawData;
	TArray<FString> PerformedTestNames;

	void Store(const FEnvQueryInstance* QueryInstance);
	void Reset()
	{
		DebugItems.Reset();
		DebugItemDetails.Reset();
		RawData.Reset();
		PerformedTestNames.Reset();
	}
};

struct ENGINE_API FEnvQueryInstance : public FEnvQueryResult
{
	typedef float FNamedParamValueType;

	/** short name of query template */
	FString QueryName;

	/** world owning this query instance */
	UWorld* World;

	/** observer's delegate */
	FQueryFinishedSignature FinishDelegate;

	/** execution params */
	TMap<FName, FNamedParamValueType> NamedParams;

	/** contexts in use */
	TMap<UClass*, FEnvQueryContextData> ContextCache;

	/** list of options */
	TArray<struct FEnvQueryOptionInstance> Options;

	/** currently processed test (-1 = generator) */
	int32 CurrentTest;

	/** non-zero if test run last step has been stopped mid-process. This indicates
	 *	index of the first item that needs processing when resumed */
	int32 CurrentTestStartingItem;

	/** list of item details */
	TArray<struct FEnvQueryItemDetails> ItemDetails;

	/** number of valid items on list */
	int32 NumValidItems;

	/** size of current value */
	uint16 ValueSize;

	/** used to breaking from item iterator loops */
	uint8 bFoundSingleResult : 1;

	/** set when testing final condition of an option */
	uint8 bPassOnSingleResult : 1;

#if WITH_EDITOR
	/** set to true to store additional debug info */
	uint8 bStoreDebugInfo : 1;
#endif // WITH_EDITOR

	/** run mode */
	TEnumAsByte<EEnvQueryRunMode::Type> Mode;

	/** item type's CDO for location tests */
	class UEnvQueryItemType_LocationBase* ItemTypeLocationCDO;

	/** item type's CDO for actor tests */
	class UEnvQueryItemType_ActorBase* ItemTypeActorCDO;

	/** if > 0 then it's how much time query has for performing current step */
	double TimeLimit;

	FEnvQueryInstance() : World(NULL), CurrentTest(-1), NumValidItems(0), bFoundSingleResult(false)
#if WITH_EDITOR
		, bStoreDebugInfo(bDebuggingInfoEnabled) 
#endif // WITH_EDITOR
	{ IncStats(); }
	FEnvQueryInstance(const FEnvQueryInstance& Other) { *this = Other; IncStats(); }
	~FEnvQueryInstance() { DecStats(); }

	/** execute single step of query */
	void ExecuteOneStep(double TimeLimit);

	/** update context cache */
	bool PrepareContext(UClass* Context, FEnvQueryContextData& ContextData);

	/** helpers for reading spatial data from context */
	bool PrepareContext(UClass* Context, TArray<FEnvQuerySpatialData>& Data);
	bool PrepareContext(UClass* Context, TArray<FVector>& Data);
	bool PrepareContext(UClass* Context, TArray<FRotator>& Data);
	/** helpers for reading actor data from context */
	bool PrepareContext(UClass* Context, TArray<AActor*>& Data);

	/** access named params */
	template<typename TEQSParam>
	FORCEINLINE bool GetParamValue(const TEQSParam& Param, typename TEQSParam::FValueType& OutValue, const FString& ParamDesc)
	{
		if (Param.ParamName != NAME_None)
		{
			FNamedParamValueType* PtrValue = NamedParams.Find(Param.ParamName);
			if (PtrValue == NULL)
			{
				EQSHEADERLOG(FString::Printf(TEXT("Query [%s] is missing param [%s] for [%s] property!"), *QueryName, *Param.ParamName.ToString(), *ParamDesc));				
				Status = EEnvQueryStatus::MissingParam;
				return false;
			}

			OutValue = *((typename TEQSParam::FValueType*)PtrValue);
		}
		else
		{
			OutValue = Param.Value;
		}

		return true;
	}

	/** raw data operations */
	void ReserveItemData(int32 NumAdditionalItems);

	template<typename TypeItem, typename TypeValue>
	void AddItemData(TypeValue ItemValue)
	{
		DEC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, RawData.GetAllocatedSize() + Items.GetAllocatedSize());

		const int32 DataOffset = RawData.AddUninitialized(ValueSize);
		TypeItem::SetValue(RawData.GetTypedData() + DataOffset, ItemValue);
		Items.Add(FEnvQueryItem(DataOffset));

		INC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, RawData.GetAllocatedSize() + Items.GetAllocatedSize());
	}

protected:

	/** prepare item data after generator has finished */
	void FinalizeGeneration();

	/** update costs and flags after test has finished */
	void FinalizeTest();
	
	/** final pass on items of finished query */
	void FinalizeQuery();

	/** normalize total score in range 0..1 */
	void NormalizeScores();

	/** sort all scores, from highest to lowest */
	void SortScores();

	/** pick one of items with highest score */
	void PickBestItem();

	/** prepare items on reaching final condition in SingleResult mode */
	void OnFinalCondition();

	/** discard all items but one */
	void PickSingleItem(int32 ItemIndex);

public:

	/** removes all runtime data that can be used for debugging (not a part of actual query result) */
	void StripRedundantData();

#if STATS
	FORCEINLINE void IncStats()
	{
		INC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, GetAllocatedSize());
		INC_DWORD_STAT_BY(STAT_AI_EnvQuery_NumItems, Items.Num());
	}

	FORCEINLINE void DecStats()
	{
		DEC_MEMORY_STAT_BY(STAT_AI_EnvQuery_InstanceMemory, GetAllocatedSize()); 
		DEC_DWORD_STAT_BY(STAT_AI_EnvQuery_NumItems, Items.Num());
	}

	uint32 GetAllocatedSize() const;
	uint32 GetContextAllocatedSize() const;
#else
	FORCEINLINE uint32 GetAllocatedSize() const { return 0; }
	FORCEINLINE uint32 GetContextAllocatedSize() const { return 0; }
	FORCEINLINE void IncStats() {}
	FORCEINLINE void DecStats() {}
#endif // STATS

#if !NO_LOGGING
	void Log(const FString Msg) const;
#endif // #if !NO_LOGGING

#if CPP || UE_BUILD_DOCS
	struct ItemIterator
	{
		ItemIterator(FEnvQueryInstance& QueryInstance) : Instance(&QueryInstance), CurrentItem(QueryInstance.CurrentTestStartingItem)
		{
			Deadline = QueryInstance.TimeLimit > 0.0 ? (FPlatformTime::Seconds() + QueryInstance.TimeLimit) : -1.0;
			InitItemScore();
		}

		~ItemIterator()
		{
			Instance->CurrentTestStartingItem = CurrentItem;
		}

		void SetScore(EEnvTestCondition::Type Condition, float Score, float Threshold)
		{
			if ((Condition == EEnvTestCondition::AtLeast && Score < Threshold) ||
				(Condition == EEnvTestCondition::UpTo && Score > Threshold))
			{
				bPassed = false;
			}
			else
			{
				ItemScore += Score;
				NumPartialScores++;
			}
		}

		void SetScore(EEnvTestCondition::Type Condition, bool bScore, bool bExpected)
		{
			if (Condition == EEnvTestCondition::Match && bScore != bExpected)
			{
				bPassed = false;
			}
			else
			{
				ItemScore += bScore == bExpected ? 1.0f : 0.0f;
				NumPartialScores++;
			}
		}

		void DiscardItem()
		{
			bPassed = false;
		}

		operator bool() const
		{
			return CurrentItem < Instance->Items.Num() && !Instance->bFoundSingleResult && (Deadline < 0 || FPlatformTime::Seconds() < Deadline);
		}

		int32 operator*() const
		{
			return CurrentItem; 
		}

		void operator++()
		{
			StoreTestResult();
			if (!Instance->bFoundSingleResult)
			{
				InitItemScore();
				for (CurrentItem++; CurrentItem < Instance->Items.Num() && !Instance->Items[CurrentItem].IsValid(); CurrentItem++) ;
			}
		}

	protected:

		FEnvQueryInstance* Instance;
		int32 CurrentItem;
		int32 NumPartialScores;
		double Deadline;
		float ItemScore;
		bool bPassed;

		void InitItemScore()
		{
			NumPartialScores = 0;
			ItemScore = 0.0f;
			bPassed = true;
		}

		void StoreTestResult();
	};
#endif

#if WITH_EDITOR
	FEQSQueryDebugData DebugData;
	static bool bDebuggingInfoEnabled;
#endif // WITH_EDITOR
	FBox GetBoundingBox() const;
};

namespace FEQSHelpers
{
#if WITH_RECAST
	const class ARecastNavMesh* FindNavMeshForQuery(struct FEnvQueryInstance& QueryInstance);
#endif // WITH_RECAST
}

UCLASS(Abstract, CustomConstructor)
class ENGINE_API UEnvQueryTypes : public UObject
{
	GENERATED_UCLASS_BODY()

	UEnvQueryTypes(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP) {}
	static FString GetShortTypeName(const UObject* Ob);

	static FString DescribeContext(TSubclassOf<class UEnvQueryContext> ContextClass);
	static FString DescribeIntParam(const FEnvIntParam& Param);
	static FString DescribeFloatParam(const FEnvFloatParam& Param);
	static FString DescribeBoolParam(const FEnvBoolParam& Param);
};
