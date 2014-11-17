// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPlatformTextField.h"

class FGenericPlatformTextField : public IPlatformTextField
{
public:
	virtual void ShowVirtualKeyboard(bool bShow, TSharedPtr<IVirtualKeyboardEntry> TextEntryWidget) override {};

};

typedef FGenericPlatformTextField FPlatformTextField;
