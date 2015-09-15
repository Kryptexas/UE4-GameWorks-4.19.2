// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "ClothManager.h"
#include "PhysicsPublic.h"
#include "Components/SkeletalMeshComponent.h"

FClothManager::FClothManager(UWorld* InAssociatedWorld)
: AssociatedWorld(InAssociatedWorld)
{
}

void FClothManagerData::PrepareCloth(float DeltaTime)
{
#if WITH_APEX_CLOTHING
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FClothManager_PrepareCloth);
	if (SkeletalMeshComponents.Num())
	{
		IsPreparingCloth.AtomicSet(true);
		for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
		{
			SkeletalMeshComponent->SubmitClothSimulationContext();	//make sure user params are passed internally

			FClothSimulationContext& ClothSimulationContext = SkeletalMeshComponent->InternalClothSimulationContext;
			SkeletalMeshComponent->ParallelTickClothing(DeltaTime, ClothSimulationContext);
		}

		IsPreparingCloth.AtomicSet(false);
	}
#endif
}

void FClothManager::RegisterForPrepareCloth(USkeletalMeshComponent* SkeletalMeshComponent, PrepareClothSchedule PrepSchedule)
{
	check(PrepSchedule != PrepareClothSchedule::MAX);
	PrepareClothDataArray[(int32)PrepSchedule].SkeletalMeshComponents.Add(SkeletalMeshComponent);
}

void FClothManager::StartCloth(float DeltaTime)
{
	//First we must prepare any cloth that waits on physics sim
	PrepareClothDataArray[(int32)PrepareClothSchedule::WaitOnPhysics].PrepareCloth(DeltaTime);

	//Reset skeletal mesh components
	bool bNeedSimulateCloth = false;
	for (FClothManagerData& PrepareData : PrepareClothDataArray)
	{
		if (PrepareData.SkeletalMeshComponents.Num())
		{
			bNeedSimulateCloth = true;
			PrepareData.SkeletalMeshComponents.Reset();
		}
		PrepareData.PrepareCompletion.SafeRelease();
	}

	if (bNeedSimulateCloth)
	{
		if (FPhysScene* PhysScene = AssociatedWorld->GetPhysicsScene())
		{
			PhysScene->StartCloth();
		}
	}
}

static TAutoConsoleVariable<int32> CVarParallelCloth(TEXT("p.ParallelCloth"), 0, TEXT("Whether to prepare and start cloth sim off the game thread"));

void FClothManager::SetupClothTickFunction(bool bTickClothSim)
{
	check(IsInGameThread());
	check(!IsPreparingClothAsync());

	bool bTickOnAny = !!CVarParallelCloth.GetValueOnGameThread();
	StartIgnorePhysicsClothTickFunction.bRunOnAnyThread = bTickOnAny;
	StartClothTickFunction.bRunOnAnyThread = bTickOnAny;

	/** Need to register tick function and we are not currently registered */
	if (bTickClothSim && !StartClothTickFunction.IsTickFunctionRegistered())
	{
		//prepare cloth data ticks
		StartIgnorePhysicsClothTickFunction.TickGroup = TG_StartPhysics;
		StartIgnorePhysicsClothTickFunction.Target = this;
		StartIgnorePhysicsClothTickFunction.bCanEverTick = true;
		StartIgnorePhysicsClothTickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);

		//simulate cloth tick
		StartClothTickFunction.TickGroup = TG_StartCloth;
		StartClothTickFunction.Target = this;
		StartClothTickFunction.bCanEverTick = true;
		StartClothTickFunction.AddPrerequisite(AssociatedWorld, StartIgnorePhysicsClothTickFunction);
		StartClothTickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);

		//wait for cloth sim to finish
		EndClothTickFunction.TickGroup = TG_EndCloth;
		EndClothTickFunction.Target = this;
		EndClothTickFunction.bCanEverTick = true;
		EndClothTickFunction.bRunOnAnyThread = false;	//this HAS to run on game thread because we need to block if cloth sim is not done
		EndClothTickFunction.AddPrerequisite(AssociatedWorld, StartClothTickFunction);
		EndClothTickFunction.RegisterTickFunction(AssociatedWorld->PersistentLevel);
	}
	else if (!bTickClothSim && StartClothTickFunction.IsTickFunctionRegistered())	//Need to unregister tick function and we current are registered
	{
		StartIgnorePhysicsClothTickFunction.UnRegisterTickFunction();
		StartClothTickFunction.UnRegisterTickFunction();
		EndClothTickFunction.UnRegisterTickFunction();
	}
}


void FClothManager::EndCloth()
{
	QUICK_SCOPE_CYCLE_COUNTER(FClothManager_ExecuteTick);
	if (FPhysScene* PhysScene = AssociatedWorld->GetPhysicsScene())
	{
		PhysScene->WaitClothScene();
	}
}

void FStartIgnorePhysicsClothTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	Target->PrepareClothDataArray[(int32)PrepareClothSchedule::IgnorePhysics].PrepareCloth(DeltaTime);
}

FString FStartIgnorePhysicsClothTickFunction::DiagnosticMessage()
{
	return TEXT("FStartIgnorePhysicsClothTickFunction");
}

void FStartClothTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	Target->StartCloth(DeltaTime);
}

FString FStartClothTickFunction::DiagnosticMessage()
{
	return TEXT("FStartClothTickFunction");
}


void FEndClothTickFunction::ExecuteTick(float DeltaTime, enum ELevelTick TickType, ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	Target->EndCloth();
}

FString FEndClothTickFunction::DiagnosticMessage()
{
	return TEXT("FEndClothTickFunction");
}