// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "AnimNode_LiveLinkPose.h"
#include "ILiveLinkClient.h"

#include "Features/IModularFeatures.h"

#include "AnimInstanceProxy.h"

#include "LiveLinkRemapAsset.h"

#define LOCTEXT_NAMESPACE "LiveLinkAnimNode"

void FAnimNode_LiveLinkPose::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	IModularFeatures& ModularFeatures = IModularFeatures::Get();

	if (ModularFeatures.IsModularFeatureAvailable(ILiveLinkClient::ModularFeatureName))
	{
		LiveLinkClient = &IModularFeatures::Get().GetModularFeature<ILiveLinkClient>(ILiveLinkClient::ModularFeatureName);
	}

	PreviousRetargetAsset = nullptr;
}

void FAnimNode_LiveLinkPose::Update_AnyThread(const FAnimationUpdateContext & Context)
{
	EvaluateGraphExposedInputs.Execute(Context);

	ULiveLinkRetargetAsset* CurrentRetargetAsset = RetargetAsset ? RetargetAsset : ULiveLinkRemapAsset::StaticClass()->GetDefaultObject<ULiveLinkRetargetAsset>();
	if (PreviousRetargetAsset != CurrentRetargetAsset)
	{
		RetargetContext = CurrentRetargetAsset->CreateRetargetContext();
		PreviousRetargetAsset = CurrentRetargetAsset;
	}
}

void FAnimNode_LiveLinkPose::Evaluate_AnyThread(FPoseContext& Output)
{
	Output.ResetToRefPose();

	if (!LiveLinkClient || !PreviousRetargetAsset)
	{
		return;
	}

	

	if(const FLiveLinkSubjectFrame* Subject = LiveLinkClient->GetSubjectData(SubjectName))
	{
		PreviousRetargetAsset->BuildPoseForSubject(*Subject, RetargetContext, Output.Pose, Output.Curve);
	}
}


void FAnimNode_LiveLinkPose::OnLiveLinkClientRegistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == ILiveLinkClient::ModularFeatureName && !LiveLinkClient)
	{
		LiveLinkClient = static_cast<ILiveLinkClient*>(ModularFeature);
	}
}

void FAnimNode_LiveLinkPose::OnLiveLinkClientUnregistered(const FName& Type, class IModularFeature* ModularFeature)
{
	if (Type == ILiveLinkClient::ModularFeatureName && ModularFeature == LiveLinkClient)
	{
		LiveLinkClient = nullptr;
	}
}

UAnimGraphNode_LiveLinkPose::UAnimGraphNode_LiveLinkPose(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FText UAnimGraphNode_LiveLinkPose::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return LOCTEXT("NodeTitle", "Live Link Pose");
}

FText UAnimGraphNode_LiveLinkPose::GetTooltipText() const
{
	return LOCTEXT("NodeTooltip", "Retrieves the current pose associated with the supplied subject");
}

FText UAnimGraphNode_LiveLinkPose::GetMenuCategory() const
{
	return LOCTEXT("NodeCategory", "Live Link");
}

#undef LOCTEXT_NAMESPACE
