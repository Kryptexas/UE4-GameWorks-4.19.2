// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * A class that simulates a cursor driven by an analog stick.
 */
class FAnalogCursor
{
public:
	FAnalogCursor();

	void Tick(const float DeltaTime, TSharedPtr<ICursor> Cursor);

	bool HandleReleased(const FKeyEvent& KeyEvent);
	bool HandleAnalog(const FAnalogInputEvent& InputEvent);

private:

	FVector2D AnalogValues;
	FVector2D CurrentSpeed;

	static const float Acceleration;
	static const float Decceleration;
	static const float MaxSpeed;
};

