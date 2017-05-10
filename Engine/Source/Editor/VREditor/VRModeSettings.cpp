// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "VRModeSettings.h"
#include "Dialogs.h"
#include "UnrealType.h"

#define LOCTEXT_NAMESPACE "VREditor"

UVRModeSettings::UVRModeSettings()
	: Super()
{
	bEnableAutoVREditMode = false;
	InteractorHand = EInteractorHand::Right;
	bShowWorldMovementGrid = true;
	bShowWorldMovementPostProcess = true;
	bShowWorldScaleProgressBar = true;
	UIBrightness = 1.5f;
	GizmoScale = 0.8f;
	DoubleClickTime = 0.25f;
	TriggerPressedThreshold_Vive = 0.03f;
	TriggerPressedThreshold_Rift = 0.2f;
}

#if WITH_EDITOR
void UVRModeSettings::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property
		&& PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UVRModeSettings, bEnableAutoVREditMode)
		&& bEnableAutoVREditMode == true)
	{
		FSuppressableWarningDialog::FSetupInfo SetupInfo(LOCTEXT("VRModeEntry_Message", "VR Mode enables you to work on your project in virtual reality using motion controllers. This feature is still under development, so you may experience bugs or crashes while using it."),
			LOCTEXT("VRModeEntry_Title", "Entering VR Mode - Experimental"), "Warning_VRModeEntry", GEditorSettingsIni);

		SetupInfo.ConfirmText = LOCTEXT("VRModeEntry_ConfirmText", "Continue");
		SetupInfo.CancelText = LOCTEXT("VRModeEntry_CancelText", "Cancel");
		SetupInfo.bDefaultToSuppressInTheFuture = true;
		FSuppressableWarningDialog VRModeEntryWarning(SetupInfo);
		bEnableAutoVREditMode = (VRModeEntryWarning.ShowModal() != FSuppressableWarningDialog::Cancel) ? true : false;
	}
}
#endif

#undef LOCTEXT_NAMESPACE