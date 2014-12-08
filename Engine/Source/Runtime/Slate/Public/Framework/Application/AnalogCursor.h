// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class FSlateApplication;

/**
 * A class that simulates a cursor driven by an analog stick.
 */
class SLATE_API FAnalogCursor
{
public:
	FAnalogCursor();

	/** Dtor */
	virtual ~FAnalogCursor()
	{}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor);

	bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& InKeyEvent);
	bool HandleAnalogInputEvent(FSlateApplication& SlateApp, const FAnalogInputEvent& InAnalogInputEvent);

protected:

	/** Getter */
	FORCEINLINE FVector2D GetAnalogValues() const
	{
		return AnalogValues;
	}

	/** Handles updating the cursor position and processing a Mouse Move Event */
	void UpdateCursorPosition(FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor, const FVector2D& NewPosition);

	/** Current speed of the cursor */
	FVector2D CurrentSpeed;

	/** Helpful statics */
	static const float DefaultAcceleration;
	static const float DefaultMaxSpeed;

private:

	/** Input from the gamepad */
	FVector2D AnalogValues;
};

