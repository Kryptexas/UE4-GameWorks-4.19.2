// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlatePrivatePCH.h"


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

	if ( bCtrl && !bAlt && !bShift && !bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command", "Cmd+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control", "Ctrl+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && !bShift && !bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Alt", "Cmd+Alt+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Alt", "Ctrl+Alt+{Key}");
#endif
	}
	else if ( bCtrl && !bAlt && bShift && !bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Shift", "Cmd+Shift+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Shift", "Ctrl+Shift+{Key}");
#endif
	}
	else if ( bCtrl && !bAlt && !bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Control", "Cmd+Ctrl+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Command", "Ctrl+Cmd+{Key}");
#endif
	}
	else if ( !bCtrl && bAlt && !bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Control + KeyName_Alt", "Ctrl+Alt+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Command + KeyName_Alt", "Cmd+Alt+{Key}");
#endif
	}
	else if ( !bCtrl && !bAlt && bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Control + KeyName_Shift", "Ctrl+Shift+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Command + KeyName_Shift", "Cmd+Shift+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && bShift && !bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Alt + KeyName_Shift", "Cmd+Alt+Shift+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Alt + KeyName_Shift", "Ctrl+Alt+Shift+{Key}");
#endif
	}
	else if ( !bCtrl && bAlt && bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Control + KeyName_Alt + KeyName_Shift", "Ctrl+Alt+Shift+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Command + KeyName_Alt + KeyName_Shift", "Cmd+Alt+Shift+{Key}");
#endif
	}
	else if ( bCtrl && !bAlt && bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Control + KeyName_Shift", "Cmd+Ctrl+Shift+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Command + KeyName_Shift", "Ctrl+Cmd+Shift+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && !bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Control + KeyName_Alt", "Cmd+Ctrl+Alt+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Command + KeyName_Alt", "Ctrl+Cmd+Alt+{Key}");
#endif
	}
	else if ( bCtrl && bAlt && bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Command + KeyName_Control + KeyName_Alt + KeyName_Shift", "Cmd+Ctrl+Alt+Shift{Key}");
#else
		FormatString = LOCTEXT("KeyName_Control + KeyName_Command + KeyName_Alt + KeyName_Shift", "Ctrl+Cmd+Alt+Shift{Key}");
#endif
	}
	else if ( !bCtrl && bAlt && !bShift && !bCmd )
	{
		FormatString = LOCTEXT("KeyName_Alt", "Alt+{Key}");
	}
	else if ( !bCtrl && bAlt && bShift && !bCmd )
	{
		FormatString = LOCTEXT("KeyName_Alt + KeyName_Shift", "Alt+Shift+{Key}");
	}
	else if ( !bCtrl && !bAlt && bShift && !bCmd )
	{
		FormatString = LOCTEXT("KeyName_Shift", "Shift+{Key}");
	}
	else if ( !bCtrl && !bAlt && !bShift && bCmd )
	{
#if PLATFORM_MAC
		FormatString = LOCTEXT("KeyName_Control", "Ctrl+{Key}");
#else
		FormatString = LOCTEXT("KeyName_Command", "Cmd+{Key}");
#endif
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
