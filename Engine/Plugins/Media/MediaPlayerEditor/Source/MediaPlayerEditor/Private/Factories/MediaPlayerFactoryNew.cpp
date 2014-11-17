// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayerEditorPrivatePCH.h"


/* UMediaPlayerFactoryNew structors
 *****************************************************************************/

UMediaPlayerFactoryNew::UMediaPlayerFactoryNew( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	SupportedClass = UMediaPlayer::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


/* UFactory overrides
 *****************************************************************************/

UObject* UMediaPlayerFactoryNew::FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn )
{
	return CastChecked<UMediaPlayer>(StaticConstructObject(InClass, InParent, InName, Flags));
}


bool UMediaPlayerFactoryNew::ShouldShowInNewMenu( ) const
{
	return true;
}
