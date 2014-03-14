// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "EnvQueryTypes.h"
#include "EnvQueryManager.generated.h"

/** wrapper for easy query execution */
USTRUCT()
struct ENGINE_API FEnvQueryRequest
{
	GENERATED_USTRUCT_BODY()

	FEnvQueryRequest() : QueryTemplate(NULL), Owner(NULL), World(NULL) {}

	// basic constructor: owner will be taken from finish delegate bindings
	FEnvQueryRequest(class UEnvQuery* Query) : QueryTemplate(Query), Owner(NULL), World(NULL) {}

	// use when owner is different from finish delegate binding
	FEnvQueryRequest(class UEnvQuery* Query, UObject* RequestOwner) : QueryTemplate(Query), Owner(RequestOwner), World(NULL) {}

	// set named params
	FORCEINLINE FEnvQueryRequest& SetFloatParam(FName ParamName, float Value) { NamedParams.Add(ParamName, Value); return *this; }
	FORCEINLINE FEnvQueryRequest& SetIntParam(FName ParamName, int32 Value) { NamedParams.Add(ParamName, *((float*)&Value)); return *this; }
	FORCEINLINE FEnvQueryRequest& SetBoolParam(FName ParamName, bool Value) { NamedParams.Add(ParamName, Value ? 1.0f : -1.0f); return *this; }
	FORCEINLINE FEnvQueryRequest& SetNamedParam(const struct FEnvNamedValue& ParamData) { NamedParams.Add(ParamData.ParamName, ParamData.Value); return *this; }
	FEnvQueryRequest& SetNamedParams(const TArray<struct FEnvNamedValue>& Params);

	// set world (for accessing query manager) when owner can't provide it
	FORCEINLINE FEnvQueryRequest& SetWorldOverride(UWorld* InWorld) { World = InWorld; return *this; }

	template< class UserClass >	
	FORCEINLINE int32 Execute(EEnvQueryRunMode::Type Mode, UserClass* InObj, typename FQueryFinishedSignature::TUObjectMethodDelegate< UserClass >::FMethodPtr InMethod)
	{
		return Execute(Mode, FQueryFinishedSignature::CreateUObject(InObj, InMethod));
	}
	template< class UserClass >	
	FORCEINLINE int32 Execute(EEnvQueryRunMode::Type Mode, UserClass* InObj, typename FQueryFinishedSignature::TUObjectMethodDelegate_Const< UserClass >::FMethodPtr InMethod)
	{
		return Execute(Mode, FQueryFinishedSignature::CreateUObject(InObj, InMethod));
	}
	int32 Execute(EEnvQueryRunMode::Type RunMode, FQueryFinishedSignature const& FinishDelegate);

protected:

	/** query to run */
	UPROPERTY()
	class UEnvQuery* QueryTemplate;

	/** querier */
	UPROPERTY()
	UObject* Owner;

	/** world */
	UPROPERTY()
	UWorld* World;

	/** list of named params */
	TMap<FName, float> NamedParams;

	friend class UEnvQueryManager;
};

/** cache of instances with sorted tests */
struct FEnvQueryInstanceCache
{
	/** query template */
	TWeakObjectPtr<class UEnvQuery> Template;

	/** instance to duplicate */
	struct FEnvQueryInstance Instance;
};

#if WITH_EDITOR
struct FEQSDebugger
{
	void StoreQuery(TSharedPtr<FEnvQueryInstance>& Query);
	TSharedPtr<FEnvQueryInstance> GetQueryForOwner(const UObject* Owner);

protected:
	// maps owner to performed queries
	TMap<const UObject*, TSharedPtr<FEnvQueryInstance> > StoredQueries;
};
#endif // WITH_EDITOR

UCLASS()
class ENGINE_API UEnvQueryManager : public UObject, public FTickableGameObject
{
	GENERATED_UCLASS_BODY()

	/** [FTickableGameObject] tick function */
	virtual void Tick(float DeltaTime) OVERRIDE;

	/** [FTickableGameObject] always tick */
	virtual bool IsTickable() const OVERRIDE { return true; }

	/** [FTickableGameObject] tick stats */
	virtual TStatId GetStatId() const OVERRIDE;

	/** execute query */
	int32 RunQuery(const struct FEnvQueryRequest& Request, EEnvQueryRunMode::Type RunMode, FQueryFinishedSignature const& FinishDelegate);

	/** alternative way to run queries. Do not use for anything other then testing! (worse performance) */
	TSharedPtr<FEnvQueryResult> RunInstantQuery(const struct FEnvQueryRequest& Request, EEnvQueryRunMode::Type RunMode);

	/** Creates a query instance configured for execution */
	TSharedPtr<struct FEnvQueryInstance> PrepareQueryInstance(const struct FEnvQueryRequest& Request, EEnvQueryRunMode::Type RunMode);

	/** execute query */
	bool AbortQuery(int32 RequestID);

	/** fail all running queries on changing persistent map */
	virtual void OnPreLoadMap();

	/** cleanup hooks for map loading */
	virtual void FinishDestroy() OVERRIDE;

	/** list of all known item types */
	static TArray<TSubclassOf<UEnvQueryItemType> > RegisteredItemTypes;

#if WITH_EDITOR
	static void NotifyAssetUpdate(UEnvQuery* Query);

	FEQSDebugger& GetDebugger() { return EQSDebugger; }

protected:
	FEQSDebugger EQSDebugger;
#endif // WITH_EDITOR

protected:

	/** currently running queries */
	TArray<TSharedPtr<struct FEnvQueryInstance> > RunningQueries;

	/** cache of instances */
	TArray<FEnvQueryInstanceCache> InstanceCache;

	/** next ID for running query */
	int32 NextQueryID;

	/** create new instance, using cached data is possible */
	TSharedPtr<struct FEnvQueryInstance> CreateQueryInstance(class UEnvQuery* Template, EEnvQueryRunMode::Type RunMode);

private:

	/** create and bind delegates in instance */
	void CreateOptionInstance(class UEnvQueryOption* OptionTemplate, const TArray<class UEnvQueryTest*>& SortedTests, struct FEnvQueryInstance& Instance);
};
