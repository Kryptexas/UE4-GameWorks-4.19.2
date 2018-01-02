// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "ApexDestructionCustomPayload.h"
#include "DestructibleComponent.h"
#include "PhysXPublic.h"
#include "ApexDestructionModule.h"
#include "AI/Navigation/NavigationSystem.h"

#if WITH_APEX

void FApexDestructionSyncActors::BuildSyncData_AssumesLocked(int32 SceneType, const TArray<physx::PxRigidActor*>& ActiveActors)
{
	//We want to consolidate the transforms so that we update each destructible component once by passing it an array of chunks to update.
	//This helps avoid a lot of duplicated work like marking render dirty, computing inverse world component, etc...

	//prepare map to update destructible components
	TArray<PxShape*> Shapes;
	for (const PxRigidActor* RigidActor : ActiveActors)
	{
		if (const FApexDestructionCustomPayload* DestructibleChunkInfo = ((FApexDestructionCustomPayload*)FPhysxUserData::Get<FCustomPhysXPayload>(RigidActor->userData)))
		{
			if (GApexModuleDestructible->owns(RigidActor) && DestructibleChunkInfo->OwningComponent.IsValid())
			{
				Shapes.AddUninitialized(RigidActor->getNbShapes());
				int32 NumShapes = RigidActor->getShapes(Shapes.GetData(), Shapes.Num());
				for (int32 ShapeIdx = 0; ShapeIdx < Shapes.Num(); ++ShapeIdx)
				{
					PxShape* Shape = Shapes[ShapeIdx];
					int32 ChunkIndex;
					if (apex::DestructibleActor* DestructibleActor = GApexModuleDestructible->getDestructibleAndChunk(Shape, &ChunkIndex))
					{
						const physx::PxMat44 ChunkPoseRT = DestructibleActor->getChunkPose(ChunkIndex);
						const physx::PxTransform Transform(ChunkPoseRT);
						if (UDestructibleComponent* DestructibleComponent = Cast<UDestructibleComponent>(FPhysxUserData::Get<UPrimitiveComponent>(DestructibleActor->userData)))
						{
							if (DestructibleComponent->IsRegistered())
							{
								TArray<FUpdateChunksInfo>& UpdateInfos = ComponentUpdateMapping.FindOrAdd(DestructibleComponent);
								FUpdateChunksInfo* UpdateInfo = new (UpdateInfos)FUpdateChunksInfo(ChunkIndex, P2UTransform(Transform));
							}
						}
					}
				}

				Shapes.Empty(Shapes.Num());	//we want to keep largest capacity array to avoid reallocs
			}
		}
	}

}

void FApexDestructionSyncActors::FinalizeSync(int32 SceneType)
{
	//update each component
	for (auto It = ComponentUpdateMapping.CreateIterator(); It; ++It)
	{
		if (UDestructibleComponent* DestructibleComponent = It.Key().Get())
		{
			TArray<FUpdateChunksInfo>& UpdateInfos = It.Value();
			if (DestructibleComponent->IsFracturedOrInitiallyStatic())
			{
				DestructibleComponent->SetChunksWorldTM(UpdateInfos);
			}
			else
			{
				//if we haven't fractured it must mean that we're simulating a destructible and so we should update our GetComponentTransform() based on the single rigid body
				DestructibleComponent->SyncComponentToRBPhysics();
			}

			UNavigationSystem::UpdateComponentInNavOctree(*DestructibleComponent);
		}
	}

	ComponentUpdateMapping.Reset();
}

TWeakObjectPtr<UPrimitiveComponent> FApexDestructionCustomPayload::GetOwningComponent() const
{
	return OwningComponent;
}

int32 FApexDestructionCustomPayload::GetItemIndex() const
{
	return ChunkIndex;
}

FName FApexDestructionCustomPayload::GetBoneName() const
{
	if(UDestructibleComponent* RawOwningComponent = OwningComponent.Get())
	{
		return RawOwningComponent->GetBoneName(UDestructibleComponent::ChunkIdxToBoneIdx(ChunkIndex));
	}
	else
	{
		return NAME_None;
	}
}

FBodyInstance* FApexDestructionCustomPayload::GetBodyInstance() const
{
	return OwningComponent.IsValid() ? &OwningComponent->BodyInstance : nullptr;
}

#endif // WITH_APEX