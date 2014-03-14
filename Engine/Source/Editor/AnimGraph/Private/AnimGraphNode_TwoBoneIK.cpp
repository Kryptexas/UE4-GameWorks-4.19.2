// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"

/////////////////////////////////////////////////////
// UAnimGraphNode_TwoBoneIK

UAnimGraphNode_TwoBoneIK::UAnimGraphNode_TwoBoneIK(const FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

FString UAnimGraphNode_TwoBoneIK::GetControllerDescription() const
{
	return TEXT("Two Bone IK");
}

FString UAnimGraphNode_TwoBoneIK::GetTooltip() const
{
	return TEXT("The Two Bone IK control applies an inverse kinematic (IK) solver to a 3-joint chain, such as the limbs of a character.");
}

FString UAnimGraphNode_TwoBoneIK::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FString Result = *GetControllerDescription();
	Result += (TitleType == ENodeTitleType::ListView) ? TEXT(" - ") : TEXT("\n");
	Result += FString::Printf(TEXT("Bone: %s"), *Node.IKBone.BoneName.ToString());
	return Result;
}

void UAnimGraphNode_TwoBoneIK::Draw( FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp ) const
{
	if ( SkelMeshComp && SkelMeshComp->SkeletalMesh && SkelMeshComp->SkeletalMesh->Skeleton )
	{
		USkeleton * Skeleton = SkelMeshComp->SkeletalMesh->Skeleton;

		DrawTargetLocation(PDI, SkelMeshComp, Skeleton, Node.EffectorLocationSpace, Node.EffectorSpaceBoneName,Node.EffectorLocation, FColor(255,128,128), FColor(180,128,128));
		DrawTargetLocation(PDI, SkelMeshComp, Skeleton, Node.JointTargetLocationSpace, Node.JointTargetSpaceBoneName,Node.JointTargetLocation, FColor(128, 255, 128), FColor(128,180,128));
	}
}

void UAnimGraphNode_TwoBoneIK::DrawTargetLocation(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelComp, USkeleton * Skeleton, uint8 SpaceBase, FName SpaceBoneName, const FVector & TargetLocation, const FColor & TargetColor, const FColor & BoneColor) const
{
	const bool bInBoneSpace = (SpaceBase == BCS_ParentBoneSpace) || (SpaceBase == BCS_BoneSpace);
	const int32 SpaceBoneIndex = bInBoneSpace ? Skeleton->GetReferenceSkeleton().FindBoneIndex(SpaceBoneName) : INDEX_NONE;
	// Transform EffectorLocation from EffectorLocationSpace to ComponentSpace.
	FTransform TargetTransform = FTransform (TargetLocation);
	FTransform CSTransform;
	ConvertToComponentSpaceTransform(SkelComp, Skeleton, TargetTransform, CSTransform, SpaceBoneIndex, SpaceBase);

	FTransform WorldTransform = CSTransform * SkelComp->ComponentToWorld;

	DrawCoordinateSystem( PDI, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator(), 20.f, SDPG_Foreground );
	DrawWireDiamond( PDI, WorldTransform.ToMatrixWithScale(), 2.f, TargetColor, SDPG_Foreground );

	if (bInBoneSpace)
	{
		ConvertToComponentSpaceTransform(SkelComp, Skeleton, FTransform::Identity, CSTransform, SpaceBoneIndex, SpaceBase);
		WorldTransform = CSTransform * SkelComp->ComponentToWorld;
		DrawCoordinateSystem( PDI, WorldTransform.GetLocation(), WorldTransform.GetRotation().Rotator(), 20.f, SDPG_Foreground );
		DrawWireDiamond( PDI, WorldTransform.ToMatrixWithScale(), 2.f, BoneColor, SDPG_Foreground );
	}
}

void UAnimGraphNode_TwoBoneIK::ConvertToComponentSpaceTransform(USkeletalMeshComponent* SkelComp, USkeleton * Skeleton, const FTransform & InTransform, FTransform & OutCSTransform, int32 BoneIndex, uint8 Space) const
{
	switch( Space )
	{
	case BCS_WorldSpace : 
		{
			OutCSTransform = InTransform;
			OutCSTransform.SetToRelativeTransform(SkelComp->ComponentToWorld);
		}
		break;

	case BCS_ComponentSpace :
		{
			// Component Space, no change.
			OutCSTransform = InTransform;
		}
		break;

	case BCS_ParentBoneSpace :
		if( BoneIndex != INDEX_NONE )
		{
			const int32 ParentIndex = Skeleton->GetReferenceSkeleton().GetParentIndex(BoneIndex);
			if( ParentIndex != INDEX_NONE )
			{
				const int32 MeshParentIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->SkeletalMesh, ParentIndex);
				if (MeshParentIndex != INDEX_NONE)
				{
					const FTransform ParentTM = SkelComp->GetBoneTransform(MeshParentIndex);
					OutCSTransform = InTransform * ParentTM;
				}
				else
				{
					OutCSTransform = InTransform;
				}
			}
		}
		break;

	case BCS_BoneSpace :
		if( BoneIndex != INDEX_NONE )
		{
			const int32 MeshBoneIndex = Skeleton->GetMeshBoneIndexFromSkeletonBoneIndex(SkelComp->SkeletalMesh, BoneIndex);
			if (MeshBoneIndex != INDEX_NONE)
			{
				const FTransform BoneTM = SkelComp->GetBoneTransform(MeshBoneIndex);
				OutCSTransform = InTransform * BoneTM;
			}
			else
			{
				OutCSTransform = InTransform;
			}
		}
		break;

	default:
		if( SkelComp->SkeletalMesh )
		{
			UE_LOG(LogAnimation, Warning, TEXT("ConvertToComponentSpaceTransform: Unknown BoneSpace %d  for Mesh: %s"), Space, *SkelComp->SkeletalMesh->GetFName().ToString() );
		}
		else
		{
			UE_LOG(LogAnimation, Warning, TEXT("ConvertToComponentSpaceTransform: Unknown BoneSpace %d  for Skeleton: %s"), Space, *Skeleton->GetFName().ToString() );
		}
		break;
	}
}
