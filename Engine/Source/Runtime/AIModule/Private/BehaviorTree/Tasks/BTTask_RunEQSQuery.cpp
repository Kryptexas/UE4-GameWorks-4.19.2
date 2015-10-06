// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "BehaviorTree/Tasks/BTTask_RunEQSQuery.h"
#include "EnvironmentQuery/Items/EnvQueryItemType.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"

UBTTask_RunEQSQuery::UBTTask_RunEQSQuery(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Run EQS Query";

	// filter with keys that have matching env query values, only for instances
	// CDO won't be able to access types from game dlls
	if (GIsEditor && !HasAnyFlags(RF_ClassDefaultObject))
	{
		CollectKeyFilters();
	}

	EQSQueryBlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_RunEQSQuery, EQSQueryBlackboardKey), UEnvQuery::StaticClass());
}

void UBTTask_RunEQSQuery::InitializeFromAsset(UBehaviorTree& Asset)
{
	Super::InitializeFromAsset(Asset);

	if (QueryConfig.Num() > 0 || bUseBBKey)
	{
		UBlackboardData* BBAsset = GetBlackboardAsset();
		if (BBAsset)
		{
			for (FAIDynamicParam& RuntimeParam : QueryConfig)
			{
				if (RuntimeParam.BBKey.NeedsResolving())
				{
					RuntimeParam.BBKey.ResolveSelectedKey(*BBAsset);
				}
			}

			EQSQueryBlackboardKey.ResolveSelectedKey(*BBAsset);
		}
	}
}

EBTNodeResult::Type UBTTask_RunEQSQuery::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AActor* QueryOwner = OwnerComp.GetOwner();
	AController* ControllerOwner = Cast<AController>(QueryOwner);
	if (ControllerOwner)
	{
		QueryOwner = ControllerOwner->GetPawn();
	}

	if (bUseBBKey)
	{
		const UBlackboardComponent* BlackboardComponent = OwnerComp.GetBlackboardComponent();
		check(BlackboardComponent);
		
		// set QueryTemplate to contents of BB key
		UObject* QueryTemplateObject = BlackboardComponent->GetValue<UBlackboardKeyType_Object>(EQSQueryBlackboardKey.GetSelectedKeyID());
		QueryTemplate = Cast<UEnvQuery>(QueryTemplateObject);

		UE_CVLOG(QueryTemplate == nullptr, QueryOwner, LogBehaviorTree, Warning, TEXT("%s configured to use BB key, but indicated key (%s) doesn't contain EQS template pointer")
			, *GetName(), *EQSQueryBlackboardKey.SelectedKeyName.ToString());
	}

	if (QueryTemplate != nullptr)
	{
		FEnvQueryRequest QueryRequest(QueryTemplate, QueryOwner);
		if (QueryConfig.Num() > 0)
		{
			UBlackboardComponent* BBComp = OwnerComp.GetBlackboardComponent();

			// resolve 
			for (FAIDynamicParam& RuntimeParam : QueryConfig)
			{
				// check if given param requires runtime resolve, like reading from BB
				if (RuntimeParam.BBKey.IsSet())
				{
					check(BBComp && "If BBKey.IsSet and there's no BB component then we\'re in the error land!");

					// grab info from BB
					switch (RuntimeParam.ParamType)
					{
					case EAIParamType::Float:
					{
						const float Value = BBComp->GetValue<UBlackboardKeyType_Float>(RuntimeParam.BBKey.GetSelectedKeyID());
						QueryRequest.SetFloatParam(RuntimeParam.ParamName, Value);
					}
					break;
					case EAIParamType::Int:
					{
						const int32 Value = BBComp->GetValue<UBlackboardKeyType_Int>(RuntimeParam.BBKey.GetSelectedKeyID());
						QueryRequest.SetIntParam(RuntimeParam.ParamName, Value);
					}
					break;
					case EAIParamType::Bool:
					{
						const bool Value = BBComp->GetValue<UBlackboardKeyType_Bool>(RuntimeParam.BBKey.GetSelectedKeyID());
						QueryRequest.SetBoolParam(RuntimeParam.ParamName, Value);
					}
					break;
					default:
						break;
					}

				}
				else
				{
					QueryRequest.SetFloatParam(RuntimeParam.ParamName, RuntimeParam.Value);
				}
			}
		}

		FBTEnvQueryTaskMemory* MyMemory = (FBTEnvQueryTaskMemory*)NodeMemory;
		MyMemory->RequestID = QueryRequest.Execute(RunMode, this, &UBTTask_RunEQSQuery::OnQueryFinished);

		const bool bValid = (MyMemory->RequestID >= 0);
		if (bValid)
		{
			WaitForMessage(OwnerComp, UBrainComponent::AIMessage_QueryFinished, MyMemory->RequestID);
			return EBTNodeResult::InProgress;
		}
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_RunEQSQuery::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UWorld* MyWorld = OwnerComp.GetWorld();
	UEnvQueryManager* QueryManager = UEnvQueryManager::GetCurrent(MyWorld);
	
	if (QueryManager)
	{
		FBTEnvQueryTaskMemory* MyMemory = (FBTEnvQueryTaskMemory*)NodeMemory;
		QueryManager->AbortQuery(MyMemory->RequestID);
	}

	return EBTNodeResult::Aborted;
}

