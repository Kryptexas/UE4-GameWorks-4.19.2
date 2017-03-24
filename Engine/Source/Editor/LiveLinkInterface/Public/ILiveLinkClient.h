// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LiveLinkRefSkeleton.h"
#include "Features/IModularFeature.h"

class LIVELINKINTERFACE_API ILiveLinkClient : public IModularFeature
{
public:
	virtual ~ILiveLinkClient() {}

	static FName ModularFeatureName;

	virtual void PushSubjectSkeleton(FName SubjectName, const FLiveLinkRefSkeleton& RefSkeleton) = 0;
	virtual void PushSubjectData(FName SubjectName, const TArray<FTransform>& Transforms) = 0;

	virtual bool GetSubjectData(FName SubjectName, TArray<FTransform>& OutTransforms, FLiveLinkRefSkeleton& OutRefSkeleton) = 0;
};