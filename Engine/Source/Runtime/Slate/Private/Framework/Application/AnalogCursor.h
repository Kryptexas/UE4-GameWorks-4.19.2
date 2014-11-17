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

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor);

	bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent);


protected:

	/** Handles updating the cursor position and processing a Mouse Move Event */
	void UpdateCursorPosition(FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor, const FVector2D& NewPosition);

private:

	FVector2D AnalogValues;
	FVector2D CurrentSpeed;

	static const float Acceleration;
	static const float Decceleration;
	static const float MaxSpeed;
	static const float OverWidgetMultiplier;
};

