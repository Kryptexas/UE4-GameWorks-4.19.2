// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_BoneDrivenController.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_BoneDrivenController::UAnimGraphNode_BoneDrivenController(const FPostConstructInitializeProperties& PCIP)
	:Super(PCIP)
{

}

FString UAnimGraphNode_BoneDrivenController::GetTooltip() const
{
	return LOCTEXT("UAnimGraphNode_BoneDrivenController_ToolTip", "Drives the transform of a bone using the transform of another").ToString();
}

FText UAnimGraphNode_BoneDrivenController::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("ControllerDesc"), GetControllerDescription());
	Args.Add(TEXT("SourceBone"), FText::FromName(Node.SourceBone.BoneName));
	Args.Add(TEXT("TargetBone"), FText::FromName(Node.TargetBone.BoneName));

	FText NodeTitle;
	if (TitleType == ENodeTitleType::ListView)
	{
		Args.Add(TEXT("Delim"), FText::FromString(TEXT(" - ")));

		if ((Node.SourceBone.BoneName == NAME_None) && (Node.TargetBone.BoneName == NAME_None))
		{
			NodeTitle = FText::Format(LOCTEXT("AnimGraphNode_BoneDrivenController_MenuTitle", "{ControllerDesc}"), Args);
		}
	}
	else
	{
		Args.Add(TEXT("Delim"), FText::FromString(TEXT("\n")));
	}

	if (NodeTitle.IsEmpty())
	{
		NodeTitle = FText::Format(LOCTEXT("AnimGraphNode_BoneDrivenController_Title", "{ControllerDesc}{Delim}Driving Bone: {SourceBone}{Delim}Driven Bone: {TargetBone}"), Args);
	}
	
	return NodeTitle;
}

FText UAnimGraphNode_BoneDrivenController::GetControllerDescription() const
{
	return LOCTEXT("BoneDrivenController", "Bone Driven Controller");
}

void UAnimGraphNode_BoneDrivenController::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{	
	static const float ArrowHeadWidth = 5.0f;
	static const float ArrowHeadHeight = 8.0f;

	int32 SourceIdx = SkelMeshComp->GetBoneIndex(Node.SourceBone.BoneName);
	int32 TargetIdx = SkelMeshComp->GetBoneIndex(Node.TargetBone.BoneName);

	if(SourceIdx != INDEX_NONE && TargetIdx != INDEX_NONE)
	{
		FTransform SourceTM = SkelMeshComp->SpaceBases[SourceIdx] * SkelMeshComp->ComponentToWorld;
		FTransform TargetTM = SkelMeshComp->SpaceBases[TargetIdx] * SkelMeshComp->ComponentToWorld;

		PDI->DrawLine(TargetTM.GetLocation(), SourceTM.GetLocation(), FLinearColor(0.0f, 0.0f, 1.0f), SDPG_Foreground, 0.5f);

		FVector ToTarget = TargetTM.GetTranslation() - SourceTM.GetTranslation();
		FVector UnitToTarget = ToTarget;
		UnitToTarget.Normalize();
		FVector Midpoint = SourceTM.GetTranslation() + 0.5f * ToTarget + 0.5f * UnitToTarget * ArrowHeadHeight;

		FVector YAxis;
		FVector ZAxis;
		UnitToTarget.FindBestAxisVectors(YAxis, ZAxis);
		FMatrix ArrowMatrix(UnitToTarget, YAxis, ZAxis, Midpoint);

		DrawConnectedArrow(PDI, ArrowMatrix, FLinearColor(0.0f, 1.0f, 0.0), ArrowHeadHeight, ArrowHeadWidth, SDPG_Foreground);

		PDI->DrawPoint(SourceTM.GetTranslation(), FLinearColor(0.8f, 0.8f, 0.2f), 5.0f, SDPG_Foreground);
		PDI->DrawPoint(SourceTM.GetTranslation() + ToTarget, FLinearColor(0.8f, 0.8f, 0.2f), 5.0f, SDPG_Foreground);
	}
}

#undef LOCTEXT_NAMESPACE
