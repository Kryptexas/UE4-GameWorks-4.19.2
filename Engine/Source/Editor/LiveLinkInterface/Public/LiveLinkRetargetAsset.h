// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"

#include "LiveLinkRetargetAsset.generated.h"

struct FLiveLinkSubjectFrame;
struct FCompactPose;
struct FBlendedCurve;

// Base class for live link retargeting contexts. Derive from this and return a new instance of your new derived class
// from CreateRetargetContext
struct FLiveLinkRetargetContext
{

};

// Base class for retargeting live link data. 
UCLASS(abstract)
class LIVELINKINTERFACE_API ULiveLinkRetargetAsset : public UObject
{
	GENERATED_UCLASS_BODY()

	// Return a context that will be passed into BuildPoseForSubject. Allows uses of ULiveLinkRetargetAsset
	// to have unique runtime data. 
	virtual TSharedPtr<FLiveLinkRetargetContext> CreateRetargetContext() const { return nullptr; }

	// Builds curve data into OutCurve from the supplied live link frame
	void BuildCurveData(const FLiveLinkSubjectFrame& InFrame, const FCompactPose& InPose, FBlendedCurve& OutCurve) const;

	// Build OutPose and OutCurve from the supplied InFrame.
	virtual void BuildPoseForSubject(const FLiveLinkSubjectFrame& InFrame, TSharedPtr<FLiveLinkRetargetContext> InOutContext, FCompactPose& OutPose, FBlendedCurve& OutCurve) const PURE_VIRTUAL(ULiveLinkRetargetAsset::BuildPoseForSubject, );
};