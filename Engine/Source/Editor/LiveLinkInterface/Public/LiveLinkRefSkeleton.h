// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"

#include "LiveLinkRefSkeleton.generated.h"

USTRUCT()
struct FLiveLinkRefSkeleton
{
	GENERATED_USTRUCT_BODY()

	void SetBoneNames(const TArray<FName>& InBoneNames) { BoneNames = InBoneNames; }

	const TArray<FName>& GetBoneNames() const { return BoneNames; }

private:

	UPROPERTY()
	TArray<FName> BoneNames;
};