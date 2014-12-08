// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AIModulePrivate.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_VectorBase.h"
#include "EnvironmentQuery/Tests/EnvQueryTest_Trace.h"

#define LOCTEXT_NAMESPACE "EnvQueryGenerator"

UEnvQueryTest_Trace::UEnvQueryTest_Trace(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	Cost = EEnvTestCost::High;
	ValidItemType = UEnvQueryItemType_VectorBase::StaticClass();
	SetWorkOnFloatValues(false);
	
	Context = UEnvQueryContext_Querier::StaticClass();
	TraceToItem.Value = false;
	ItemOffsetZ.Value = 0.0f;
	ContextOffsetZ.Value = 0.0f;

	TraceData.SetGeometryOnly();
}

void UEnvQueryTest_Trace::RunTest(FEnvQueryInstance& QueryInstance) const
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

	for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
	{
		ContextLocations[ContextIndex].Z += ContextZ;
	}

	for (FEnvQueryInstance::ItemIterator It(this, QueryInstance); It; ++It)
	{
		const FVector ItemLocation = GetItemLocation(QueryInstance, *It) + FVector(0,0,ItemZ);
		AActor* ItemActor = GetItemActor(QueryInstance, *It);

		for (int32 ContextIndex = 0; ContextIndex < ContextLocations.Num(); ContextIndex++)
		{
			const bool bHit = TraceFunc.Execute(ItemLocation, ContextLocations[ContextIndex], ItemActor, QueryInstance.World, TraceCollisionChannel, TraceParams, TraceExtent);
			It.SetScore(TestPurpose, FilterType, bHit, bWantsHit);
		}
	}
}

FString UEnvQueryTest_Trace::GetDescriptionTitle() const
{
	UEnum* ChannelEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("ETraceTypeQuery"), true);
	FString ChannelDesc = ChannelEnum->GetEnumText(TraceData.TraceChannel).ToString();

	FString DirectionDesc = TraceToItem.IsNamedParam() ?
		FString::Printf(TEXT("%s, direction: %s"), *UEnvQueryTypes::DescribeContext(Context).ToString(), *UEnvQueryTypes::DescribeBoolParam(TraceToItem)) :
		FString::Printf(TEXT("%s %s"), TraceToItem.Value ? TEXT("from") : TEXT("to"), *UEnvQueryTypes::DescribeContext(Context).ToString());

	return FString::Printf(TEXT("%s: %s on %s"), 
		*Super::GetDescriptionTitle(), *DirectionDesc, *ChannelDesc);
}

FText UEnvQueryTest_Trace::GetDescriptionDetails() const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("TraceData"),  TraceData.ToText(FEnvTraceData::Detailed));
	Args.Add(TEXT("TestParams"),  DescribeBoolTestParams("hit"));
	
	return LOCTEXT("TraceDescription", "{TraceData}\n{TestParams}");
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

#undef LOCTEXT_NAMESPACE
