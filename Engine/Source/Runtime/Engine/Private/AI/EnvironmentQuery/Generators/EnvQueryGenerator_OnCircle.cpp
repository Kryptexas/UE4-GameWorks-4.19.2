// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryGenerator_OnCircle::UEnvQueryGenerator_OnCircle(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
{
	GenerateDelegate.BindUObject(this, &UEnvQueryGenerator_OnCircle::GenerateItems);

	CircleCenter = UEnvQueryContext_Querier::StaticClass();
	ItemType = UEnvQueryItemType_Point::StaticClass();
	Radius.Value = 1000.0f;
	ItemSpacing = 50.0f;
	ArcDirectionStart = UEnvQueryContext_Querier::StaticClass();
	Angle.Value = 360.f;
	AngleRadians = FMath::DegreesToRadians(360.f);
}

void UEnvQueryGenerator_OnCircle::PostLoad()
{
	Super::PostLoad();

	if (Angle.Value <= 0)
	{
		Angle.Value = 360.f;
	}
	AngleRadians = FMath::DegreesToRadians(Angle.Value);
}

FVector UEnvQueryGenerator_OnCircle::CalcDirection(FEnvQueryInstance& QueryInstance) const
{
	AActor* Querier = Cast<AActor>(QueryInstance.Owner.Get());

	if (bDefineArc)
	{
		TArray<FVector> Start;
		TArray<FVector> End;
		QueryInstance.PrepareContext(ArcDirectionStart, Start);
		QueryInstance.PrepareContext(ArcDirectionEnd, End);

		if (Start.Num() > 0)
		{
			if (End.Num() > 0)
			{
				return (End[0] - Start[0]).SafeNormal();
			}
			else
			{
				TArray<AActor*> StartActors;
				QueryInstance.PrepareContext(ArcDirectionStart, StartActors);
				if (StartActors.Num() > 0)
				{
					return StartActors[0]->GetActorRotation().Vector();
				}
				else
				{
					UE_VLOG(Querier, LogEQS, Warning, TEXT("UEnvQueryGenerator_OnCircle::CalcDirection failed to generate direction actor for %s. Using Querier."), *QueryInstance.QueryName);
				}
			}
		}
		else
		{
			UE_VLOG(Querier, LogEQS, Warning, TEXT("UEnvQueryGenerator_OnCircle::CalcDirection failed to calc direction in %s. Using querier facing."), *QueryInstance.QueryName);
		}
	}
	
	return Querier->GetActorRotation().Vector();
}

void UEnvQueryGenerator_OnCircle::GenerateItems(FEnvQueryInstance& QueryInstance)
{
	float AngleDegree = 360.f;
	float RadiusValue = 0.f;

	if (QueryInstance.GetParamValue(Angle, AngleDegree, TEXT("Angle")) == false
		|| QueryInstance.GetParamValue(Radius, RadiusValue, TEXT("Radius")) == false
		|| AngleDegree <= 0.f || AngleDegree > 360.f
		|| RadiusValue <= 0.f)
	{
		return;
	}

	AngleRadians = FMath::DegreesToRadians(Angle.Value);

	// first generate points on a circle
	const float CircumferenceLength = 2.f * PI * RadiusValue;
	const float ArcAnglePercentage = Angle.Value / 360.f;
	const float ArcLength = CircumferenceLength * ArcAnglePercentage;
	const int32 StepsCount = FMath::Ceil(ArcLength / ItemSpacing) + 1;
	const float AngleStep = AngleDegree / (StepsCount - 1);

	FVector StartDirection = CalcDirection(QueryInstance);
	TArray<FVector> CenterLocationCandidates;
	QueryInstance.PrepareContext(CircleCenter, CenterLocationCandidates);

	FVector CenterLocation(0);
	if (CenterLocationCandidates.Num() > 0)
	{
		CenterLocation = CenterLocationCandidates[0];
	}
	else
	{
		AActor* Querier = Cast<AActor>(QueryInstance.Owner.Get());
		if (Querier)
		{
			CenterLocation = Querier->GetActorLocation();
		}
	}

	StartDirection = StartDirection.RotateAngleAxis(-AngleDegree/2, FVector::UpVector) * RadiusValue;

	TArray<FVector> ItemCandidates;
	ItemCandidates.AddZeroed(StepsCount);
	for (int32 Step = 0; Step < StepsCount; ++Step)
	{
		ItemCandidates[Step] = CenterLocation + StartDirection.RotateAngleAxis(AngleStep*Step, FVector::UpVector);
	}

	if (TraceType == EEnvQueryTrace::Navigation)
	{
		// @todo this needs to be optimize to batch raycasts
		const ARecastNavMesh* NavMesh = FEQSHelpers::FindNavMeshForQuery(QueryInstance);
		
		TSharedPtr<const FNavigationQueryFilter> NavigationFilter = UNavigationQueryFilter::GetQueryFilter(NavigationFilterClass);

		for (int32 Step = 0; Step < StepsCount; ++Step)
		{
			NavMesh->Raycast(CenterLocation, ItemCandidates[Step], ItemCandidates[Step], NavigationFilter);
		}
	}

	for (int32 Step = 0; Step < StepsCount; ++Step)
	{
		QueryInstance.AddItemData<UEnvQueryItemType_Point>(ItemCandidates[Step]);
	}

#if WITH_RECAST
	//const ARecastNavMesh* NavMesh = FEQSHelpers::FindNavMeshForQuery(QueryInstance);
	//if (NavMesh == NULL) return;

	//float PathDistanceValue = 0.0f;
	//float DensityValue = 0.0f;
	//bool bFromContextValue = true;

	//if (! ||
	//	!QueryInstance.GetParamValue(Density, DensityValue, TEXT("Density")) ||
	//	!QueryInstance.GetParamValue(PathFromContext, bFromContextValue, TEXT("PathFromContext")))
	//{
	//	return;
	//}

	//const int32 nItems = FPlatformMath::Trunc((PathDistanceValue * 2.0f / DensityValue) + 1);
	//const int32 nItemsHalf = nItems / 2;

	//TArray<FVector> ContextLocations;
	//QueryInstance.PrepareContext(GenerateAround, ContextLocations);
	//QueryInstance.ReserveItemData(nItemsHalf * nItemsHalf * ContextLocations.Num());

	//TArray<NavNodeRef> NavNodeRefs;
	//NavMesh->BeginBatchQuery();

	//int32 DataOffset = 0;
	//for (int32 i = 0; i < ContextLocations.Num(); i++)
	//{
	//	// find all node refs in pathing distance
	//	FBox AllowedBounds;
	//	NavNodeRefs.Reset();
	//	FindNodeRefsInPathDistance(NavMesh, ContextLocations[i], PathDistanceValue, bFromContextValue, NavNodeRefs, AllowedBounds);

	//	// cast 2D grid on generated node refs
	//	for (int32 iX = 0; iX < nItems; ++iX)
	//	{
	//		for (int32 iY = 0; iY < nItems; ++iY)
	//		{
	//			const FVector TestPoint = ContextLocations[i] - FVector(DensityValue * (iX - nItemsHalf), DensityValue * (iY - nItemsHalf), 0);
	//			if (!AllowedBounds.IsInsideXY(TestPoint))
	//			{
	//				continue;
	//			}

	//			// trace line on navmesh, and process all hits with collected node refs
	//			TArray<FNavLocation> Hits;
	//			NavMesh->ProjectPointMulti(TestPoint, Hits, FVector::ZeroVector, AllowedBounds.Min.Z, AllowedBounds.Max.Z);

	//			for (int32 i = 0; i < Hits.Num(); i++)
	//			{
	//				if (IsNavLocationInPathDistance(NavMesh, Hits[i], NavNodeRefs))
	//				{
	//					// store generated point
	//					QueryInstance.AddItemData<UEnvQueryItemType_Point>(Hits[i].Location);
	//				}
	//			}
	//		}
	//	}
	//}

	//NavMesh->FinishBatchQuery();
#endif // WITH_RECAST
}

