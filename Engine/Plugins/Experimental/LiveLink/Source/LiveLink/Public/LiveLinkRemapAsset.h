// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LiveLinkRetargetAsset.h"

#include "LiveLinkRemapAsset.generated.h"

// Remap asset for data coming from Live Link. Allows simple application of bone transforms into current pose based on name remapping only
UCLASS()
class ULiveLinkRemapAsset : public ULiveLinkRetargetAsset
{
	GENERATED_UCLASS_BODY()

	// Begin ULiveLinkRetargetAsset interface
	virtual void BuildPoseForSubject(const FLiveLinkSubjectFrame& InFrame, TSharedPtr<FLiveLinkRetargetContext> InOutContext, FCompactPose& OutPose, FBlendedCurve& OutCurve) const override;
	// End ULiveLinkRetargetAsset interface
};