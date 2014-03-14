// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPlatformTextField.h"

class FGenericPlatformTextField : public IPlatformTextField
{
public:
	virtual void ShowKeyboard(bool bShow, TSharedPtr<SVirtualKeyboardEntry> TextEntryWidget) OVERRIDE {};

};

typedef FGenericPlatformTextField FPlatformTextField;
