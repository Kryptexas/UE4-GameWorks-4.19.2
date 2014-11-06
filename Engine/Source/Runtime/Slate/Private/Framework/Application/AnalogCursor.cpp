// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"
#include "AnalogCursor.h"

const float FAnalogCursor::Acceleration     = 50.0f;
const float FAnalogCursor::Decceleration    = 100.0f;
const float FAnalogCursor::MaxSpeed         = 100.0f;

FAnalogCursor::FAnalogCursor()
{

}

void FAnalogCursor::Tick(const float DeltaTime, TSharedPtr<ICursor> Cursor)
{
	float CurrentMaxSpeedX = FMath::Abs(AnalogValues.X) * MaxSpeed;
	float CurrentMaxSpeedY = FMath::Abs(AnalogValues.Y) * MaxSpeed;

	CurrentSpeed += AnalogValues * Acceleration * DeltaTime;
	CurrentSpeed.X = FMath::Clamp(CurrentSpeed.X, -CurrentMaxSpeedX, CurrentMaxSpeedX);
	CurrentSpeed.Y = FMath::Clamp(CurrentSpeed.Y, -CurrentMaxSpeedY, CurrentMaxSpeedY);

	FVector2D Pos = Cursor->GetPosition();
	Pos += CurrentSpeed;
	Cursor->SetPosition(Pos.X, Pos.Y);
}

bool FAnalogCursor::HandleReleased(const FKeyEvent& KeyEvent)
{
	FKey Key = KeyEvent.GetKey();
	if (Key == EKeys::Gamepad_LeftX)
	{
		AnalogValues.X = 0.0f;
	}
	else if (Key == EKeys::Gamepad_LeftY)
	{
		AnalogValues.Y = 0.0f;
	}
	else
	{
		return false;
	}
	return true;
}

bool FAnalogCursor::HandleAnalog(const FAnalogInputEvent& InputEvent)
{
	FKey Key = InputEvent.GetKey();
	float AnalogValue = InputEvent.GetAnalogValue();
	if (FMath::Abs(AnalogValue) < 0.1f)
	{
		AnalogValue = 0.0f;
	}
	if (Key == EKeys::Gamepad_LeftX)
	{
		AnalogValues.X = AnalogValue;
	}
	else if (Key == EKeys::Gamepad_LeftY)
	{
		AnalogValues.Y = AnalogValue;
	}
	else
	{
		return false;
	}
	return true;
}