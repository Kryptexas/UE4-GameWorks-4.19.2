// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "LiveEditorDevice.h"
#include "LiveEditorWizardBase.h"

class FLiveEditorDeviceSetupWizard : public FLiveEditorWizardBase
{
public:
	FLiveEditorDeviceSetupWizard();
	virtual ~FLiveEditorDeviceSetupWizard();

	void Start( PmDeviceID _DeviceID, struct FLiveEditorDeviceData &Data );
	void Configure( bool bHasButtons, bool bHasEndlessEncoders );

	virtual FString GetAdvanceButtonText() const;

protected:
	virtual void OnWizardFinished( struct FLiveEditorDeviceData &Data );

private:
	struct FConfigurationState *ConfigState;
};
