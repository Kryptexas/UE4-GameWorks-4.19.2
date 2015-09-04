// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ClothManager.h"
#include "PhysicsPublic.h"
#include "Components/SkeletalMeshComponent.h"

FClothManager::FClothManager(UWorld* AssociatedWorld, FPhysScene* InPhysScene)
 : PhysScene(InPhysScene) 
 {
	ClothManagerTickFunction.bCanEverTick = true;
	ClothManagerTickFunction.Target = this;
	ClothManagerTickFunction.TickGroup = TG_StartPhysics;
	ClothManagerTickFunction.bRunOnAnyThread = true;	//this value seems to be ignored
	ClothManagerTickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);
 }

void FClothManager::Tick(float DeltaTime)
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
		PhysScene->StartCloth();
		IsPreparingCloth.AtomicSet(false);
	}
#endif
}

void FClothManager::RegisterForClothThisFrame(USkeletalMeshComponent* SkeletalMeshComponent)
{
	SkeletalMeshComponents.Add(SkeletalMeshComponent);
}


class FClothManagerTickTask
{
	FClothManager* ClothManager;
	float DeltaTime;

public:
	FClothManagerTickTask(FClothManager* InClothManager, float InDeltaTime)
		: ClothManager(InClothManager)
		, DeltaTime(InDeltaTime)
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FClothManagerTickTask, STATGROUP_TaskGraphTasks);
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
		ClothManager->Tick(DeltaTime);
	}
};


void FClothManagerTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	QUICK_SCOPE_CYCLE_COUNTER(FClothManagerTickFunction_ExecuteTick);

	TGraphTask<FClothManagerTickTask>::CreateTask().ConstructAndDispatchWhenReady(Target, DeltaTime);
}

FString FClothManagerTickFunction::DiagnosticMessage()
{
	return TEXT("FClothManagerTickFunction");
}
