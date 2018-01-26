// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "Factories/SoundFactory.h"
#include "AssetRegistryModule.h"
#include "Components/AudioComponent.h"
#include "Audio.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundNode.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundNodeModulator.h"
#include "Sound/SoundNodeAttenuation.h"
#include "AudioDeviceManager.h"
#include "Editor.h"
#include "Misc/MessageDialog.h"
#include "Misc/FeedbackContext.h"
#include "EditorFramework/AssetImportData.h"

static bool bSoundFactorySuppressImportOverwriteDialog = false;

static void InsertSoundNode(USoundCue* SoundCue, UClass* NodeClass, int32 NodeIndex)
{
	USoundNode* SoundNode = SoundCue->ConstructSoundNode<USoundNode>(NodeClass);

	// If this node allows >0 children but by default has zero - create a connector for starters
	if (SoundNode->GetMaxChildNodes() > 0 && SoundNode->ChildNodes.Num() == 0)
	{
		SoundNode->CreateStartingConnectors();
	}

	SoundNode->GraphNode->NodePosX = -150 * NodeIndex - 100;
	SoundNode->GraphNode->NodePosY = -35;

	// Link the node to the cue.
	SoundNode->ChildNodes[0] = SoundCue->FirstNode;

	// Link the attenuation node to root.
	SoundCue->FirstNode = SoundNode;

	SoundCue->LinkGraphNodesFromSoundNodes();
}

static void CreateSoundCue(USoundWave* Sound, UObject* InParent, EObjectFlags Flags, bool bIncludeAttenuationNode, bool bIncludeModulatorNode, bool bIncludeLoopingNode, float CueVolume)
{
	// then first create the actual sound cue
	FString SoundCueName = FString::Printf(TEXT("%s_Cue"), *Sound->GetName());

	// Create sound cue and wave player
	USoundCue* SoundCue = NewObject<USoundCue>(InParent, *SoundCueName, Flags);
	USoundNodeWavePlayer* WavePlayer = SoundCue->ConstructSoundNode<USoundNodeWavePlayer>();

	int32 NodeIndex = (int32)bIncludeAttenuationNode + (int32)bIncludeModulatorNode + (int32)bIncludeLoopingNode;

	WavePlayer->GraphNode->NodePosX = -150 * NodeIndex - 100;
	WavePlayer->GraphNode->NodePosY = -35;

	// Apply the initial volume.
	SoundCue->VolumeMultiplier = CueVolume;

	WavePlayer->SetSoundWave(Sound);
	SoundCue->FirstNode = WavePlayer;
	SoundCue->LinkGraphNodesFromSoundNodes();

	if (bIncludeLoopingNode)
	{
		WavePlayer->bLooping = true;
	}

	if (bIncludeModulatorNode)
	{
		InsertSoundNode(SoundCue, USoundNodeModulator::StaticClass(), --NodeIndex);
	}

	if (bIncludeAttenuationNode)
	{
		InsertSoundNode(SoundCue, USoundNodeAttenuation::StaticClass(), --NodeIndex);
	}

	// Make sure the content browser finds out about this newly-created object.  This is necessary when sound
	// cues are created automatically after creating a sound node wave.  See use of bAutoCreateCue in USoundTTSFactory.
	if ((Flags & (RF_Public | RF_Standalone)) != 0)
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(SoundCue);
	}
}


USoundFactory::USoundFactory(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SupportedClass = USoundWave::StaticClass();
	Formats.Add(TEXT("wav;Sound"));

	bCreateNew = false;
	bAutoCreateCue = false;
	bIncludeAttenuationNode = false;
	bIncludeModulatorNode = false;
	bIncludeLoopingNode = false;
	CueVolume = 0.75f;
	CuePackageSuffix = TEXT("_Cue");
	bEditorImport = true;
} 

