// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

UEnvQueryTest_Trace::UEnvQueryTest_Trace(const class FPostConstructInitializeProperties& PCIP) : Super(PCIP)
{
	ExecuteDelegate.BindUObject(this, &UEnvQueryTest_Trace::RunTest);

	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	bWorkOnFloatValues = false;
	
	Context = UEnvQueryContext_Querier::StaticClass();
	TraceToItem.Value = false;
	ItemOffsetZ.Value = 0.0f;
	ContextOffsetZ.Value = 0.0f;

	TraceData.SetGeometryOnly();
}

void UEnvQueryTest_Trace::RunTest(struct FEnvQueryInstance& QueryInstance)
{
	bool bWantsHit = false;
	bool bTraceToItem = false;
	float ItemZ = 0.0f;
	float ContextZ = 0.0f;

	if (!QueryInstance.GetParamValue(BoolFilter, bWantsHit, TEXT("BoolFilter")) ||
		!QueryInstance.GetParamValue(TraceToItem, bTraceToItem, TEXT("TraceToItem")) ||
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

	FCollisionQueryParams TraceParams(TEXT("EnvQueryTrace"), TraceData.bTraceComplex);
	TraceParams.bTraceAsyncScene = true;

	TArray<AActor*> IgnoredActors;
	if (QueryInstance.PrepareContext(Context, IgnoredActors))
	{
		TraceParams.AddIgnoredActors(IgnoredActors);
	}
	
	ECollisionChannel TraceCollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceData.TraceChannel);	
	FVector TraceExtent(TraceData.ExtentX, TraceData.ExtentY, TraceData.ExtentZ);
	FRunTraceSignature TraceFunc;
	switch (TraceData.TraceShape)
	{
	case EEnvTraceShape::Line:
		TraceFunc.BindUObject(this, bTraceToItem ? &UEnvQueryTest_Trace::RunLineTraceTo : &UEnvQueryTest_Trace::RunLineTraceFrom);
		break;

	case EEnvTraceShape::Box:
		TraceFunc.BindUObject(this, bTraceToItem ? &UEnvQueryTest_Trace::RunBoxTraceTo : &UEnvQueryTest_Trace::RunBoxTraceFrom);
		break;

	case EEnvTraceShape::Sphere:
		TraceFunc.BindUObject(this, bTraceToItem ? &UEnvQueryTest_Trace::RunSphereTraceTo : &UEnvQueryTest_Trace::RunSphereTraceFrom);
		break;

	case EEnvTraceShape::Capsule:
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
			const bool bHit = TraceFunc.Execute(ItemLocation, ContextLocations[iContext], ItemActor, QueryInstance.World, TraceCollisionChannel, TraceParams, TraceExtent);
			It.SetScore(Condition, bHit, bWantsHit);
		}
	}
}

FString UEnvQueryTest_Trace::GetDescriptionTitle() const
{
	UEnum* ChannelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETraceTypeQuery"), true);
	FString ChannelDesc = ChannelEnum->GetEnumText(TraceData.TraceChannel).ToString();

	FString DirectionDesc = TraceToItem.IsNamedParam() ?
		FString::Printf(TEXT("%s, direction: %s"), *UEnvQueryTypes::DescribeContext(Context), *UEnvQueryTypes::DescribeBoolParam(TraceToItem)) :
		FString::Printf(TEXT("%s %s"), TraceToItem.Value ? TEXT("from") : TEXT("to"), *UEnvQueryTypes::DescribeContext(Context));

	return FString::Printf(TEXT("%s: %s on %s"), 
		*Super::GetDescriptionTitle(), *DirectionDesc, *ChannelDesc);
}

FString UEnvQueryTest_Trace::GetDescriptionDetails() const
{
	FString Desc;

	Desc = TraceData.ToString(FEnvTraceData::Detailed);
	Desc.AppendChar(TEXT('\n'));
	Desc += DescribeBoolTestParams("hit");

	return Desc;
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
