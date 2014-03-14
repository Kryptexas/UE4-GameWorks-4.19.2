// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Abstract base class of animation composite base
 * This contains Composite Section data and some necessary interface to make this work
 */

#pragma once
#include "EditorAnimCompositeSegment.generated.h"

DECLARE_DELEGATE_OneParam( FOnAnimSegmentChanged, class UEditorAnimSegment*)


UCLASS(hidecategories=UObject, MinimalAPI, BlueprintType)
class UEditorAnimCompositeSegment: public UEditorAnimBaseObj
{
	GENERATED_UCLASS_BODY()
public:

	/** Default blend in time. */
	UPROPERTY(EditAnywhere, Category=Montage)
	FAnimSegment AnimSegment;

	int AnimSegmentIndex;

	virtual void InitAnimSegment(int AnimSegmentIndex);
	virtual bool ApplyChangesToMontage() OVERRIDE;

	virtual bool PropertyChangeRequiresRebuild(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;

private:
	//FAnimTrack* GetAnimTrack
};
