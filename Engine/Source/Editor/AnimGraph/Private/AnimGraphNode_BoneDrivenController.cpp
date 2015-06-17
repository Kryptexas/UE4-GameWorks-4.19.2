// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "AnimGraphPrivatePCH.h"
#include "AnimGraphNode_BoneDrivenController.h"
#include "CompilerResultsLog.h"
#include "PropertyEditing.h"

#define LOCTEXT_NAMESPACE "A3Nodes"

UAnimGraphNode_BoneDrivenController::UAnimGraphNode_BoneDrivenController(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{

}

FText UAnimGraphNode_BoneDrivenController::GetTooltipText() const
{
	return LOCTEXT("UAnimGraphNode_BoneDrivenController_ToolTip", "Drives the transform of a bone using the transform of another");
}

FText UAnimGraphNode_BoneDrivenController::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if ((Node.SourceBone.BoneName == NAME_None) && (Node.TargetBone.BoneName == NAME_None) && (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle))
	{
		return GetControllerDescription();
	}
	// @TODO: the bone can be altered in the property editor, so we have to 
	//        choose to mark this dirty when that happens for this to properly work
	else //if (!CachedNodeTitles.IsTitleCached(TitleType, this))
	{
		FFormatNamedArguments Args;
		Args.Add(TEXT("ControllerDesc"), GetControllerDescription());
		Args.Add(TEXT("SourceBone"), FText::FromName(Node.SourceBone.BoneName));
		Args.Add(TEXT("TargetBone"), FText::FromName(Node.TargetBone.BoneName));

		if (TitleType == ENodeTitleType::ListView || TitleType == ENodeTitleType::MenuTitle)
		{
			Args.Add(TEXT("Delim"), FText::FromString(TEXT(" - ")));
		}
		else
		{
			Args.Add(TEXT("Delim"), FText::FromString(TEXT("\n")));
		}

		// FText::Format() is slow, so we cache this to save on performance
		CachedNodeTitles.SetCachedTitle(TitleType, FText::Format(LOCTEXT("AnimGraphNode_BoneDrivenController_Title", "{ControllerDesc}{Delim}Driver Bone: {SourceBone}{Delim}Driven Bone: {TargetBone}"), Args), this);
	}	
	return CachedNodeTitles[TitleType];
}

FText UAnimGraphNode_BoneDrivenController::GetControllerDescription() const
{
	return LOCTEXT("BoneDrivenController", "Bone Driven Controller");
}

void UAnimGraphNode_BoneDrivenController::Draw(FPrimitiveDrawInterface* PDI, USkeletalMeshComponent* SkelMeshComp) const
{	
	static const float ArrowHeadWidth = 5.0f;
	static const float ArrowHeadHeight = 8.0f;

	const int32 SourceIdx = SkelMeshComp->GetBoneIndex(Node.SourceBone.BoneName);
	const int32 TargetIdx = SkelMeshComp->GetBoneIndex(Node.TargetBone.BoneName);

	if ((SourceIdx != INDEX_NONE) && (TargetIdx != INDEX_NONE))
	{
		const FTransform SourceTM = SkelMeshComp->GetSpaceBases()[SourceIdx] * SkelMeshComp->ComponentToWorld;
		const FTransform TargetTM = SkelMeshComp->GetSpaceBases()[TargetIdx] * SkelMeshComp->ComponentToWorld;

		PDI->DrawLine(TargetTM.GetLocation(), SourceTM.GetLocation(), FLinearColor(0.0f, 0.0f, 1.0f), SDPG_Foreground, 0.5f);

		const FVector ToTarget = TargetTM.GetTranslation() - SourceTM.GetTranslation();
		const FVector UnitToTarget = ToTarget.GetSafeNormal();
		FVector Midpoint = SourceTM.GetTranslation() + 0.5f * ToTarget + 0.5f * UnitToTarget * ArrowHeadHeight;

		FVector YAxis;
		FVector ZAxis;
		UnitToTarget.FindBestAxisVectors(YAxis, ZAxis);
		const FMatrix ArrowMatrix(UnitToTarget, YAxis, ZAxis, Midpoint);

		DrawConnectedArrow(PDI, ArrowMatrix, FLinearColor(0.0f, 1.0f, 0.0), ArrowHeadHeight, ArrowHeadWidth, SDPG_Foreground);

		PDI->DrawPoint(SourceTM.GetTranslation(), FLinearColor(0.8f, 0.8f, 0.2f), 5.0f, SDPG_Foreground);
		PDI->DrawPoint(SourceTM.GetTranslation() + ToTarget, FLinearColor(0.8f, 0.8f, 0.2f), 5.0f, SDPG_Foreground);
	}
}

void UAnimGraphNode_BoneDrivenController::ValidateAnimNodeDuringCompilation(USkeleton* ForSkeleton, FCompilerResultsLog& MessageLog)
{
	if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.SourceBone.BoneName) == INDEX_NONE)
	{
		MessageLog.Warning(*LOCTEXT("NoSourceBone", "@@ - You must pick a source bone as the Driver joint").ToString(), this);
	}

	if (Node.SourceComponent == EComponentType::None)
	{
		MessageLog.Warning(*LOCTEXT("NoSourceComponent", "@@ - You must pick a source component on the Driver joint").ToString(), this);
	}
	
	if (ForSkeleton->GetReferenceSkeleton().FindBoneIndex(Node.TargetBone.BoneName) == INDEX_NONE)
	{
		MessageLog.Warning(*LOCTEXT("NoTargetBone", "@@ - You must pick a target bone as the Driven joint").ToString(), this);
	}

	if (Node.TargetComponent == EComponentType::None)
	{
		MessageLog.Warning(*LOCTEXT("NoTargetComponent", "@@ - You must pick a target component on the Driven joint").ToString(), this);
	}

	Super::ValidateAnimNodeDuringCompilation(ForSkeleton, MessageLog);
}

void UAnimGraphNode_BoneDrivenController::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.EditCategory(TEXT("Source (Driver)"));
	IDetailCategoryBuilder& MappingCategory = DetailBuilder.EditCategory(TEXT("Mapping"));
	DetailBuilder.EditCategory(TEXT("Destination (Driven)"));

	TSharedRef<IPropertyHandle> NodeHandle = DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(UAnimGraphNode_BoneDrivenController, Node), GetClass());

	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_BoneDrivenController, DrivingCurve)));

	TAttribute<EVisibility> MappingVisibility = TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateUObject(this, &UAnimGraphNode_BoneDrivenController::AreNonCurveMappingValuesVisible));
	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_BoneDrivenController, Multiplier))).Visibility(MappingVisibility);
	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_BoneDrivenController, bUseRange))).Visibility(MappingVisibility);
	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_BoneDrivenController, RangeMin))).Visibility(MappingVisibility);
	MappingCategory.AddProperty(NodeHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FAnimNode_BoneDrivenController, RangeMax))).Visibility(MappingVisibility);
}

EVisibility UAnimGraphNode_BoneDrivenController::AreNonCurveMappingValuesVisible() const
{
	return (Node.DrivingCurve != nullptr) ? EVisibility::Collapsed : EVisibility::Visible;
}


#undef LOCTEXT_NAMESPACE
