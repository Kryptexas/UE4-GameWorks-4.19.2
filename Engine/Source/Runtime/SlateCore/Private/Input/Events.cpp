// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "SlateCorePrivatePCH.h"


/* Static initialization
 *****************************************************************************/

const FTouchKeySet FTouchKeySet::StandardSet(EKeys::LeftMouseButton);
const FTouchKeySet FTouchKeySet::EmptySet(EKeys::Invalid);

FText FInputEvent::ToText() const
{
	return NSLOCTEXT("Events", "Unimplemented", "Unimplemented");
}

FText FCharacterEvent::ToText() const
{
	return FText::Format( NSLOCTEXT("Events","Char","Char({0})"), FText::FromString(FString(1, &Character)) );
}

FText FControllerEvent::ToText() const
{
	return NSLOCTEXT("Events","Gamepad","Gamepad()");
}

FText FKeyboardEvent::ToText() const
{
	return FText::Format( NSLOCTEXT("Events","Key","Key({0})"), Key.GetDisplayName() );
}

FText FPointerEvent::ToText() const
{
	return FText::Format( NSLOCTEXT("Events","Pointer","Pointer({0}, {1})"), EffectingButton.GetDisplayName() );
}