UObject* USoundFactory::FactoryCreateBinary
(
	UClass*				Class,
	UObject*			InParent,
	FName				Name,
	EObjectFlags		Flags,
	UObject*			Context,
	const TCHAR*		FileType,
	const uint8*&		Buffer,
	const uint8*			BufferEnd,
	FFeedbackContext*	Warn
	)
{
	FEditorDelegates::OnAssetPreImport.Broadcast(this, Class, InParent, Name, FileType);

	if (FCString::Stricmp(FileType, TEXT("WAV")) == 0)
	{
		// create the group name for the cue
		const FString GroupName = InParent->GetFullGroupName(false);
		FString CuePackageName = InParent->GetOutermost()->GetName();
		CuePackageName += CuePackageSuffix;
		if (GroupName.Len() > 0 && GroupName != TEXT("None"))
		{
			CuePackageName += TEXT(".");
			CuePackageName += GroupName;
		}

		// validate the cue's group
		FText Reason;
		const bool bCuePathIsValid = FName(*CuePackageSuffix).IsValidGroupName(Reason);
		const bool bMoveCue = CuePackageSuffix.Len() > 0 && bCuePathIsValid && bAutoCreateCue;
		if (bAutoCreateCue)
		{
			if (!bCuePathIsValid)
			{
				FMessageDialog::Open(EAppMsgType::Ok, FText::Format(NSLOCTEXT("UnrealEd", "Error_ImportFailed_f", "Import failed for {0}: {1}"), FText::FromString(CuePackageName), Reason));
				FEditorDelegates::OnAssetPostImport.Broadcast(this, nullptr);
				return nullptr;
			}
		}

		// if we are creating the cue move it when necessary
		UPackage* CuePackage = bMoveCue ? CreatePackage(nullptr, *CuePackageName) : nullptr;

		// if the sound already exists, remember the user settings
		USoundWave* ExistingSound = FindObject<USoundWave>(InParent, *Name.ToString());

		TArray<UAudioComponent*> ComponentsToRestart;
		FAudioDeviceManager* AudioDeviceManager = GEngine->GetAudioDeviceManager();
		if (AudioDeviceManager && ExistingSound)
		{
			// Will block internally on audio thread completing outstanding commands
			AudioDeviceManager->StopSoundsUsingResource(ExistingSound, &ComponentsToRestart);
		}

		bool bUseExistingSettings = bSoundFactorySuppressImportOverwriteDialog;

		if (ExistingSound && !bSoundFactorySuppressImportOverwriteDialog && !GIsAutomationTesting)
		{
			DisplayOverwriteOptionsDialog(FText::Format(
				NSLOCTEXT("SoundFactory", "ImportOverwriteWarning", "You are about to import '{0}' over an existing sound."),
				FText::FromName(Name)));

			switch (OverwriteYesOrNoToAllState)
			{

			case EAppReturnType::Yes:
			case EAppReturnType::YesAll:
			{
				// Overwrite existing settings
				bUseExistingSettings = false;
				break;
			}
			case EAppReturnType::No:
			case EAppReturnType::NoAll:
			{
				// Preserve existing settings
				bUseExistingSettings = true;
				break;
			}
			default:
			{
				FEditorDelegates::OnAssetPostImport.Broadcast(this, nullptr);
				return nullptr;
			}
			}
		}

		// See if this may be an ambisonics import by checking ambisonics naming convention (ambix)
		FString RootName = Name.GetPlainNameString();
		FString AmbiXTag = RootName.Right(6).ToLower();
		FString FuMaTag = RootName.Right(5).ToLower();

		// check for AmbiX or FuMa tag for the file
		bool bIsAmbiX = (AmbiXTag == TEXT("_ambix"));
		bool bIsFuMa = (FuMaTag == TEXT("_fuma"));

		// Reset the flag back to false so subsequent imports are not suppressed unless the code explicitly suppresses it
		bSoundFactorySuppressImportOverwriteDialog = false;

		TArray<uint8> RawWaveData;
		RawWaveData.Empty(BufferEnd - Buffer);
		RawWaveData.AddUninitialized(BufferEnd - Buffer);
		FMemory::Memcpy(RawWaveData.GetData(), Buffer, RawWaveData.Num());

		// Read the wave info and make sure we have valid wave data
		FWaveModInfo WaveInfo;
		FString ErrorMessage;
		if (WaveInfo.ReadWaveInfo(RawWaveData.GetData(), RawWaveData.Num(), &ErrorMessage))
		{
			// Validate if somebody has used the ambiX or FuMa tag that the ChannelCount is 4 channels
			if ((bIsAmbiX || bIsFuMa) && (int32)*WaveInfo.pChannels != 4)
			{
				Warn->Logf(ELogVerbosity::Error, TEXT("Tried to import ambisonics format file but requires exactly 4 channels: '%s'"), *Name.ToString());
				FEditorDelegates::OnAssetPostImport.Broadcast(this, nullptr);
				return nullptr;
			}

			if (*WaveInfo.pBitsPerSample != 16)
			{
				WaveInfo.ReportImportFailure();
				Warn->Logf(ELogVerbosity::Error, TEXT("Currently, only 16 bit WAV files are supported (%s)."), *Name.ToString());
				FEditorDelegates::OnAssetPostImport.Broadcast(this, nullptr);
				return nullptr;
			}
		}
		else
		{
			Warn->Logf(ELogVerbosity::Error, TEXT("Unable to read wave file '%s' - \"%s\""), *Name.ToString(), *ErrorMessage);
			FEditorDelegates::OnAssetPostImport.Broadcast(this, nullptr);
			return nullptr;
		}


		// Use pre-existing sound if it exists and we want to keep settings,
		// otherwise create new sound and import raw data.
		USoundWave* Sound = (bUseExistingSettings && ExistingSound) ? ExistingSound : NewObject<USoundWave>(InParent, Name, Flags);

		if (bUseExistingSettings && ExistingSound)
		{
			// Clear resources so that if it's already been played, it will reload the wave data
			Sound->FreeResources();
		}

		// Store the current file path and timestamp for re-import purposes
		Sound->AssetImportData->Update(CurrentFilename);

		// Compressed data is now out of date.
		Sound->InvalidateCompressedData();
		 
		// If we're a multi-channel file, we're going to spoof the behavior of the SoundSurroundFactory
		int32 ChannelCount = (int32)*WaveInfo.pChannels;
		check(ChannelCount >0);

		int32 NumSamples = WaveInfo.SampleDataSize / sizeof(int16);
		int32 NumFrames = NumSamples / ChannelCount;

		if (ChannelCount > 2)
		{
			// We need to deinterleave the raw PCM data in the multi-channel file reuse a scratch buffer
			TArray<int16> DeinterleavedAudioScratchBuffer;

			// Store the array of raw .wav files we're going to create from the deinterleaved int16 data
			TArray<uint8> RawChannelWaveData[SPEAKER_Count];

			// Ptr to the pcm data of the imported sound wave
			int16* SampleDataBuffer = (int16*)WaveInfo.SampleDataStart;

			int32 TotalSize = 0;

			Sound->ChannelOffsets.Empty(SPEAKER_Count);
			Sound->ChannelOffsets.AddZeroed(SPEAKER_Count);

			Sound->ChannelSizes.Empty(SPEAKER_Count);
			Sound->ChannelSizes.AddZeroed(SPEAKER_Count);

			TArray<int32> ChannelIndices;
			if (ChannelCount == 4)
			{
				ChannelIndices = { 
					SPEAKER_FrontLeft, 
					SPEAKER_FrontRight, 
					SPEAKER_LeftSurround, 
					SPEAKER_RightSurround 
				};
			}
			else if (ChannelCount == 6)
			{
				ChannelIndices = { 
					SPEAKER_FrontLeft, 
					SPEAKER_FrontRight, 
					SPEAKER_FrontCenter, 
					SPEAKER_LeftSurround, 
					SPEAKER_LowFrequency, 
					SPEAKER_LeftSurround, 
					SPEAKER_RightSurround 
				};
			}
			else if (ChannelCount == 8)
			{
				ChannelIndices = { 
					SPEAKER_FrontLeft, 
					SPEAKER_FrontRight, 
					SPEAKER_FrontCenter, 
					SPEAKER_LeftSurround, 
					SPEAKER_LowFrequency, 
					SPEAKER_LeftSurround, 
					SPEAKER_RightSurround,
					SPEAKER_LeftBack,
					SPEAKER_RightBack
				};
			}
			else
			{
				Warn->Logf(ELogVerbosity::Error, TEXT("Wave file '%s' has unsupported number of channels %d"), *Name.ToString(), ChannelCount);
				FEditorDelegates::OnAssetPostImport.Broadcast(this, nullptr);
				return nullptr;
			}

			// Make some new sound waves
			for (int32 Chan = 0; Chan < ChannelCount; ++Chan)
			{
				// Build the deinterleaved buffer for the channel
				DeinterleavedAudioScratchBuffer.Empty(NumFrames);
				for (int32 Frame = 0; Frame < NumFrames; ++Frame)
				{
					const int32 SampleIndex = Frame * ChannelCount + Chan;
					DeinterleavedAudioScratchBuffer.Add(SampleDataBuffer[SampleIndex]);
				}

				// Now create a sound wave asset
				SerializeWaveFile(RawChannelWaveData[Chan], (uint8*)DeinterleavedAudioScratchBuffer.GetData(), NumFrames * sizeof(int16), 1, *WaveInfo.pSamplesPerSec);

				// The current TotalSize is the "offset" into the bulk data for this sound wave
				Sound->ChannelOffsets[ChannelIndices[Chan]] = TotalSize;

				// "ChannelSize" is the size of the .wav file representing this channel of data
				const int32 ChannelSize = RawChannelWaveData[Chan].Num();

				// Store it in the sound wave
				Sound->ChannelSizes[ChannelIndices[Chan]] = ChannelSize;

				// TotalSize is the sum of all ChannelSizes
				TotalSize += ChannelSize;
			}

			// Now we have an array of mono .wav files in the format that the SoundSurroundFactory expects
			// copy the data into the bulk byte data

			// Get the raw data bulk byte pointer and copy over the .wav files we generated
			Sound->RawData.Lock(LOCK_READ_WRITE);

			uint8* LockedData = (uint8*)Sound->RawData.Realloc(TotalSize);
			int32 RawDataOffset = 0;


			if (bIsAmbiX || bIsFuMa)
			{
				check(ChannelCount == 4);

				// Flag that this is an ambisonics file
				Sound->bIsAmbisonics = true;
			}
			for (int32 Chan = 0; Chan < ChannelCount; ++Chan)
			{
				const int32 ChannelSize = RawChannelWaveData[Chan].Num();
				FMemory::Memcpy(LockedData + RawDataOffset, RawChannelWaveData[Chan].GetData(), ChannelSize);
				RawDataOffset += ChannelSize;
			}

			Sound->RawData.Unlock();
		}
		else
		{
			// For mono and stereo assets, just copy the data into the buffer
			Sound->RawData.Lock(LOCK_READ_WRITE);
			void* LockedData = Sound->RawData.Realloc(BufferEnd - Buffer);
			FMemory::Memcpy(LockedData, Buffer, BufferEnd - Buffer);
			Sound->RawData.Unlock();
		}

		Sound->Duration = (float)NumFrames / *WaveInfo.pSamplesPerSec;
		Sound->SampleRate = *WaveInfo.pSamplesPerSec;
		Sound->NumChannels = ChannelCount;

		FEditorDelegates::OnAssetPostImport.Broadcast(this, Sound);

		if (ExistingSound && bUseExistingSettings)
		{
			// Call PostEditChange() to update text to speech
			Sound->PostEditChange();
		}

		// if we're auto creating a default cue
		if (bAutoCreateCue)
		{
			CreateSoundCue(Sound, bMoveCue ? CuePackage : InParent, Flags, bIncludeAttenuationNode, bIncludeModulatorNode, bIncludeLoopingNode, CueVolume);
		}

		for (int32 ComponentIndex = 0; ComponentIndex < ComponentsToRestart.Num(); ++ComponentIndex)
		{
			ComponentsToRestart[ComponentIndex]->Play();
		}

		Sound->bNeedsThumbnailGeneration = true;

		return Sound;
	}
	else
	{
		// Unrecognized.
		Warn->Logf(ELogVerbosity::Error, TEXT("Unrecognized sound format '%s' in %s"), FileType, *Name.ToString());
		FEditorDelegates::OnAssetPostImport.Broadcast(this, nullptr);
		return nullptr;
	}
}

void USoundFactory::SuppressImportOverwriteDialog()
{
	bSoundFactorySuppressImportOverwriteDialog = true;
}