FString UEnvQueryGenerator_OnCircle::GetDescriptionTitle() const
{
	return FString::Printf(TEXT("%s: generate items on circle around %s"),
		*Super::GetDescriptionTitle(), *UEnvQueryTypes::DescribeContext(CircleCenter));
}

FString UEnvQueryGenerator_OnCircle::GetDescriptionDetails() const
{
	static const UEnum* EnvQueryTraceEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("EEnvQueryTrace"));

	FString RaycastDesc = bUseNavigationRaycast ? FString::Printf(TEXT("\nTraceType %s"), *EnvQueryTraceEnum->GetEnumName(TraceType)) : TEXT("");
	FString ArcDesc = bDefineArc ? FString::Printf(TEXT("\nLimit to %.2f angle both sides on line %s-%s"), Angle.Value, *UEnvQueryTypes::DescribeContext(ArcDirectionStart), *UEnvQueryTypes::DescribeContext(ArcDirectionEnd)) 
		: TEXT("");

	return FString::Printf(TEXT("radius: %s, item span: %.2f%s%s")
		, *UEnvQueryTypes::DescribeFloatParam(Radius)
		, ItemSpacing
		, *RaycastDesc
		, *ArcDesc);
}

#if WITH_EDITOR

void UEnvQueryGenerator_OnCircle::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) 
{
	static const FName NAME_TraceType = GET_MEMBER_NAME_CHECKED(UEnvQueryGenerator_OnCircle, TraceType);
	static const FName NAME_Angle = GET_MEMBER_NAME_CHECKED(UEnvQueryGenerator_OnCircle, Angle);
	static const FName NAME_Radius = GET_MEMBER_NAME_CHECKED(UEnvQueryGenerator_OnCircle, Radius);
	static const FName NAME_ItemSpacing = GET_MEMBER_NAME_CHECKED(UEnvQueryGenerator_OnCircle, ItemSpacing);

	if (PropertyChangedEvent.Property != NULL)
	{
		const FName PropName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropName == NAME_TraceType)
		{
			bUseNavigationRaycast = TraceType == EEnvQueryTrace::Navigation;
		}
		else if (PropName == NAME_Angle)
		{
			Angle.Value = FMath::Clamp(Angle.Value, 0.0f, 360.f);
			AngleRadians = FMath::DegreesToRadians(Angle.Value);
			bDefineArc = Angle.Value < 360.f && Angle.Value > 0.f;
		}
		else if (PropName == NAME_Radius)
		{
			if (Radius.Value <= 0.f)
			{
				Radius.Value = 100.f;
			}
		}
		else if (PropName == NAME_ItemSpacing)
		{
			if (ItemSpacing <= 1.f)
			{
				ItemSpacing = 1.f;
			}
		}
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // WITH_EDITOR