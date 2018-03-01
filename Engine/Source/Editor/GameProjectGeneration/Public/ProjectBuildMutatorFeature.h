// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Features/IModularFeature.h"

class FProjectBuildMutatorFeature : public IModularFeature
{
public:

	virtual bool RequiresProjectBuild(FName InPlatformInfoName) const = 0;

	static FORCEINLINE FName GetFeatureName()
	{
		static const FName Name(TEXT("ProjectBuildMutatorFeature"));
		return Name;
	}
};