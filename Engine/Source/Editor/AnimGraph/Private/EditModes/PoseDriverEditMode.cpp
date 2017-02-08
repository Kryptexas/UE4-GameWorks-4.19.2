// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "EditModes/PoseDriverEditMode.h"
#include "SceneManagement.h"
#include "AnimNodes/AnimNode_PoseDriver.h"
#include "AnimGraphNode_PoseDriver.h"
#include "IPersonaPreviewScene.h"
#include "Animation/DebugSkelMeshComponent.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

void FPoseDriverEditMode::EnterMode(class UAnimGraphNode_Base* InEditorNode, struct FAnimNode_Base* InRuntimeNode)
{
	RuntimeNode = static_cast<FAnimNode_PoseDriver*>(InRuntimeNode);
	GraphNode = CastChecked<UAnimGraphNode_PoseDriver>(InEditorNode);

	FAnimNodeEditMode::EnterMode(InEditorNode, InRuntimeNode);
}

void FPoseDriverEditMode::ExitMode()
{
	RuntimeNode = nullptr;
	GraphNode = nullptr;

	FAnimNodeEditMode::ExitMode();
}

static FLinearColor GetColorFromWeight(float InWeight)
{
	return FMath::Lerp(FLinearColor::White, FLinearColor::Red, InWeight);
}

void FPoseDriverEditMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	USkeletalMeshComponent* SkelComp = GetAnimPreviewScene().GetPreviewMeshComponent();

	// Tell graph node last comp we were used on. A bit ugly, but no easy way to get from details customization to editor instance
	GraphNode->LastPreviewComponent = SkelComp;

	static const float DrawLineWidth = 0.1f;
	static const float DrawAxisLength = 20.f;
	static const float DrawPosSize = 2.f;

	int32 BoneIndex = SkelComp->GetBoneIndex(RuntimeNode->SourceBone.BoneName);
	if (BoneIndex != INDEX_NONE)
	{
		// Get transform of driven bone, used as basis for drawing
		FTransform BoneWorldTM = SkelComp->GetBoneTransform(BoneIndex);
		FVector BonePos = BoneWorldTM.GetLocation();

		// Transform that we are evaluating pose in
		FTransform EvalSpaceTM = SkelComp->GetComponentToWorld();

		// If specifying space to eval in, get that space
		int32 EvalSpaceBoneIndex = SkelComp->GetBoneIndex(RuntimeNode->EvalSpaceBone.BoneName);
		FName ParentBoneName = SkelComp->GetParentBone(RuntimeNode->SourceBone.BoneName);
		int32 ParentBoneIndex = SkelComp->GetBoneIndex(ParentBoneName);
		if (EvalSpaceBoneIndex != INDEX_NONE)
		{
			EvalSpaceTM = SkelComp->GetBoneTransform(EvalSpaceBoneIndex);
		}
		// Otherwise, just use parent bone
		else if(ParentBoneIndex != INDEX_NONE)
		{
			EvalSpaceTM = SkelComp->GetBoneTransform(ParentBoneIndex);
		}

		// Rotation drawing
		if (RuntimeNode->DriveSource == EPoseDriverSource::Rotation)
		{
			FVector LocalVec = RuntimeNode->SourceBoneTM.TransformVectorNoScale(RuntimeNode->RBFParams.GetTwistAxisVector());
			FVector WorldVec = EvalSpaceTM.TransformVectorNoScale(LocalVec);
			PDI->DrawLine(BonePos, BonePos + (WorldVec*DrawAxisLength), FLinearColor::Green, SDPG_Foreground, DrawLineWidth);
		}
		// Translation drawing
		else if (RuntimeNode->DriveSource == EPoseDriverSource::Translation)
		{
			FVector LocalPos = RuntimeNode->SourceBoneTM.GetTranslation();
			FVector WorldPos = EvalSpaceTM.TransformPosition(LocalPos);
			DrawWireDiamond(PDI, FTranslationMatrix(WorldPos), DrawPosSize, FLinearColor::Green, SDPG_Foreground);
		}

		// Build array of weight for every target
		TArray<float> PerTargetWeights;
		PerTargetWeights.AddZeroed(RuntimeNode->PoseTargets.Num());
		for (const FRBFOutputWeight& Weight : RuntimeNode->OutputWeights)
		{
			PerTargetWeights[Weight.TargetIndex] = Weight.TargetWeight;
		}

		// Draw every target
		for (int32 TargetIdx=0 ; TargetIdx <  RuntimeNode->PoseTargets.Num() ; TargetIdx++)
		{
			FPoseDriverTarget& PoseTarget = RuntimeNode->PoseTargets[TargetIdx];

			bool bSelected = (GraphNode->SelectedTargetIndex == TargetIdx);
			float AxisLength = bSelected ? (DrawAxisLength * 1.5f) : DrawAxisLength;
			float LineWidth = bSelected ? (DrawLineWidth * 3.f) : DrawLineWidth;
			float PosSize = bSelected ? (DrawPosSize * 1.5f) : DrawPosSize;

			// Rotation drawing
			if (RuntimeNode->DriveSource == EPoseDriverSource::Rotation)
			{
				FVector LocalVec = PoseTarget.TargetRotation.RotateVector(RuntimeNode->RBFParams.GetTwistAxisVector());
				FVector WorldVec = EvalSpaceTM.TransformVectorNoScale(LocalVec);
				PDI->DrawLine(BonePos, BonePos + (WorldVec*AxisLength), GetColorFromWeight(PerTargetWeights[TargetIdx]), SDPG_Foreground, LineWidth);
			}
			// Translation drawing
			else if (RuntimeNode->DriveSource == EPoseDriverSource::Translation)
			{
				FVector LocalPos = PoseTarget.TargetTranslation;
				FVector WorldPos = EvalSpaceTM.TransformPosition(LocalPos);
				DrawWireDiamond(PDI, FTranslationMatrix(WorldPos), PosSize, GetColorFromWeight(PerTargetWeights[TargetIdx]), SDPG_Foreground, LineWidth);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
