// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "VREditorModule.h"
#include "VREditorInputProcessor.h"
#include "VREditorMode.h"
#include "SLevelViewport.h"

FVREditorInputProcessor::FVREditorInputProcessor(FVREditorMode* InVRMode)
	: VRMode(InVRMode)
{
}

FVREditorInputProcessor::~FVREditorInputProcessor()
{
}

void FVREditorInputProcessor::Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor)
{
}

bool FVREditorInputProcessor::HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	FEditorViewportClient* ViewportClient = VRMode->GetLevelViewportPossessedForVR().GetViewportClient().Get();
	FViewport* ActiveViewport = VRMode->GetLevelViewportPossessedForVR().GetActiveViewport();
	return VRMode->HandleInputKey(ViewportClient, ActiveViewport, InKeyEvent.GetKey(), InKeyEvent.IsRepeat() ? IE_Repeat : IE_Pressed);
}

bool FVREditorInputProcessor::HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent)
{
	FEditorViewportClient* ViewportClient = VRMode->GetLevelViewportPossessedForVR().GetViewportClient().Get();
	FViewport* ActiveViewport = VRMode->GetLevelViewportPossessedForVR().GetActiveViewport();
	return VRMode->HandleInputKey(ViewportClient, ActiveViewport, InKeyEvent.GetKey(), IE_Released);
}

bool FVREditorInputProcessor::HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent)
{
	FEditorViewportClient* ViewportClient = VRMode->GetLevelViewportPossessedForVR().GetViewportClient().Get();
	FViewport* ActiveViewport = VRMode->GetLevelViewportPossessedForVR().GetActiveViewport();
	return VRMode->HandleInputAxis(ViewportClient, ActiveViewport, InAnalogInputEvent.GetUserIndex(), InAnalogInputEvent.GetKey(), InAnalogInputEvent.GetAnalogValue(), FApp::GetDeltaTime());
}

bool FVREditorInputProcessor::HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent)
{
	FEditorViewportClient* ViewportClient = VRMode->GetLevelViewportPossessedForVR().GetViewportClient().Get();

	return false;
}
