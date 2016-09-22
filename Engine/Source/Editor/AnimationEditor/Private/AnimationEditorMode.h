// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ApplicationMode.h"
#include "WorkflowCentricApplication.h"
#include "PersonaModule.h"

class FAnimationEditorMode : public FApplicationMode
{
public:
	FAnimationEditorMode(TSharedRef<class FWorkflowCentricApplication> InHostingApp, TSharedRef<class ISkeletonTree> InSkeletonTree);

	/** FApplicationMode interface */
	virtual void RegisterTabFactories(TSharedPtr<FTabManager> InTabManager) override;

protected:
	/** The hosting app */
	TWeakPtr<class FWorkflowCentricApplication> HostingAppPtr;

	/** The tab factories we support */
	FWorkflowAllowedTabSet TabFactories;
};