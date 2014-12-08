// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "AI/Navigation/NavigationSystem.h"
#include "AI/Navigation/NavAgentInterface.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Pathfinding.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryTest_Pathfinding::UEnvQueryTest_Pathfinding(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Context = UEnvQueryContext_Querier::StaticClass();
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	TestMode = EEnvTestPathfinding::PathExist;
	PathToItem.Value = true;
	FloatFilter.Value = 1000.0f;
	DiscardUnreachable.Value = true;
	HierarchicalPathfinding.Value = true;
}

void UEnvQueryTest_Pathfinding::RunTest(FEnvQueryInstance& QueryInstance) const
{
	bool bWantsPath = false;
	bool bPathToItem = false;
	bool bHierarchical = false;
	bool bDiscardFailed = false;
// 	float UseThreshold = 0.0f;
	float MinThresholdValue = 0.0f;
	float MaxThresholdValue = 0.0f;

	if (!QueryInstance.GetParamValue(BoolFilter, bWantsPath, TEXT("BoolFilter")) ||
		!QueryInstance.GetParamValue(PathToItem, bPathToItem, TEXT("PathToItem")) ||
		!QueryInstance.GetParamValue(HierarchicalPathfinding, bHierarchical, TEXT("HierarchicalPathfinding")) ||
		!QueryInstance.GetParamValue(DiscardUnreachable, bDiscardFailed, TEXT("DiscardUnreachable")) ||
// 		!QueryInstance.GetParamValue(FloatFilter, UseThreshold, TEXT("FloatFilter")) ||
		!QueryInstance.GetParamValue(FloatFilterMax, MaxThresholdValue, TEXT("FloatFilterMax")) ||
		!QueryInstance.GetParamValue(FloatFilterMin, MinThresholdValue, TEXT("FloatFilterMin"))
	   )

	{
		return;
	}

	UNavigationSystem* NavSys = QueryInstance.World->GetNavigationSystem();
	ANavigationData* NavData = FindNavigationData(NavSys, QueryInstance.Owner.Get());
	if (!NavData)
	{
		return;
	}

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(Context, ContextLocations))
	{
		return;
	}

	EPathFindingMode::Type PFMode(bHierarchical ? EPathFindingMode::Hierarchical : EPathFindingMode::Regular);

	if (GetWorkOnFloatValues())
	{
		FFindPathSignature FindPathFunc;
		FindPathFunc.BindUObject(this, TestMode == EEnvTestPathfinding::PathLength ?
			(bPathToItem ? &UEnvQueryTest_Pathfinding::FindPathLengthTo : &UEnvQueryTest_Pathfinding::FindPathLengthFrom) :
			(bPathToItem ? &UEnvQueryTest_Pathfinding::FindPathCostTo : &UEnvQueryTest_Pathfinding::FindPathCostFrom) );

		NavData->BeginBatchQuery();
		for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
		{
			const FVector ItemLocation = GetItemLocation(QueryInstance, *It);
			for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
			{
				const float PathValue = FindPathFunc.Execute(ItemLocation, ContextLocations[ContextIndex], PFMode, NavData, NavSys, QueryInstance.Owner.Get());
				It.SetScore(TestPurpose, FilterType, PathValue, MinThresholdValue, MaxThresholdValue);

				if (bDiscardFailed && PathValue >= BIG_NUMBER)
				{
					It.DiscardItem();
				}
			}
		}
		NavData->FinishBatchQuery();
	}
	else
	{
		NavData->BeginBatchQuery();
		if (bPathToItem)
		{
			for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
			{
				const FVector ItemLocation = GetItemLocation(QueryInstance, *It);
				for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
				{
					const bool bFoundPath = TestPathTo(ItemLocation, ContextLocations[ContextIndex], PFMode, NavData, NavSys, QueryInstance.Owner.Get());
					It.SetScore(TestPurpose, FilterType, bFoundPath, bWantsPath);
				}
			}
		}
		else
		{
			for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
			{
				const FVector ItemLocation = GetItemLocation(QueryInstance, *It);
				for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
				{
					const bool bFoundPath = TestPathFrom(ItemLocation, ContextLocations[ContextIndex], PFMode, NavData, NavSys, QueryInstance.Owner.Get());
					It.SetScore(TestPurpose, FilterType, bFoundPath, bWantsPath);
				}
			}
		}
		NavData->FinishBatchQuery();
	}
}

FString UEnvQueryTest_Pathfinding::GetDescriptionTitle() const
{
	FString ModeDesc[] = { TEXT("PathExist"), TEXT("PathCost"), TEXT("PathLength") };

	FString DirectionDesc = PathToItem.IsNamedParam() ?
		FString::Printf(TEXT("%s, direction: %s"), *UEnvQueryTypes::DescribeContext(Context).ToString(), *UEnvQueryTypes::DescribeBoolParam(PathToItem)) :
		FString::Printf(TEXT("%s %s"), PathToItem.Value ? TEXT("from") : TEXT("to"), *UEnvQueryTypes::DescribeContext(Context).ToString());

	return FString::Printf(TEXT("%s: %s"), *ModeDesc[TestMode], *DirectionDesc);
}

