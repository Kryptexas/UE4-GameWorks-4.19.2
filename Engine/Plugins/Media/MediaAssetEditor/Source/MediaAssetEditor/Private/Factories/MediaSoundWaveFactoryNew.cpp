// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"


/* UMediaSoundWaveFactoryNew structors
 *****************************************************************************/

UMediaSoundWaveFactoryNew::UMediaSoundWaveFactoryNew( const class FPostConstructInitializeProperties& PCIP )
	: Super(PCIP)
{
	SupportedClass = UMediaSoundWave::StaticClass();
	bCreateNew = true;
	bEditAfterNew = true;
}


/* UFactory overrides
 *****************************************************************************/

UObject* UMediaSoundWaveFactoryNew::FactoryCreateNew( UClass* InClass, UObject* InParent, FName InName, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn )
{
	UMediaSoundWave* MediaSoundWave = ConstructObject<UMediaSoundWave>(InClass, InParent, InName, Flags);

	if ((MediaSoundWave != nullptr) && (InitialMediaAsset != nullptr))
	{
		MediaSoundWave->SetMediaAsset(InitialMediaAsset);
	}

	return MediaSoundWave;
}


bool UMediaSoundWaveFactoryNew::ShouldShowInNewMenu( ) const
{
	return true;
}
