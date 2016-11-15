// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "SAnimationBlendSpaceBase.h"
#include "Animation/BlendSpace1D.h"
#include "AnimationBlendSpace1DHelpers.h"

class SBlendSpaceEditor1D : public SBlendSpaceEditorBase
{
public:
	SLATE_BEGIN_ARGS(SBlendSpaceEditor1D)
		: _BlendSpace1D(NULL)
		{}
		SLATE_ARGUMENT(UBlendSpace1D*, BlendSpace1D)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<IPersonaPreviewScene>& InPreviewScene, FSimpleMulticastDelegate& OnPostUndo);
protected:	
	virtual void ResampleData() override;
	
	/** Generates editor elements in 1D (line) space */
	FLineElementGenerator ElementGenerator;
};
