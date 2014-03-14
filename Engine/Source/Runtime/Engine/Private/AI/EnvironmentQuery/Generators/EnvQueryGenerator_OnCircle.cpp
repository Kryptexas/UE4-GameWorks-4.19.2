// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryGenerator_OnCircle::UEnvQueryGenerator_OnCircle(const class FPostConstructInitializeProperties& PCIP) 
	: Super(PCIP)
{
	GenerateDelegate.BindUObject(this, &UEnvQueryGenerator_OnCircle::GenerateItems);

	CircleCenter = UEnvQueryContext_Querier::StaticClass();
	ItemType = UEnvQueryItemType_Point::StaticClass();
	Radius.Value = 1000.0f;
	ItemSpacing.Value = 50.0f;
	ArcDirection.DirMode = EEnvDirection::TwoPoints;
	ArcDirection.LineFrom = UEnvQueryContext_Querier::StaticClass();
	ArcDirection.Rotation = UEnvQueryContext_Querier::StaticClass();
	Angle.Value = 360.f;
	AngleRadians = FMath::DegreesToRadians(360.f);

	TraceData.bCanProjectDown = false;
	TraceData.bCanDisableTrace = false;
	TraceData.TraceMode = EEnvQueryTrace::Navigation;

	ProjectionData.TraceMode = EEnvQueryTrace::None;
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
	FVector Direction = Querier->GetActorRotation().Vector();

	if (bDefineArc)
	{
		if (ArcDirection.DirMode == EEnvDirection::TwoPoints)
		{
			TArray<FVector> Start;
			TArray<FVector> End;
			QueryInstance.PrepareContext(ArcDirection.LineFrom, Start);
			QueryInstance.PrepareContext(ArcDirection.LineTo, End);

			if (Start.Num() > 0 && End.Num() > 0)
			{
				Direction = (End[0] - Start[0]).SafeNormal();
			}
			else
			{
				UE_VLOG(Querier, LogEQS, Warning, TEXT("UEnvQueryGenerator_OnCircle::CalcDirection failed to calc direction in %s. Using querier facing."), *QueryInstance.QueryName);
			}
		}
		else
		{
			TArray<FRotator> Rot;
			QueryInstance.PrepareContext(ArcDirection.Rotation, Rot);

			if (Rot.Num() > 0)
			{
				Direction = Rot[0].Vector();
			}
			else
			{
				UE_VLOG(Querier, LogEQS, Warning, TEXT("UEnvQueryGenerator_OnCircle::CalcDirection failed to calc direction in %s. Using querier facing."), *QueryInstance.QueryName);
			}
		}
	}

	return Direction;
}

void UEnvQueryGenerator_OnCircle::GenerateItems(FEnvQueryInstance& QueryInstance)
{
	float AngleDegree = 360.f;
	float RadiusValue = 0.f;
	float ItemSpace = 1.0f;

	if (QueryInstance.GetParamValue(Angle, AngleDegree, TEXT("Angle")) == false
		|| QueryInstance.GetParamValue(Radius, RadiusValue, TEXT("Radius")) == false
		|| QueryInstance.GetParamValue(ItemSpacing, ItemSpace, TEXT("ItemSpacing")) == false
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
	const int32 StepsCount = FMath::Ceil(ArcLength / ItemSpace) + 1;
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

#if WITH_RECAST
	// @todo this needs to be optimize to batch raycasts
	const ARecastNavMesh* NavMesh = 
		(TraceData.TraceMode == EEnvQueryTrace::Navigation) || (ProjectionData.TraceMode == EEnvQueryTrace::Navigation) ?
		FEQSHelpers::FindNavMeshForQuery(QueryInstance) : NULL;

	if (NavMesh)
	{
		NavMesh->BeginBatchQuery();
	}
#endif

	if (TraceData.TraceMode == EEnvQueryTrace::Navigation)
	{
#if WITH_RECAST
		TSharedPtr<const FNavigationQueryFilter> NavigationFilter = UNavigationQueryFilter::GetQueryFilter(NavMesh, TraceData.NavigationFilter);

		for (int32 Step = 0; Step < StepsCount; ++Step)
		{
			NavMesh->Raycast(CenterLocation, ItemCandidates[Step], ItemCandidates[Step], NavigationFilter);
		}
#endif
	}
	else
	{
		FRunTraceSignature TraceFunc;
		switch (TraceData.TraceShape)
		{
		case EEnvTraceShape::Line:		TraceFunc.BindUObject(this, &UEnvQueryGenerator_OnCircle::RunLineTrace); break;
		case EEnvTraceShape::Sphere:	TraceFunc.BindUObject(this, &UEnvQueryGenerator_OnCircle::RunSphereTrace); break;
		case EEnvTraceShape::Capsule:	TraceFunc.BindUObject(this, &UEnvQueryGenerator_OnCircle::RunCapsuleTrace); break;
		case EEnvTraceShape::Box:		TraceFunc.BindUObject(this, &UEnvQueryGenerator_OnCircle::RunBoxTrace); break;
		default: break;
		}

		if (TraceFunc.IsBound())
		{
			ECollisionChannel TraceCollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceData.TraceChannel);	
			FVector TraceExtent(TraceData.ExtentX, TraceData.ExtentY, TraceData.ExtentZ);

			FCollisionQueryParams TraceParams(TEXT("EnvQueryTrace"), TraceData.bTraceComplex);
			TraceParams.bTraceAsyncScene = true;

			for (int32 Step = 0; Step < StepsCount; ++Step)
			{
				TraceFunc.Execute(CenterLocation, ItemCandidates[Step], QueryInstance.World, TraceCollisionChannel, TraceParams, TraceExtent, ItemCandidates[Step]);
			}
		}
	}

#if WITH_RECAST
	if (NavMesh)
	{
		ProjectAndFilterNavPoints(ItemCandidates, NavMesh);
		NavMesh->FinishBatchQuery();
	}
#endif

	for (int32 Step = 0; Step < ItemCandidates.Num(); ++Step)
	{
		QueryInstance.AddItemData<UEnvQueryItemType_Point>(ItemCandidates[Step]);
	}
}

