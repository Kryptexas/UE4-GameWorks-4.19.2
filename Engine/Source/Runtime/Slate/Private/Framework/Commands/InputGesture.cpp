// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	InputGesture.cpp: Implements the FInputGesture class.
=============================================================================*/

#include "Slate.h"


#define LOCTEXT_NAMESPACE "FInputGesture"


/* FInputGesture interface
 *****************************************************************************/

/**
 * Returns the friendly, localized string name of this key binding
 * @todo Slate: Got to be a better way to do this
 */
FText FInputGesture::GetInputText( ) const
{
	FText FormatString = LOCTEXT("ShortcutKeyWithNoAdditionalModifierKeys", "{Key}");

	if ( bCtrl && !bAlt && !bShift )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command", "Command+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control", "Ctrl+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && !bShift )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Alt", "Command+Alt+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Alt", "Ctrl+Alt+{Key}");
#endif
	}
	else if ( bCtrl && !bAlt && bShift )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Shift", "Command+Shift+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Shift", "Ctrl+Shift+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && bShift )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Alt + KeyName_Shift", "Command+Alt+Shift+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Alt + KeyName_Shift", "Ctrl+Alt+Shift+{Key}");
#endif
	}
	else if ( !bCtrl && bAlt && !bShift )
	{
		FormatString = LOCTEXT("KeyName_Alt", "Alt+{Key}");
	}
	else if ( !bCtrl && bAlt && bShift )
	{
		FormatString = LOCTEXT("KeyName_Alt + KeyName_Shift", "Alt+Shift+{Key}");
	}
	else if ( !bCtrl && !bAlt && bShift )
	{
		FormatString = LOCTEXT("KeyName_Shift", "Shift+{Key}");
	}

	FFormatNamedArguments Args;
	Args.Add( TEXT("Key"), GetKeyText() );

	return FText::Format( FormatString, Args );
}


FText FInputGesture::GetKeyText( ) const
{
	FText OutString; // = KeyGetDisplayName(Key);

	if (Key.IsValid() && !Key.IsModifierKey())
	{
		OutString = Key.GetDisplayName();
	}

	return OutString;
}


#undef LOCTEXT_NAMESPACE
