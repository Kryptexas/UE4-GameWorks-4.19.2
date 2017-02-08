// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "ClothingSimulation.h"

#include "ClothingSimulationInterface.h"
#include "ClothingSystemRuntimeModule.h"
#include "Components/SkeletalMeshComponent.h"

void FClothingSimulationBase::FillContext(USkeletalMeshComponent* InComponent, IClothingSimulationContext* InOutContext)
{
	FClothingSimulationContextBase* BaseContext = static_cast<FClothingSimulationContextBase*>(InOutContext);
	BaseContext->ComponentToWorld = InComponent->ComponentToWorld;
	BaseContext->PredictedLod = InComponent->PredictedLODLevel;
	InComponent->GetWindForCloth_GameThread(BaseContext->WindVelocity, BaseContext->WindAdaption);
	
	if(USkinnedMeshComponent* MasterComponent = InComponent->MasterPoseComponent.Get())
	{
		const int32 NumBones = InComponent->GetNumComponentSpaceTransforms();
		
		BaseContext->BoneTransforms.Empty(NumBones);
		BaseContext->BoneTransforms.AddDefaulted(NumBones);

		for(int32 BoneIndex = 0; BoneIndex < NumBones; ++BoneIndex)
		{
			if(InComponent->MasterBoneMap.IsValidIndex(BoneIndex))
			{
				const int32 ParentIndex = InComponent->MasterBoneMap[BoneIndex];
				BaseContext->BoneTransforms[BoneIndex] = MasterComponent->GetComponentSpaceTransforms()[ParentIndex];
			}
			else
			{
				BaseContext->BoneTransforms[BoneIndex] = FTransform::Identity;
			}
		}
	}
	else
	{
		BaseContext->BoneTransforms = InComponent->GetComponentSpaceTransforms();
	}

	UWorld* ComponentWorld = InComponent->GetWorld();
	check(ComponentWorld);

	BaseContext->DeltaSeconds = ComponentWorld->GetDeltaSeconds();

	BaseContext->TeleportMode = InComponent->ClothTeleportMode;

	BaseContext->MaxDistanceScale = InComponent->GetClothMaxDistanceScale();
}
