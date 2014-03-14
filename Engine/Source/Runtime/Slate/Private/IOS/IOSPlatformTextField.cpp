// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "Slate.h"
#include "IOSPlatformTextField.h"

FIOSPlatformTextField::FIOSPlatformTextField()
	: TextField( NULL )
{
}

FIOSPlatformTextField::~FIOSPlatformTextField()
{
	if(TextField != NULL)
	{
		[TextField release];
		TextField = NULL;
	}
}

void FIOSPlatformTextField::ShowKeyboard(bool bShow, TSharedPtr<SVirtualKeyboardEntry> TextEntryWidget)
{
	if(TextField == NULL)
	{
		TextField = [SlateTextField alloc];
	}

	if(bShow)
	{
		// these functions must be run on the main thread
		dispatch_async(dispatch_get_main_queue(),^ {
			[TextField show: TextEntryWidget];
		} );
	}
}


@implementation SlateTextField

-(void)show:(TSharedPtr<SVirtualKeyboardEntry>)InTextWidget
{
	TextWidget = InTextWidget;

	UIAlertView* AlertView = [[UIAlertView alloc] initWithTitle:@""
													message:@""
													delegate:self
													cancelButtonTitle:NSLocalizedString(@"Cancel", nil)
													otherButtonTitles:NSLocalizedString(@"OK", nil), nil];

	// give the UIAlertView a style so a UITextField is created
	switch(TextWidget->GetKeyboardType())
	{
		case EKeyboardType::Keyboard_Password:
			AlertView.alertViewStyle = UIAlertViewStyleSecureTextInput;
			break;
		case EKeyboardType::Keyboard_Default:
		case EKeyboardType::Keyboard_Email:
		case EKeyboardType::Keyboard_Number:
		case EKeyboardType::Keyboard_Web:
		default:
			AlertView.alertViewStyle = UIAlertViewStylePlainTextInput;
			break;
	}

	UITextField* TextField = [AlertView textFieldAtIndex:0];
	TextField.clearsOnBeginEditing = NO;
	TextField.clearsOnInsertion = NO;
	TextField.autocorrectionType = UITextAutocorrectionTypeNo;
	TextField.autocapitalizationType = UITextAutocapitalizationTypeNone;
	TextField.text = [NSString stringWithFormat:@"%s" , TCHAR_TO_ANSI(*TextWidget->GetText().ToString())];
	TextField.placeholder = [NSString stringWithFormat:@"%s" , TCHAR_TO_ANSI(*TextWidget->GetHintText().ToString())];

	// set up the keyboard styles not supported in the AlertViewStyle styles
	switch(TextWidget->GetKeyboardType())
	{
		case EKeyboardType::Keyboard_Email:
			TextField.keyboardType = UIKeyboardTypeEmailAddress;
			break;
		case EKeyboardType::Keyboard_Number:
			TextField.keyboardType = UIKeyboardTypeNumberPad;
			break;
		case EKeyboardType::Keyboard_Web:
			TextField.keyboardType = UIKeyboardTypeURL;
			break;
		case EKeyboardType::Keyboard_Default:
		case EKeyboardType::Keyboard_Password:
		default:
			// nothing to do, UIAlertView style handles these keyboard types
			break;
	}

	[AlertView show];

	[AlertView release];
}

- (void)alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
	// index 1 is the OK button
	if(buttonIndex == 1)
	{
		UITextField* TextField = [alertView textFieldAtIndex:0];
		FString Text = ANSI_TO_TCHAR([TextField.text cStringUsingEncoding: NSASCIIStringEncoding]);
		TextWidget->SetText(FText::FromString(Text));
	}
}

@end