FString UBTTask_RunEQSQuery::GetStaticDescription() const
{
	return bUseBBKey ? FString::Printf(TEXT("%s: EQS query indicated by %s blackboard key\nResult Blackboard key: %s"), *Super::GetStaticDescription(), *EQSQueryBlackboardKey.SelectedKeyName.ToString(), *BlackboardKey.SelectedKeyName.ToString())
		: FString::Printf(TEXT("%s: %s\nResult Blackboard key: %s"), *Super::GetStaticDescription(), *GetNameSafe(QueryTemplate), *BlackboardKey.SelectedKeyName.ToString());
}

void UBTTask_RunEQSQuery::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
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

void UBTTask_RunEQSQuery::OnQueryFinished(TSharedPtr<FEnvQueryResult> Result)
{
	if (Result->IsAborted())
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

	bool bSuccess = (Result->Items.Num() >= 1);
	if (bSuccess)
	{
		UBlackboardComponent* MyBlackboard = MyComp->GetBlackboardComponent();
		UEnvQueryItemType* ItemTypeCDO = (UEnvQueryItemType*)Result->ItemType->GetDefaultObject();

		bSuccess = ItemTypeCDO->StoreInBlackboard(BlackboardKey, MyBlackboard, Result->RawData.GetData() + Result->Items[0].DataOffset);		
		if (!bSuccess)
		{
			UE_VLOG(MyOwner, LogBehaviorTree, Warning, TEXT("Failed to store query result! item:%s key:%s"),
				*UEnvQueryTypes::GetShortTypeName(Result->ItemType).ToString(),
				*UBehaviorTreeTypes::GetShortTypeName(BlackboardKey.SelectedKeyType));
		}
	}

	FAIMessage::Send(MyComp, FAIMessage(UBrainComponent::AIMessage_QueryFinished, this, Result->QueryID, bSuccess));
}

void UBTTask_RunEQSQuery::CollectKeyFilters()
{
// 	for (int32 TypeIndex = 0; TypeIndex < UEnvQueryManager::RegisteredItemTypes.Num(); TypeIndex++)
// 	{
// 		UEnvQueryItemType* ItemTypeCDO = (UEnvQueryItemType*)UEnvQueryManager::RegisteredItemTypes[TypeIndex]->GetDefaultObject();
// 		ItemTypeCDO->AddBlackboardFilters(BlackboardKey, this);
// 	}
}

void UBTTask_RunEQSQuery::PostLoad()
{
	Super::PostLoad();

	if (QueryParams.Num() > 0)
	{
		FAIDynamicParam::GenerateConfigurableParamsFromNamedValues(*this, QueryConfig, QueryParams);
		QueryParams.Empty();
	}
}

#if WITH_EDITOR
void UBTTask_RunEQSQuery::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property &&
		PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UBTTask_RunEQSQuery, QueryTemplate))
	{
		if (QueryTemplate)
		{
			QueryTemplate->CollectQueryParams(*this, QueryConfig);
		}
		else
		{
			QueryConfig.Reset();
		}
	}
}

FName UBTTask_RunEQSQuery::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.RunEQSQuery.Icon");
}

#endif
