// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UBTTask_RunEQSQuery::UBTTask_RunEQSQuery(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	NodeName = "Run EQS Query";

	// filter with keys that have matching env query values, only for instances
	// CDO won't be able to access types from game dlls
	if (GIsEditor && !HasAnyFlags(RF_ClassDefaultObject))
	{
		CollectKeyFilters();
	}
}

EBTNodeResult::Type UBTTask_RunEQSQuery::ExecuteTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	AActor* QueryOwner = OwnerComp->GetOwner();
	if (AController* ControllerOwner = Cast<AController>(QueryOwner))
	{
		QueryOwner = ControllerOwner->GetPawn();
	}

	FEnvQueryRequest QueryRequest(QueryTemplate, QueryOwner);
	QueryRequest.SetNamedParams(QueryParams);

	FBTEnvQueryTaskMemory* MyMemory = (FBTEnvQueryTaskMemory*)NodeMemory;
	MyMemory->RequestID = QueryRequest.Execute(EEnvQueryRunMode::SingleResult, this, &UBTTask_RunEQSQuery::OnQueryFinished);

	const bool bValid = (MyMemory->RequestID >= 0);
	if (bValid)
	{
		WaitForMessage(OwnerComp, UBrainComponent::AIMessage_QueryFinished, MyMemory->RequestID);
		return EBTNodeResult::InProgress;
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_RunEQSQuery::AbortTask(class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory) const
{
	UWorld* MyWorld = OwnerComp->GetWorld();
	UEnvQueryManager* QueryManager = MyWorld->GetEnvironmentQueryManager();
	if (QueryManager)
	{
		FBTEnvQueryTaskMemory* MyMemory = (FBTEnvQueryTaskMemory*)NodeMemory;
		QueryManager->AbortQuery(MyMemory->RequestID);
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_RunEQSQuery::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s: %s\nBlackboard key: %s"), *Super::GetStaticDescription(),
		*GetNameSafe(QueryTemplate), *BlackboardKey.SelectedKeyName.ToString());
}

void UBTTask_RunEQSQuery::DescribeRuntimeValues(const class UBehaviorTreeComponent* OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	if (Verbosity == EBTDescriptionVerbosity::Detailed)
	{
		FBTEnvQueryTaskMemory* MyMemory = (FBTEnvQueryTaskMemory*)NodeMemory;
		Values.Add(FString::Printf(TEXT("request: %d"), MyMemory->RequestID));
	}
}

uint16 UBTTask_RunEQSQuery::GetInstanceMemorySize() const
{
	return sizeof(FBTEnvQueryTaskMemory);
}

void UBTTask_RunEQSQuery::OnQueryFinished(TSharedPtr<struct FEnvQueryResult> Result)
{
	if (Result->Status == EEnvQueryStatus::Aborted)
	{
		return;
	}

	AActor* MyOwner = Cast<AActor>(Result->Owner.Get());
	if (APawn* PawnOwner = Cast<APawn>(MyOwner))
	{
		MyOwner = PawnOwner->GetController();
	}

	UBehaviorTreeComponent* MyComp = MyOwner ? MyOwner->FindComponentByClass<UBehaviorTreeComponent>() : NULL;
	if (!MyComp)
	{
		UE_LOG(LogBehaviorTree, Warning, TEXT("Unable to find behavior tree to notify about finished query from %s!"), *GetNameSafe(MyOwner));
		return;
	}

	bool bSuccess = (Result->Items.Num() == 1);
	if (bSuccess)
	{
		UBlackboardComponent* MyBlackboard = MyComp->GetBlackboardComponent();
		UEnvQueryItemType* ItemTypeCDO = (UEnvQueryItemType*)Result->ItemType->GetDefaultObject();

		bSuccess = ItemTypeCDO->StoreInBlackboard(BlackboardKey, MyBlackboard, Result->RawData.GetTypedData() + Result->Items[0].DataOffset);		
		if (!bSuccess)
		{
			UE_VLOG(MyOwner, LogBehaviorTree, Warning, TEXT("Failed to store query result! item:%s key:%s"),
				*UEnvQueryTypes::GetShortTypeName(Result->ItemType),
				*UBehaviorTreeTypes::GetShortTypeName(BlackboardKey.SelectedKeyType));
		}
	}

	FAIMessage::Send(MyComp, FAIMessage(UBrainComponent::AIMessage_QueryFinished, this, Result->QueryID, bSuccess));
}

void UBTTask_RunEQSQuery::CollectKeyFilters()
{
	for (int32 i = 0; i < UEnvQueryManager::RegisteredItemTypes.Num(); i++)
	{
		UEnvQueryItemType* ItemTypeCDO = (UEnvQueryItemType*)UEnvQueryManager::RegisteredItemTypes[i]->GetDefaultObject();
		ItemTypeCDO->AddBlackboardFilters(BlackboardKey, this);
	}
}

#if WITH_EDITOR
void UBTTask_RunEQSQuery::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UBTTask_RunEQSQuery, QueryTemplate))
	{
		if (QueryTemplate)
		{
			QueryTemplate->CollectQueryParams(QueryParams);
		}
	}
}
#endif
