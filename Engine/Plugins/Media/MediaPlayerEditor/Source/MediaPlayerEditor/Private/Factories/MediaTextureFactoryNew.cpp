// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaPlayerEditorPrivatePCH.h"


/* UMediaTextureFactoryNew structors
 *****************************************************************************/

UMediaTextureFactoryNew::UMediaTextureFactoryNew( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	SupportedClass = UMediaTexture::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


/* UFactory overrides
 *****************************************************************************/

UObject* UMediaTextureFactoryNew::FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn )
{
	UMediaTexture* MediaTexture = ConstructObject<UMediaTexture>(InClass, InParent, InName, Flags);

	if ((MediaTexture != nullptr) && (InitialMediaPlayer != nullptr))
	{
		MediaTexture->SetMediaPlayer(InitialMediaPlayer);
	}

	return MediaTexture;
}


bool UMediaTextureFactoryNew::ShouldShowInNewMenu( ) const
{
	return true;
}