FText UEnvQueryTest_Pathfinding::GetDescriptionDetails() const
{
	FText AdditionalDesc;
	if (HierarchicalPathfinding.IsNamedParam())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("PathName"), FText::FromString(HierarchicalPathfinding.ParamName.ToString()));
		AdditionalDesc = FText::Format(LOCTEXT("HierarchicalPathfindingWithName", "Hierarchical pathfinding: {PathName}"), Args);
	}
	else if (HierarchicalPathfinding.Value)
	{
		AdditionalDesc = LOCTEXT("UseHierarchicalPathfinding", "Use hierarchical pathfinding");
	}

	if (DiscardUnreachable.IsNamedParam())
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("Description"), AdditionalDesc);
		Args.Add(TEXT("UnreachableParamName"), FText::FromName(DiscardUnreachable.ParamName));

		if(AdditionalDesc.IsEmpty())
		{
			AdditionalDesc = FText::Format(LOCTEXT("DiscardUnreachableWithParam", "Discard unreachable: {UnreachableParamName}"), Args);
		}
		else
		{
			AdditionalDesc = FText::Format(LOCTEXT("DescWithUnreachableAndParam", "{Description}, discard unreachable: {UnreachableParamName}"), Args);
		}
	}
	else if (DiscardUnreachable.Value)
	{
		if(AdditionalDesc.IsEmpty())
		{
			AdditionalDesc = LOCTEXT("DiscardUnreachable", "Discard unreachable");
		}
		else
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("Description"), AdditionalDesc);
			AdditionalDesc = FText::Format(LOCTEXT("DescWithUnreachable", "{Description}, discard unreachable"), Args);
		}
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("Description"), AdditionalDesc);

	if (GetWorkOnFloatValues())
	{
		Args.Add(TEXT("FloatParamsDescription"), DescribeFloatTestParams());
		AdditionalDesc = FText::Format(LOCTEXT("DescriptionWithDescribedFloatParams", "{Description}\n{FloatParamsDescription}"), Args);
	}
	else
	{
		Args.Add(TEXT("BoolParamsDescription"), DescribeBoolTestParams("existing path"));
		AdditionalDesc = FText::Format(LOCTEXT("DescriptionWithDescribedBoolParams", "{Description}\n{BoolParamsDescription}"), Args);
	}

	return AdditionalDesc;
}

#if WITH_EDITOR
void UEnvQueryTest_Pathfinding::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UEnvQueryTest_Pathfinding,TestMode))
	{
		SetWorkOnFloatValues(TestMode != EEnvTestPathfinding::PathExist);
		Condition = EEnvTestCondition::NoCondition;
	}
}
#endif

void UEnvQueryTest_Pathfinding::PostLoad()
{
	Super::PostLoad();
	
	SetWorkOnFloatValues(TestMode != EEnvTestPathfinding::PathExist);
}

bool UEnvQueryTest_Pathfinding::TestPathFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner) const
{
	const bool bPathExists = NavSys->TestPathSync(FPathFindingQuery(PathOwner, NavData, ItemPos, ContextPos), Mode);
	return bPathExists;
}

bool UEnvQueryTest_Pathfinding::TestPathTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner) const
{
	const bool bPathExists = NavSys->TestPathSync(FPathFindingQuery(PathOwner, NavData, ContextPos, ItemPos), Mode);
	return bPathExists;
}

float UEnvQueryTest_Pathfinding::FindPathCostFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner) const
{
	FPathFindingResult Result = NavSys->FindPathSync(FPathFindingQuery(PathOwner, NavData, ItemPos, ContextPos), Mode);
	return (Result.IsSuccessful()) ? Result.Path->GetCost() : BIG_NUMBER;
}

float UEnvQueryTest_Pathfinding::FindPathCostTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner) const
{
	FPathFindingResult Result = NavSys->FindPathSync(FPathFindingQuery(PathOwner, NavData, ContextPos, ItemPos), Mode);
	return (Result.IsSuccessful()) ? Result.Path->GetCost() : BIG_NUMBER;
}

float UEnvQueryTest_Pathfinding::FindPathLengthFrom(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner) const
{
	FPathFindingResult Result = NavSys->FindPathSync(FPathFindingQuery(PathOwner, NavData, ItemPos, ContextPos), Mode);
	return (Result.IsSuccessful()) ? Result.Path->GetLength() : BIG_NUMBER;
}

float UEnvQueryTest_Pathfinding::FindPathLengthTo(const FVector& ItemPos, const FVector& ContextPos, EPathFindingMode::Type Mode, const ANavigationData* NavData, UNavigationSystem* NavSys, const UObject* PathOwner) const
{
	FPathFindingResult Result = NavSys->FindPathSync(FPathFindingQuery(PathOwner, NavData, ContextPos, ItemPos), Mode);
	return (Result.IsSuccessful()) ? Result.Path->GetLength() : BIG_NUMBER;
}

ANavigationData* UEnvQueryTest_Pathfinding::FindNavigationData(UNavigationSystem* NavSys, UObject* Owner) const
{
	INavAgentInterface* NavAgent = Cast<INavAgentInterface>(Owner);
	if (NavAgent)
	{
		return NavSys->GetNavDataForProps(NavAgent->GetNavAgentProperties());
	}

	return NavSys->GetMainNavData(FNavigationSystem::DontCreate);
}

#undef LOCTEXT_NAMESPACE
