// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "../../EnvironmentQuery/EnvQueryTypes.h"
#include "BTTask_RunEQSQuery.generated.h"

struct FBTEnvQueryTaskMemory
{
	/** Query request ID */
	int32 RequestID;
};

UCLASS(MinimalAPI)
class UBTTask_RunEQSQuery : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	/** query to run */
	UPROPERTY(Category=Node, EditAnywhere)
	class UEnvQuery* QueryTemplate;

	/** optional parameters for query */
	UPROPERTY(Category=Node, EditAnywhere)
	TArray<struct FEnvNamedValue> QueryParams;

	virtual EBTNodeResult::Type ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;
	virtual EBTNodeResult::Type AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const OVERRIDE;

	virtual void DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const OVERRIDE;
	virtual FString GetStaticDescription() const OVERRIDE;
	virtual uint16 GetInstanceMemorySize() const OVERRIDE;

	/** finish task */
	void OnQueryFinished(TSharedPtr<struct FEnvQueryResult> Result);

#if WITH_EDITOR
	/** prepare query params */
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;
#endif

protected:

	/** gather all filters from existing EnvQueryItemTypes */
	void CollectKeyFilters();
};
