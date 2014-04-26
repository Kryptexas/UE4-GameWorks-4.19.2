// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IPlatformTextField.h"

#import <UIKit/UIKit.h>

@class SlateTextField;

class FIOSPlatformTextField : public IPlatformTextField
{
public:
	FIOSPlatformTextField();
	virtual ~FIOSPlatformTextField();

	virtual void ShowKeyboard(bool bShow, TSharedPtr<SVirtualKeyboardEntry> TextEntryWidget) OVERRIDE;

private:
	SlateTextField* TextField;
};

typedef FIOSPlatformTextField FPlatformTextField;


@interface SlateTextField : NSObject<UIAlertViewDelegate>
{
	TSharedPtr<SVirtualKeyboardEntry> TextWidget;
}

-(void)show:(TSharedPtr<SVirtualKeyboardEntry>)InTextWidget;

@end
