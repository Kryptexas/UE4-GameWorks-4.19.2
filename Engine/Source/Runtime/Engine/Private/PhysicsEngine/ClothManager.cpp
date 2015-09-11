// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ClothManager.h"
#include "PhysicsPublic.h"
#include "Components/SkeletalMeshComponent.h"

FClothManager::FClothManager(UWorld* AssociatedWorld)
 {
	PrepareClothDataArray[(int32)PrepareClothSchedule::IgnorePhysics].TickFunction.TickGroup = TG_StartPhysics;
	PrepareClothDataArray[(int32)PrepareClothSchedule::WaitOnPhysics].TickFunction.TickGroup = TG_PreCloth;

	for(FClothManagerData& PrepareClothData : PrepareClothDataArray)
	{
		PrepareClothData.TickFunction.bCanEverTick = true;
		PrepareClothData.TickFunction.Target = &PrepareClothData;
		PrepareClothData.TickFunction.bRunOnAnyThread = true;	//this value seems to be ignored
		PrepareClothData.TickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);
	}
 }

void FClothManagerData::PrepareCloth(float DeltaTime)
{
#if WITH_APEX_CLOTHING
	if(SkeletalMeshComponents.Num())
	{
		IsPreparingCloth.AtomicSet(true);
		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{	
			SkeletalMeshComponent->SubmitClothSimulationContext();	//make sure user params are passed internally

			FClothSimulationContext& ClothSimulationContext = SkeletalMeshComponent->InternalClothSimulationContext;
			SkeletalMeshComponent->ParallelTickClothing(DeltaTime, ClothSimulationContext);
		}

		SkeletalMeshComponents.Empty(SkeletalMeshComponents.Num());
		IsPreparingCloth.AtomicSet(false);
	}
#endif
}

void FClothManager::RegisterForPrepareCloth(USkeletalMeshComponent* SkeletalMeshComponent, PrepareClothSchedule PrepSchedule)
{
	check(PrepSchedule != PrepareClothSchedule::MAX);
	PrepareClothDataArray[(int32)PrepSchedule].SkeletalMeshComponents.Add(SkeletalMeshComponent);
}

class FPrepareClothTickTask
{
	FClothManagerData* PrepareClothData;
	float DeltaTime;

public:
	FPrepareClothTickTask(FClothManagerData* InPrepareClothData, float InDeltaTime)
		: PrepareClothData(InPrepareClothData)
		, DeltaTime(InDeltaTime)
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FPrepareClothTickTask, STATGROUP_TaskGraphTasks);
	}
	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::AnyThread;
	}
	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::FireAndForget;
	}

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		PrepareClothData->PrepareCloth(DeltaTime);
	}
};


void FPrepareClothTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	QUICK_SCOPE_CYCLE_COUNTER(FPrepareClothTickFunction_ExecuteTick);

	TGraphTask<FPrepareClothTickTask>::CreateTask().ConstructAndDispatchWhenReady(Target, DeltaTime);
}

FString FPrepareClothTickFunction::DiagnosticMessage()
{
	return TEXT("FPrepareClothTickFunction");
}
