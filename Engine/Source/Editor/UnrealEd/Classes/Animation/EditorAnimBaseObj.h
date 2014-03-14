// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/**
 * Abstract base class of animation composite base
 * This contains Composite Section data and some necessary interface to make this work
 */

#pragma once
#include "EditorAnimBaseObj.generated.h"

DECLARE_DELEGATE_TwoParams( FOnAnimObjectChange, class UObject*, bool)


UCLASS(abstract, MinimalAPI)
class UEditorAnimBaseObj: public UObject
{
	GENERATED_UCLASS_BODY()
public:
	

	virtual void InitFromAnim(UAnimSequenceBase* AnimObjectIn, FOnAnimObjectChange OnChange);
	virtual bool ApplyChangesToMontage();

	UAnimSequenceBase* AnimObject;
	FOnAnimObjectChange OnChange;

	virtual void PreEditChange( class FEditPropertyChain& PropertyAboutToChange ) OVERRIDE;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) OVERRIDE;

	virtual bool PropertyChangeRequiresRebuild(FPropertyChangedEvent& PropertyChangedEvent) { return true;}

	//void NotifyUser();
};
