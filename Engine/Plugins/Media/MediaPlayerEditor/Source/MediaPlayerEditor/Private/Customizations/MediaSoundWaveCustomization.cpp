// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "Customizations/MediaSoundWaveCustomization.h"
#include "Sound/SoundWave.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"

/* IDetailCustomization interface
 *****************************************************************************/

void FMediaSoundWaveCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	static const FName StreamingName(TEXT("Streaming"));
	DetailBuilder.HideCategory(StreamingName);

	static const FName ImportSettingsName(TEXT("File Path"));
	DetailBuilder.HideCategory(ImportSettingsName);
}
