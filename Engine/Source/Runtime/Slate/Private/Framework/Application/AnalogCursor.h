// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSlateApplication;

/**
 * A class that simulates a cursor driven by an analog stick.
 */
class FAnalogCursor
{
public:
	FAnalogCursor();

	void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor);

	bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent);

private:

	FVector2D CurrentPos;

	FVector2D AnalogValues;
	FVector2D CurrentSpeed;

	static const float Acceleration;
	static const float Decceleration;
	static const float MaxSpeed;
	static const float OverWidgetMultiplier;
};

