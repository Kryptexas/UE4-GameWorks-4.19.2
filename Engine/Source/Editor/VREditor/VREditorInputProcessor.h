// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Framework/Application/IInputProcessor.h"

class FVREditorMode;

class FVREditorInputProcessor : public IInputProcessor
{
public:
	FVREditorInputProcessor(FVREditorMode* InVRMode);
	virtual ~FVREditorInputProcessor();

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override;

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent) override;
	virtual bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent) override;
	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override;

private:
	FVREditorMode* VRMode;
};
