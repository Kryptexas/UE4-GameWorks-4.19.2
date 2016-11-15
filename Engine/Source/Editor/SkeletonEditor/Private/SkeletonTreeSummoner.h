// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WorkflowTabFactory.h"

struct FSkeletonTreeSummoner : public FWorkflowTabFactory
{
public:
	FSkeletonTreeSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp, TSharedRef<class ISkeletonTree> InSkeletonTree);

	/** FWorkflowTabFactory interface */
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override;

	/** Reference to our skeleton tree */
	TWeakPtr<class ISkeletonTree> SkeletonTreePtr;
};
