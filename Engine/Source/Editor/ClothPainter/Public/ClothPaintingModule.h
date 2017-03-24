// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ModuleInterface.h"

#include "WorkflowOrientedApp/WorkflowCentricApplication.h"

class FApplicationMode;
class UEditorExperimentalSettings;

const static FName PaintModeID = "ClothPaintMode";

class FClothPaintingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// Setup and register our editmode
	void SetupMode();
	
	// Unregister and shut down our edit mode
	void ShutdownMode();

protected:
	TSharedRef<FApplicationMode> ExtendApplicationMode(const FName ModeName, TSharedRef<FApplicationMode> InMode);
protected:
	TArray<TWeakPtr<FApplicationMode>> RegisteredApplicationModes;
	FWorkflowApplicationModeExtender Extender;

private:

	// Weak ptr back to experimental settings as we'll need to unsubscribe later
	TWeakObjectPtr<UEditorExperimentalSettings> ExperimentalSettings;

	// Handler for the experimental settings changing so we can enable/disable the mode on the fly
	void OnExperimentalSettingsChanged(FName PropertyName);

	// Handle for the above event so it can be unregistered when nexessary
	FDelegateHandle ExperimentalSettingsEventHandle;
};
