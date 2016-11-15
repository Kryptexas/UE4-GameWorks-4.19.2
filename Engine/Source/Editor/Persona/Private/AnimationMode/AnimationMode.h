// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "PersonaDelegates.h"
#include "PersonaModule.h"

/////////////////////////////////////////////////////
// FAnimEditAppMode

class FAnimEditAppMode : public FPersonaAppMode
{
public:
	FAnimEditAppMode(TSharedPtr<class FPersona> InPersona);
};

/////////////////////////////////////////////////////
// FAnimAssetPropertiesSummoner

struct FAssetPropertiesSummoner : public FWorkflowTabFactory
{
	FAssetPropertiesSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp, FOnGetAsset InOnGetAsset, FOnDetailsCreated InOnDetailsCreated);

	// FWorkflowTabFactory interface
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedPtr<SToolTip> CreateTabToolTipWidget(const FWorkflowTabSpawnInfo& Info) const override;
	// FWorkflowTabFactory interface

private:
	FOnGetAsset OnGetAsset;
	FOnDetailsCreated OnDetailsCreated;
};