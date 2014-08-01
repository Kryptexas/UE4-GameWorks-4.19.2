// Copyright 1998-2014 Epic Games, Inc. All Rights Reserved.

#include "MediaAssetEditorPrivatePCH.h"


#define LOCTEXT_NAMESPACE "AssetTypeActions"


/* FAssetTypeActions_SoundBase overrides
 *****************************************************************************/

bool FMediaSoundWaveActions::CanFilter( )
{
	return true;
}


uint32 FMediaSoundWaveActions::GetCategories( )
{
	return EAssetTypeCategories::Sounds;
}


FText FMediaSoundWaveActions::GetName( ) const
{
	return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_MediaSoundWave", "Media Sound Wave");
}


UClass* FMediaSoundWaveActions::GetSupportedClass( ) const
{
	return UMediaSoundWave::StaticClass();
}


FColor FMediaSoundWaveActions::GetTypeColor( ) const
{
	return FColor(0, 0, 255);
}


bool FMediaSoundWaveActions::HasActions ( const TArray<UObject*>& InObjects ) const
{
	return false;
}


#undef LOCTEXT_NAMESPACE
