// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "GameFramework/TouchInterface.h"

UTouchInterface::UTouchInterface(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
	// defaults
	ActiveOpacity = 1.0f;
	InactiveOpacity = 0.1f;
	TimeUntilDeactive = 0.5f;
	TimeUntilReset = 2.0f;
	ActivationDelay = 0.f;
	StartupDelay = 0.f;

	bPreventRecenter = false;
}

void UTouchInterface::Activate(TSharedPtr<SVirtualJoystick> VirtualJoystick)
{
	VirtualJoystick->SetGlobalParameters(ActiveOpacity, InactiveOpacity, TimeUntilDeactive, TimeUntilReset, ActivationDelay, bPreventRecenter, StartupDelay);

	// convert from the UStructs to the slate structs
	TArray<SVirtualJoystick::FControlInfo> SlateControls;

	for (int32 ControlIndex = 0; ControlIndex < Controls.Num(); ControlIndex++)
	{
		FTouchInputControl Control = Controls[ControlIndex];
		SVirtualJoystick::FControlInfo* SlateControl = new(SlateControls)SVirtualJoystick::FControlInfo;

		SlateControl->Image1 = FCoreStyle::GetDynamicImageBrush("Engine.Joystick.Image1", Control.Image1, Control.Image1->GetFName());
		SlateControl->Image2 = FCoreStyle::GetDynamicImageBrush("Engine.Joystick.Image2", Control.Image2, Control.Image2->GetFName());
		SlateControl->Center = Control.Center;
		SlateControl->VisualSize = Control.VisualSize;
		SlateControl->ThumbSize = Control.ThumbSize;
		if (Control.InputScale.Size() > DELTA)
		{
			SlateControl->InputScale = Control.InputScale;
		}
		SlateControl->InteractionSize = Control.InteractionSize;
		SlateControl->MainInputKey = Control.MainInputKey;
		SlateControl->AltInputKey = Control.AltInputKey;
	}

	// set them as active!
	VirtualJoystick->SetControls(SlateControls);
}
