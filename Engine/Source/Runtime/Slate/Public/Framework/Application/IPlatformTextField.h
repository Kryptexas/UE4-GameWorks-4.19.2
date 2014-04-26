// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

class SVirtualKeyboardEntry;

class IPlatformTextField
{
public:
	virtual ~IPlatformTextField() {};

	virtual void ShowKeyboard(bool bShow, TSharedPtr<SVirtualKeyboardEntry> TextEntryWidget) = 0;

private:

};
