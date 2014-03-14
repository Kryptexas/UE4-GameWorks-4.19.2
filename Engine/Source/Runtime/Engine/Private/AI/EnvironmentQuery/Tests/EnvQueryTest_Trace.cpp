// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryTest_Trace::UEnvQueryTest_Trace(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ExecuteDelegate.BindUObject(this, &UEnvQueryTest_Trace::RunTest);

	Context = UEnvQueryContext_Querier::StaticClass();
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_LocationBase::StaticClass();
	bWorkOnFloatValues = false;
	TraceToItem.Value = false;
	TraceExtentX.Value = 10.0f;
	TraceExtentY.Value = 10.0f;
	TraceExtentZ.Value = 10.0f;
	TraceComplex.Value = true;
	ItemOffsetZ.Value = 0.0f;
	ContextOffsetZ.Value = 0.0f;
}

void UEnvQueryTest_Trace::RunTest(struct FEnvQueryInstance& QueryInstance)
{
	bool bWantsHit = false;
	bool bTraceToItem = false;
	bool bTraceComplex = true;
	float ExtentX = 0.0f;
	float ExtentY = 0.0f;
	float ExtentZ = 0.0f;
	float ItemZ = 0.0f;
	float ContextZ = 0.0f;

	if (!QueryInstance.GetParamValue(BoolFilter, bWantsHit, TEXT("BoolFilter")) ||
		!QueryInstance.GetParamValue(TraceToItem, bTraceToItem, TEXT("TraceToItem")) ||
		!QueryInstance.GetParamValue(TraceComplex, bTraceComplex, TEXT("TraceComplex")) ||
		!QueryInstance.GetParamValue(TraceExtentX, ExtentX, TEXT("TraceExtentX")) ||
		!QueryInstance.GetParamValue(TraceExtentY, ExtentY, TEXT("TraceExtentY")) ||
		!QueryInstance.GetParamValue(TraceExtentZ, ExtentZ, TEXT("TraceExtentY")) ||
		!QueryInstance.GetParamValue(ItemOffsetZ, ItemZ, TEXT("ItemOffsetZ")) ||
		!QueryInstance.GetParamValue(ContextOffsetZ, ContextZ, TEXT("ContextOffsetZ"))
		)
	{
		return;
	}

	TArray<FVector> ContextLocations;
	if (!QueryInstance.PrepareContext(Context, ContextLocations))
	{
		return;
	}

	FCollisionQueryParams TraceParams(TEXT("EnvQueryTrace"), bTraceComplex);
	TraceParams.bTraceAsyncScene = true;

	TArray<AActor*> IgnoredActors;
	if (QueryInstance.PrepareContext(Context, IgnoredActors))
	{
		TraceParams.AddIgnoredActors(IgnoredActors);
	}
	
	ECollisionChannel TraceCollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceChannel);	
	FRunTraceSignature TraceFunc;
	switch (TraceMode)
	{
	case EEnvTestTrace::Line:
		TraceFunc.BindUObject(this, bTraceToItem ? &UEnvQueryTest_Trace::RunLineTraceTo : &UEnvQueryTest_Trace::RunLineTraceFrom);
		break;

	case EEnvTestTrace::Box:
		TraceFunc.BindUObject(this, bTraceToItem ? &UEnvQueryTest_Trace::RunBoxTraceTo : &UEnvQueryTest_Trace::RunBoxTraceFrom);
		break;

	case EEnvTestTrace::Sphere:
		TraceFunc.BindUObject(this, bTraceToItem ? &UEnvQueryTest_Trace::RunSphereTraceTo : &UEnvQueryTest_Trace::RunSphereTraceFrom);
		break;

	case EEnvTestTrace::Capsule:
		TraceFunc.BindUObject(this, bTraceToItem ? &UEnvQueryTest_Trace::RunCapsuleTraceTo : &UEnvQueryTest_Trace::RunCapsuleTraceFrom);
		break;

	default:
		return;
	}

	for (int32 iContext = 0; iContext < ContextLocations.Num(); iContext++)
	{
		ContextLocations[iContext].Z += ContextZ;
	}

	for (FEnvQueryInstance::ItemIterator It(QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, *It) + FVector(0,0,ItemZ);
		AActor* ItemActor = GetItemActor(QueryInstance, *It);

		for (int32 iContext = 0; iContext < ContextLocations.Num(); iContext++)
		{
			const bool bHit = TraceFunc.Execute(ItemLocation, ContextLocations[iContext], ItemActor, QueryInstance.World, TraceCollisionChannel, TraceParams, FVector(ExtentX, ExtentY, ExtentZ));
			It.SetScore(Condition, bHit, bWantsHit);
		}
	}
}

