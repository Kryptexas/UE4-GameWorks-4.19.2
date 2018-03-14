// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ClothingSimulationInteractorNv.h"

#include "ClothingSimulationNv.h"

void UClothingSimulationInteractorNv::PhysicsAssetUpdated()
{
#if WITH_NVCLOTH
	Commands.Add(NvInteractorCommand::CreateLambda([](FClothingSimulationNv* InSimulation, FClothingSimulationContextNv* InContext)
	{
		InSimulation->RefreshPhysicsAsset();
	}));

	MarkDirty();
#endif
}

void UClothingSimulationInteractorNv::ClothConfigUpdated()
{
#if WITH_NVCLOTH
	Commands.Add(NvInteractorCommand::CreateLambda([](FClothingSimulationNv* InSimulation, FClothingSimulationContextNv* InContext)
	{
		InSimulation->RefreshClothConfig();
	}));

	MarkDirty();
#endif
}

void UClothingSimulationInteractorNv::Sync(IClothingSimulation* InSimulation, IClothingSimulationContext* InContext)
{
#if WITH_NVCLOTH
	check(InSimulation);
	check(InContext);

	FClothingSimulationNv* NvSim = static_cast<FClothingSimulationNv*>(InSimulation);
	FClothingSimulationContextNv* NvContext = static_cast<FClothingSimulationContextNv*>(InContext);

	for(NvInteractorCommand& Command : Commands)
	{
		Command.Execute(NvSim, NvContext);
	}
	Commands.Reset();
#endif
}

void UClothingSimulationInteractorNv::SetAnimDriveSpringStiffness(float InStiffness)
{
#if WITH_NVCLOTH
	Commands.Add(NvInteractorCommand::CreateLambda([InStiffness](FClothingSimulationNv* InSimulation, FClothingSimulationContextNv* InContext)
	{
		InSimulation->ExecutePerActor([InStiffness](FClothingActorNv& Actor)
		{
			Actor.CurrentAnimDriveSpringStiffness = InStiffness;
		});
	}));

	MarkDirty();
#endif
}

void UClothingSimulationInteractorNv::SetAnimDriveDamperStiffness(float InStiffness)
{
#if WITH_NVCLOTH
	Commands.Add(NvInteractorCommand::CreateLambda([InStiffness](FClothingSimulationNv* InSimulation, FClothingSimulationContextNv* InContext)
	{
		InSimulation->ExecutePerActor([InStiffness](FClothingActorNv& Actor)
		{
			Actor.CurrentAnimDriveDamperStiffness = InStiffness;
		});
	}));

	MarkDirty();
#endif
}
void UClothingSimulationInteractorNv::EnableGravityOverride(const FVector& InVector)
{
#if WITH_NVCLOTH
	Commands.Add(NvInteractorCommand::CreateLambda([InVector](FClothingSimulationNv* InSimulation, FClothingSimulationContextNv* InContext)
	{
		InSimulation->ExecutePerActor([InVector](FClothingActorNv& Actor)
		{
			Actor.bUseGravityOverride = true;
			Actor.GravityOverride = InVector;
		});
	}));

	MarkDirty();
#endif
}

void UClothingSimulationInteractorNv::DisableGravityOverride()
{
#if WITH_NVCLOTH
	Commands.Add(NvInteractorCommand::CreateLambda([](FClothingSimulationNv* InSimulation, FClothingSimulationContextNv* InContext)
	{
		InSimulation->ExecutePerActor([](FClothingActorNv& Actor)
		{
			Actor.bUseGravityOverride = false;
			Actor.GravityOverride = FVector(0.0f, 0.0f, 0.0f);
		});
	}));

	MarkDirty();
#endif
}

