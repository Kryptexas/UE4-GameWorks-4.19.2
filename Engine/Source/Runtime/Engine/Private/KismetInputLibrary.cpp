// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "EnginePrivate.h"
#include "MessageLog.h"
#include "UObjectToken.h"


//////////////////////////////////////////////////////////////////////////
// UKismetInputLibrary

#define LOCTEXT_NAMESPACE "KismetInputLibrary"


UKismetInputLibrary::UKismetInputLibrary(const class FPostConstructInitializeProperties& PCIP)
	: Super(PCIP)
{
}

void UKismetInputLibrary::CalibrateTilt()
{
	GEngine->Exec(NULL, TEXT("CALIBRATEMOTION"));
}

FKey UKismetInputLibrary::GetKey(const FKeyboardEvent& Input)
{
	return Input.GetKey();
}

bool UKismetInputLibrary::EqualEqual_KeyKey(FKey A, FKey B)
{
	return A == B;
}

bool UKismetInputLibrary::InputEvent_IsRepeat(const FInputEvent& Input)
{
	return Input.IsRepeat();
}

bool UKismetInputLibrary::InputEvent_IsShiftDown(const FInputEvent& Input)
{
	return Input.IsShiftDown();
}

bool UKismetInputLibrary::InputEvent_IsLeftShiftDown(const FInputEvent& Input)
{
	return Input.IsLeftShiftDown();
}

bool UKismetInputLibrary::InputEvent_IsRightShiftDown(const FInputEvent& Input)
{
	return Input.IsRightShiftDown();
}

bool UKismetInputLibrary::InputEvent_IsControlDown(const FInputEvent& Input)
{
	return Input.IsControlDown();
}

bool UKismetInputLibrary::InputEvent_IsLeftControlDown(const FInputEvent& Input)
{
	return Input.IsLeftControlDown();
}

bool UKismetInputLibrary::InputEvent_IsRightControlDown(const FInputEvent& Input)
{
	return Input.IsRightControlDown();
}

bool UKismetInputLibrary::InputEvent_IsAltDown(const FInputEvent& Input)
{
	return Input.IsAltDown();
}

bool UKismetInputLibrary::InputEvent_IsLeftAltDown(const FInputEvent& Input)
{
	return Input.IsLeftAltDown();
}

bool UKismetInputLibrary::InputEvent_IsRightAltDown(const FInputEvent& Input)
{
	return Input.IsRightAltDown();
}

bool UKismetInputLibrary::InputEvent_IsCommandDown(const FInputEvent& Input)
{
	return Input.IsCommandDown();
}

bool UKismetInputLibrary::InputEvent_IsLeftCommandDown(const FInputEvent& Input)
{
	return Input.IsLeftCommandDown();
}

bool UKismetInputLibrary::InputEvent_IsRightCommandDown(const FInputEvent& Input)
{
	return Input.IsRightCommandDown();
}

#undef LOCTEXT_NAMESPACE