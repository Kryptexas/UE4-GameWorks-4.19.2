// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PreviewScene.h"
#include "AnimGraphDefinitions.h"

class FTransitionPoseEvaluatorNodeDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails( IDetailLayoutBuilder& DetailBuilder ) OVERRIDE;

private:
	EVisibility GetFramesToCachePoseVisibility( ) const;

private:
	TWeakObjectPtr<UAnimGraphNode_TransitionPoseEvaluator> EvaluatorNode;
};

