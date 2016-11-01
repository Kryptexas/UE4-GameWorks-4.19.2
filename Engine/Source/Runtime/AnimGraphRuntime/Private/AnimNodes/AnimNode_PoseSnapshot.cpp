// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphRuntimePrivatePCH.h"
#include "AnimNodes/AnimNode_PoseSnapshot.h"
#include "AnimInstanceProxy.h"

/////////////////////////////////////////////////////
// FAnimCachePoseNode


void FAnimNode_PoseSnapshot::Evaluate(FPoseContext& Output)
{
	FCompactPose& OutPose = Output.Pose;
	OutPose.ResetToRefPose();

	if(const FAnimInstanceProxy::FPoseSnapshot* PoseSnapshot = Output.AnimInstanceProxy->GetPoseSnapshot(SnapshotName))
	{
		const TArray<FTransform>& LocalTMs = PoseSnapshot->LocalTransforms;
		const FBoneContainer& RequiredBones = OutPose.GetBoneContainer();
		for (FCompactPoseBoneIndex PoseBoneIndex : OutPose.ForEachBoneIndex())
		{
			const int32 SkeletonBoneIndex = RequiredBones.GetSkeletonIndex(PoseBoneIndex);
			const int32 MeshBoneIndex = RequiredBones.GetSkeletonToPoseBoneIndexArray()[SkeletonBoneIndex];

			if (LocalTMs.IsValidIndex(MeshBoneIndex))
			{
				OutPose[PoseBoneIndex] = LocalTMs[MeshBoneIndex];
			}
		}
	}
}


void FAnimNode_PoseSnapshot::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this) + " Snapshot Name:" + SnapshotName.ToString();
	DebugData.AddDebugItem(DebugLine, true);
}