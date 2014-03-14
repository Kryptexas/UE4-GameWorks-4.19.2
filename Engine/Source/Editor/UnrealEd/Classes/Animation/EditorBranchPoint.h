// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Abstract base class of animation composite base
 * This contains Composite Section data and some necessary interface to make this work
 */

#pragma once
#include "EditorBranchPoint.generated.h"


UCLASS(hidecategories=UObject, MinimalAPI, BlueprintType)
class UEditorBranchPoint: public UEditorAnimBaseObj
{
	GENERATED_UCLASS_BODY()
public:

	/** Default blend in time. */
	UPROPERTY(EditAnywhere, Category=Montage)
	FBranchingPoint BranchingPoint;
	

	int32 BranchIndex;

	virtual void InitBranchPoint(int BranchIndexIn);
	virtual bool ApplyChangesToMontage() OVERRIDE;
};
