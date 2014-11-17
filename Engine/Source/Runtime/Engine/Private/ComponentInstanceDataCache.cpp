// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"

#include "Components/ApplicationLifecycleComponent.h"
#include "AI/BehaviorTree/BlackboardComponent.h"
#include "AI/BrainComponent.h"
#include "AI/BehaviorTree/BehaviorTreeComponent.h"
#include "Debug/GameplayDebuggingControllerComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/MovementComponent.h"
#include "Components/SceneComponent.h"
#include "Components/LightComponentBase.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "GameFramework/NavMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/SpectatorPawnMovement.h"
#include "Vehicles/WheeledVehicleMovementComponent.h"
#include "Vehicles/WheeledVehicleMovementComponent4W.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/RotatingMovementComponent.h"
#include "AI/Navigation/NavigationComponent.h"
#include "AI/Navigation/PathFollowingComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "Components/PawnSensingComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Atmosphere/AtmosphericFogComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "AI/Navigation/NavigationGraphNodeComponent.h"
#include "PhysicsEngine/PhysicsConstraintComponent.h"
#include "PhysicsEngine/PhysicsThrusterComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BrushComponent.h"
#include "Components/DrawFrustumComponent.h"
#include "AI/EnvironmentQuery/EQSRenderingComponent.h"
#include "Debug/GameplayDebuggingComponent.h"
#include "Components/LineBatchComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/MeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "Components/DestructibleComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/InteractiveFoliageComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/ModelComponent.h"
#include "AI/Navigation/NavLinkRenderingComponent.h"
#include "AI/Navigation/NavMeshRenderingComponent.h"
#include "AI/Navigation/NavTestRenderingComponent.h"
#include "Components/NiagaraComponent.h"
#include "Components/ShapeComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/DrawSphereComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/VectorFieldComponent.h"
#include "PhysicsEngine/RadialForceComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/WindDirectionalSourceComponent.h"
#include "Components/TimelineComponent.h"
#include "Slate.h"
#include "AI/NavDataGenerator.h"
#include "OnlineSubsystemUtils.h"

void FComponentInstanceDataCache::AddInstanceData(TSharedPtr<FComponentInstanceDataBase> NewData)
{
	check(NewData.IsValid());
	FName TypeName = NewData->GetDataTypeName();
	TypeToDataMap.Add(TypeName, NewData);
}

void FComponentInstanceDataCache::GetInstanceDataOfType(FName TypeName, TArray< TSharedPtr<FComponentInstanceDataBase> >& OutData) const
{
	TypeToDataMap.MultiFind(TypeName, OutData);
}


void FComponentInstanceDataCache::GetFromActor(AActor* Actor, FComponentInstanceDataCache& Cache)
{
	if(Actor != NULL)
	{
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		// Grab per-instance data we want to persist
		for (int32 CompIdx=0; CompIdx<Components.Num(); CompIdx++)
		{
			UActorComponent* Component = Components[CompIdx];
			if(Component->bCreatedByConstructionScript) // Only cache data from 'created by construction script' components
			{
				Component->GetComponentInstanceData(Cache);
			}
		}
	}
}

void FComponentInstanceDataCache::ApplyToActor(AActor* Actor)
{
	if(Actor != NULL)
	{
		TArray<UActorComponent*> Components;
		Actor->GetComponents(Components);

		// Apply per-instance data.
		for (int32 CompIdx=0; CompIdx<Components.Num(); CompIdx++)
		{
			UActorComponent* Component = Components[CompIdx];
			if(Component->bCreatedByConstructionScript) // Only try and apply data to 'created by construction script' components
			{
				Component->ApplyComponentInstanceData(*this);
			}
		}
	}
}