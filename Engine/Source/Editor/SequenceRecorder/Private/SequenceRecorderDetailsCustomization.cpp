// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderDetailsCustomization.h"
#include "SequenceRecorderSettings.h"
#include "ISequenceRecorder.h"
#include "Modules/ModuleManager.h"
#include "DetailLayoutBuilder.h"

void FSequenceRecorderDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	ISequenceRecorder& RecorderModule = FModuleManager::Get().LoadModuleChecked<ISequenceRecorder>("SequenceRecorder");
	if (!RecorderModule.HasAudioRecorder())
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, RecordAudio));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, AudioGain));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, AudioSubDirectory));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, bSplitAudioChannelsIntoSeparateTracks));
	}
}
