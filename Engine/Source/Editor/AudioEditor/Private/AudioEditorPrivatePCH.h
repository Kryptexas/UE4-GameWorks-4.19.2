// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealEd.h"
#include "AudioEditorModule.h"
#include "SoundDefinitions.h"
#include "AudioEditor.h"

#include "SoundClassEditor.h"
#include "SoundCueEditor.h"
#include "SoundCueGraphConnectionDrawingPolicy.h"
#include "SoundCueGraphNodeFactory.h"
#include "SoundCueGraphEditorCommands.h"

// Engine audio things
#include "Sound/SoundClass.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeDialoguePlayer.h"
#include "Sound/DialogueWave.h"

// Sound class graph
#include "SoundClassGraph/SoundClassGraph.h"
#include "SoundClassGraph/SoundClassGraphNode.h"
#include "SoundClassGraph/SoundClassGraphSchema.h"

// Sound cue graph
#include "SoundCueGraph/SoundCueGraph.h"
#include "SoundCueGraph/SoundCueGraphNode.h"
#include "SoundCueGraph/SoundCueGraphNode_Base.h"
#include "SoundCueGraph/SoundCueGraphNode_Root.h"
#include "SoundCueGraph/SoundCueGraphSchema.h"

// Factories
#include "Factories/DialogueVoiceFactory.h"
#include "Factories/DialogueWaveFactory.h"
#include "Factories/ReimportSoundFactory.h"
#include "Factories/ReimportSoundSurroundFactory.h"
#include "Factories/ReverbEffectFactory.h"
#include "Factories/SoundAttenuationFactory.h"
#include "Factories/SoundClassFactory.h"
#include "Factories/SoundConcurrencyFactory.h"
#include "Factories/SoundCueFactoryNew.h"
#include "Factories/SoundFactory.h"
#include "Factories/SoundMixFactory.h"
#include "Factories/SoundSurroundFactory.h"

// Private includes
#include "SGraphNodeSoundBase.h"
#include "SGraphNodeSoundResult.h"
#include "SoundClassEditor.h"
#include "SoundCueEditor.h"
#include "SoundCueGraphConnectionDrawingPolicy.h"
#include "SoundCueGraphNodeFactory.h"
#include "SSoundClassActionMenu.h"
#include "SSoundCuePalette.h"

// Asset type actions
#include "AssetTypeActions/AssetTypeActions_DialogueVoice.h"
#include "AssetTypeActions/AssetTypeActions_DialogueWave.h"
#include "AssetTypeActions/AssetTypeActions_ReverbEffect.h"
#include "AssetTypeActions/AssetTypeActions_SoundAttenuation.h"
#include "AssetTypeActions/AssetTypeActions_SoundBase.h"
#include "AssetTypeActions/AssetTypeActions_SoundClass.h"
#include "AssetTypeActions/AssetTypeActions_SoundConcurrency.h"
#include "AssetTypeActions/AssetTypeActions_SoundCue.h"
#include "AssetTypeActions/AssetTypeActions_SoundMix.h"
#include "AssetTypeActions/AssetTypeActions_SoundWave.h"