void UEnvQueryGenerator_OnCircle::RunLineTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos)
{
	FHitResult OutHit;
	const bool bHit = World->LineTraceSingle(OutHit, StartPos, EndPos, Channel, Params);
	HitPos = bHit ? OutHit.ImpactPoint : EndPos;
};

void UEnvQueryGenerator_OnCircle::RunSphereTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos)
{
	FHitResult OutHit;
	const bool bHit = World->SweepSingle(OutHit, StartPos, EndPos, FQuat::Identity, Channel, FCollisionShape::MakeSphere(Extent.X), Params);
	HitPos = bHit ? OutHit.ImpactPoint : EndPos;
};

void UEnvQueryGenerator_OnCircle::RunCapsuleTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos)
{
	FHitResult OutHit;
	const bool bHit = World->SweepSingle(OutHit, StartPos, EndPos, FQuat::Identity, Channel, FCollisionShape::MakeCapsule(Extent.X, Extent.Z), Params);
	HitPos = bHit ? OutHit.ImpactPoint : EndPos;
};

void UEnvQueryGenerator_OnCircle::RunBoxTrace(const FVector& StartPos, const FVector& EndPos, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent, FVector& HitPos)
{
	FHitResult OutHit;
	const bool bHit = World->SweepSingle(OutHit, StartPos, EndPos, FQuat((EndPos - StartPos).Rotation()), Channel, FCollisionShape::MakeBox(Extent), Params);
	HitPos = bHit ? OutHit.ImpactPoint : EndPos;
};

FString UEnvQueryGenerator_OnCircle::GetDescriptionTitle() const
{
	return FString::Printf(TEXT("%s: generate items on circle around %s"),
		*Super::GetDescriptionTitle(), *UEnvQueryTypes::DescribeContext(CircleCenter));
}

FString UEnvQueryGenerator_OnCircle::GetDescriptionDetails() const
{
	FString Desc = FString::Printf(TEXT("radius: %s, item span: %.2f\n%s"),
		*UEnvQueryTypes::DescribeFloatParam(Radius), *UEnvQueryTypes::DescribeFloatParam(ItemSpacing),
		*TraceData.ToString(FEnvTraceData::Detailed));

	if (bDefineArc)
	{
		Desc += FString::Printf(TEXT("\nLimit to %.2f angle both sides on %s"), Angle.Value, *ArcDirection.ToString());
	}

	FString ProjDesc = ProjectionData.ToString(FEnvTraceData::Brief);
	if (ProjDesc.Len() > 0)
	{
		Desc += TEXT(", ");
		Desc += ProjDesc;
	}

	return Desc;
}

#if WITH_EDITOR

void UEnvQueryGenerator_OnCircle::PostEditChangeProperty( struct FPropertyChangedEvent& PropertyChangedEvent) 
{
	static const FName NAME_Angle = GET_MEMBER_NAME_CHECKED(UEnvQueryGenerator_OnCircle, Angle);
	static const FName NAME_Radius = GET_MEMBER_NAME_CHECKED(UEnvQueryGenerator_OnCircle, Radius);

	if (PropertyChangedEvent.Property != NULL)
	{
		const FName PropName = PropertyChangedEvent.MemberProperty->GetFName();
		if (PropName == NAME_Angle)
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

		ItemSpacing.Value = FMath::Max(1.0f, ItemSpacing.Value);
	}

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif // WITH_EDITOR