FString UEnvQueryTest_Trace::GetDescriptionTitle() const
{
	UEnum* ChannelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETraceTypeQuery"), true);
	FString ChannelDesc = ChannelEnum->GetEnumString(TraceChannel);

	FString DirectionDesc = TraceToItem.IsNamedParam() ?
		FString::Printf(TEXT("%s, direction: %s"), *UEnvQueryTypes::DescribeContext(Context), *UEnvQueryTypes::DescribeBoolParam(TraceToItem)) :
		FString::Printf(TEXT("%s %s"), TraceToItem.Value ? TEXT("from") : TEXT("to"), *UEnvQueryTypes::DescribeContext(Context));

	return FString::Printf(TEXT("%s: %s on %s"), 
		*Super::GetDescriptionTitle(), *DirectionDesc, *ChannelDesc);
}

FString UEnvQueryTest_Trace::GetDescriptionDetails() const
{
	FString ShapeDesc = (TraceMode == EEnvTestTrace::Line) ? TEXT("line") :
		(TraceMode == EEnvTestTrace::Sphere) ? FString::Printf(TEXT("sphere (radius: %s)"), *UEnvQueryTypes::DescribeFloatParam(TraceExtentX)) :
		(TraceMode == EEnvTestTrace::Capsule) ? FString::Printf(TEXT("capsule (radius: %s, half height: %s)"),
			*UEnvQueryTypes::DescribeFloatParam(TraceExtentX), *UEnvQueryTypes::DescribeFloatParam(TraceExtentY)) :
		(TraceMode == EEnvTestTrace::Box) ? FString::Printf(TEXT("box (extent: %s %s %s)"),
			*UEnvQueryTypes::DescribeFloatParam(TraceExtentX), *UEnvQueryTypes::DescribeFloatParam(TraceExtentY), *UEnvQueryTypes::DescribeFloatParam(TraceExtentZ)) :
		TEXT("unknown");

	if (TraceComplex.IsNamedParam())
	{
		ShapeDesc += FString::Printf(TEXT(", complex collisions: %s"), *TraceComplex.ParamName.ToString());
	}
	else if (TraceComplex.Value)
	{
		ShapeDesc += TEXT(", complex collisions");
	}

	ShapeDesc.AppendChar(TEXT('\n'));
	ShapeDesc += DescribeBoolTestParams("hit");

	return ShapeDesc;
}

bool UEnvQueryTest_Trace::RunLineTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->LineTraceTest(ContextPos, ItemPos, Channel, TraceParams);
	return bHit;
}

bool UEnvQueryTest_Trace::RunLineTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->LineTraceTest(ItemPos, ContextPos, Channel, TraceParams);
	return bHit;
}

bool UEnvQueryTest_Trace::RunBoxTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->SweepTest(ContextPos, ItemPos, FQuat((ItemPos - ContextPos).Rotation()), Channel, FCollisionShape::MakeBox(Extent), TraceParams);
	return bHit;
}

bool UEnvQueryTest_Trace::RunBoxTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->SweepTest(ItemPos, ContextPos, FQuat((ContextPos - ItemPos).Rotation()), Channel, FCollisionShape::MakeBox(Extent), TraceParams);
	return bHit;
}

bool UEnvQueryTest_Trace::RunSphereTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->SweepTest(ContextPos, ItemPos, FQuat::Identity, Channel, FCollisionShape::MakeSphere(Extent.X), TraceParams);
	return bHit;
}

bool UEnvQueryTest_Trace::RunSphereTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->SweepTest(ItemPos, ContextPos, FQuat::Identity, Channel, FCollisionShape::MakeSphere(Extent.X), TraceParams);
	return bHit;
}

bool UEnvQueryTest_Trace::RunCapsuleTraceTo(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->SweepTest(ContextPos, ItemPos, FQuat::Identity, Channel, FCollisionShape::MakeCapsule(Extent.X, Extent.Z), TraceParams);
	return bHit;
}

bool UEnvQueryTest_Trace::RunCapsuleTraceFrom(const FVector& ItemPos, const FVector& ContextPos, AActor* ItemActor, UWorld* World, enum ECollisionChannel Channel, const FCollisionQueryParams& Params, const FVector& Extent)
{
	FCollisionQueryParams TraceParams(Params);
	TraceParams.AddIgnoredActor(ItemActor);

	const bool bHit = World->SweepTest(ItemPos, ContextPos, FQuat::Identity, Channel, FCollisionShape::MakeCapsule(Extent.X, Extent.Z), TraceParams);
	return bHit;
}
