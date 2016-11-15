// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "SequenceRecorderPrivatePCH.h"
#include "SequenceRecorderDetailsCustomization.h"
#include "SequenceRecorderSettings.h"
#include "ISequenceRecorder.h"
#include "ModuleManager.h"

void FSequenceRecorderDetailsCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	ISequenceRecorder& RecorderModule = FModuleManager::Get().LoadModuleChecked<ISequenceRecorder>("SequenceRecorder");
	if (!RecorderModule.HasAudioRecorder())
	{
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, RecordAudio));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, AudioGain));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, AudioInputBufferSize));
		DetailBuilder.HideProperty(GET_MEMBER_NAME_CHECKED(USequenceRecorderSettings, AudioSubDirectory));
	}
}