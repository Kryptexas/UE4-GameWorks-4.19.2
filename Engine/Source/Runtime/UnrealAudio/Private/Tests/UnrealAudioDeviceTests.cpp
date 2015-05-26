// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#include "UnrealAudioPrivate.h"
#include "UnrealAudioModule.h"
#include "UnrealAudioDeviceModule.h"
#include "UnrealAudioTests.h"
#include "UnrealAudioTestGenerators.h"
#include "UnrealAudioSoundFile.h"

#if ENABLE_UNREAL_AUDIO

namespace UAudio
{
	static FUnrealAudioModule* UnrealAudioModule = nullptr;
	static bool bTestInitialized = false;

	void FUnrealAudioModule::InitializeTests(FUnrealAudioModule* Module)
	{
		UnrealAudioModule = Module;
		bTestInitialized = true;
	}

	bool TestDeviceQuery()
	{
		if (!bTestInitialized)
		{
			return false;
		}

		UE_LOG(LogUnrealAudio, Display, TEXT("Running audio device query test..."));

		IUnrealAudioDeviceModule* UnrealAudioDevice = UnrealAudioModule->GetDeviceModule();

		if (!UnrealAudioDevice)
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed: No Audio Device Object."));
			return false;
		}

		
		UE_LOG(LogUnrealAudio, Display, TEXT("Querying which audio device API we're using..."));
		EDeviceApi::Type DeviceApi;
		if (!UnrealAudioDevice->GetDevicePlatformApi(DeviceApi))
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed."));
			return false;
		}

		const TCHAR* ApiName = EDeviceApi::ToString(DeviceApi);
		UE_LOG(LogUnrealAudio, Display, TEXT("Success: Using %s"), ApiName);

		if (DeviceApi == EDeviceApi::DUMMY)
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("This is a dummy API. Platform not implemented yet."));
			return true;
		}

		UE_LOG(LogUnrealAudio, Display, TEXT("Querying the number of output devices for current system..."));
		uint32 NumOutputDevices = 0;
		if (!UnrealAudioDevice->GetNumOutputDevices(NumOutputDevices))
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed."));
			return false;
		}
		UE_LOG(LogUnrealAudio, Display, TEXT("Success: %d Output Devices Found"), NumOutputDevices);

		UE_LOG(LogUnrealAudio, Display, TEXT("Retrieving output device info for each output device..."));
		for (uint32 DeviceIndex = 0; DeviceIndex < NumOutputDevices; ++DeviceIndex)
		{
			FDeviceInfo DeviceInfo;
			if (!UnrealAudioDevice->GetOutputDeviceInfo(DeviceIndex, DeviceInfo))
			{
				UE_LOG(LogUnrealAudio, Display, TEXT("Failed for index %d."), DeviceIndex);
				return false;
			}

			UE_LOG(LogUnrealAudio, Display, TEXT("Device Index: %d"), DeviceIndex);
			UE_LOG(LogUnrealAudio, Display, TEXT("Device Name: %s"), *DeviceInfo.FriendlyName);
			UE_LOG(LogUnrealAudio, Display, TEXT("Device FrameRate: %d"), DeviceInfo.FrameRate);
			UE_LOG(LogUnrealAudio, Display, TEXT("Device NumChannels: %d"), DeviceInfo.NumChannels);
			UE_LOG(LogUnrealAudio, Display, TEXT("Device Default?: %s"), DeviceInfo.bIsSystemDefault ? TEXT("yes") : TEXT("no"));
			UE_LOG(LogUnrealAudio, Display, TEXT("Device Native Format: %s"), EStreamFormat::ToString(DeviceInfo.StreamFormat));

			UE_LOG(LogUnrealAudio, Display, TEXT("Speaker Array:"));

			for (int32 Channel = 0; Channel < DeviceInfo.Speakers.Num(); ++Channel)
			{
				UE_LOG(LogUnrealAudio, Display, TEXT("    %s"), ESpeaker::ToString(DeviceInfo.Speakers[Channel]));
			}
		}

		return true;
	}

	/**
	* TestCallback
	* Callback function used for all device test functions. 
	* Function takes input params and passes to the ITestGenerator object to generate output buffers.
	*/
	static bool TestCallback(FCallbackInfo& CallbackInfo)	
	{
		static int32 CurrentSecond = -1;
		int32 StreamSecond = (int32)CallbackInfo.StreamTime;
		if (StreamSecond != CurrentSecond)
		{
			if (CurrentSecond == -1)
			{
				UE_LOG(LogUnrealAudio, Display, TEXT("Stream Time (seconds):"));
			}
			CurrentSecond = StreamSecond;
			UE_LOG(LogUnrealAudio, Display, TEXT("%d"), CurrentSecond);
		}

		// Sets any data used by lots of different generators in static data struct
		Test::UpdateCallbackData(CallbackInfo);

		// Cast the user data object to our own FSimpleFM object (since we know we used that when we created the stream)
		Test::IGenerator* Generator = (Test::IGenerator*)CallbackInfo.UserData;

		// Get the next buffer of output data
		Generator->GetNextBuffer(CallbackInfo);
		return true;
	}

	static bool DoOutputTest(const FString& TestName, double LifeTime, Test::IGenerator* Generator)
	{
		check(Generator);

		if (!bTestInitialized)
		{
			return false;
		}

		UE_LOG(LogUnrealAudio, Display, TEXT("Running audio test: \"%s\"..."), *TestName);

		if (LifeTime > 0.0)
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Time of test: %d (seconds)"), LifeTime);
		}
		else
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Time of test: (indefinite)"));
		}

		IUnrealAudioDeviceModule* UnrealAudioDevice = UnrealAudioModule->GetDeviceModule();

		EDeviceApi::Type DeviceApi;
		if (!UnrealAudioDevice->GetDevicePlatformApi(DeviceApi))
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed."));
			return false;
		}
		if (DeviceApi == EDeviceApi::DUMMY)
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("This is a dummy API. Platform not implemented yet."));
			return true;
		}

		uint32 DefaultDeviceIndex = 0;
		if (!UnrealAudioDevice->GetDefaultOutputDeviceIndex(DefaultDeviceIndex))
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed to get default device index."));
			return false;
		}

		FDeviceInfo DeviceInfo;
		if (!UnrealAudioDevice->GetOutputDeviceInfo(DefaultDeviceIndex, DeviceInfo))
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed to get default device info."));
			return false;
		}

		FCreateStreamParams CreateStreamParams;
		CreateStreamParams.OutputDeviceIndex = DefaultDeviceIndex;
		CreateStreamParams.CallbackFunction = &TestCallback;
		CreateStreamParams.UserData = (void*)Generator;
		CreateStreamParams.CallbackBlockSize = 1024;

		UE_LOG(LogUnrealAudio, Display, TEXT("Creating the stream..."));
		if (!UnrealAudioDevice->CreateStream(CreateStreamParams))
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed to create a stream."));
			return false;
		}

		UE_LOG(LogUnrealAudio, Display, TEXT("Starting the stream."));
		UnrealAudioDevice->StartStream();

		// Block until the synthesizer is done
		while (!Generator->IsDone())
		{
			FPlatformProcess::Sleep(1);
		}

		// Stop the output stream (which blocks until its actually freed)
		UE_LOG(LogUnrealAudio, Display, TEXT("Shutting down the stream."));
		if (!UnrealAudioDevice->StopStream())
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed to stop the device stream."));
			return false;
		}

		if (!UnrealAudioDevice->ShutdownStream())
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Failed to shut down the device stream."));
			return false;
		}

		// At this point audio device I/O is done
		UE_LOG(LogUnrealAudio, Display, TEXT("Success!"));
		return true;
	}

	bool TestDeviceOutputSimple(double LifeTime)
	{
		Test::FSimpleOutput SimpleOutput(LifeTime);
		return DoOutputTest("output simple test", LifeTime, &SimpleOutput);
	}

	bool TestDeviceOutputRandomizedFm(double LifeTime)
	{
		Test::FPhaseModulator RandomizedFmGenerator(LifeTime);
		return DoOutputTest("output randomized FM synthesis", LifeTime, &RandomizedFmGenerator);
	}

	bool TestDeviceOutputNoisePan(double LifeTime)
	{
		Test::FNoisePan SimpleWhiteNoisePan(LifeTime);
		return DoOutputTest("output white noise panner", LifeTime, &SimpleWhiteNoisePan);
	}

	bool TestSourceImport(const FSoundFileImportSettings& ImportSettings)
	{
		UE_LOG(LogUnrealAudio, Display, TEXT("Testing exporting a single sound source."));

		UE_LOG(LogUnrealAudio, Display, TEXT("First importing."));
		UE_LOG(LogUnrealAudio, Display, TEXT("Import Path: %s"), *ImportSettings.SoundFilePath);
		UE_LOG(LogUnrealAudio, Display, TEXT("Import Format: %s"), *FString::Printf(TEXT("%s - %s"), ESoundFileFormat::ToStringMajor(ImportSettings.Format), ESoundFileFormat::ToStringMinor(ImportSettings.Format)));
		UE_LOG(LogUnrealAudio, Display, TEXT("Import SampleRate: %d"), ImportSettings.SampleRate);
		UE_LOG(LogUnrealAudio, Display, TEXT("Import EncodingQuality: %.2f"), ImportSettings.EncodingQuality);
		UE_LOG(LogUnrealAudio, Display, TEXT("Perform Peak Normalization: %s"), ImportSettings.bPerformPeakNormalization ? TEXT("Yes") : TEXT("No"));

		
		TSharedPtr<ISoundFile> ImportedSoundFile = UnrealAudioModule->ImportSound(ImportSettings);
		ESoundFileState::Type State = ImportedSoundFile->GetState();
		while (State != ESoundFileState::IMPORTED)
		{
			FPlatformProcess::Sleep(1);

			State = ImportedSoundFile->GetState();
			if (State == ESoundFileState::HAS_ERROR)
			{
				UE_LOG(LogUnrealAudio, Error, TEXT("Failed to import sound source!"));
				UE_LOG(LogUnrealAudio, Error, TEXT("Error: %s"), ESoundFileError::ToString(ImportedSoundFile->GetError()));
				return false;
			}
		}
		UE_LOG(LogUnrealAudio, Display, TEXT("Succeeded importing sound source!"));
		return true;
	}

	bool TestSourceImportExport(const FSoundFileImportSettings& ImportSettings)
	{
		//  Check if the soundfile path is a folder or a single file:
		if (FPaths::DirectoryExists(ImportSettings.SoundFilePath))
		{
			FString FolderPath = ImportSettings.SoundFilePath;
			UE_LOG(LogUnrealAudio, Display, TEXT("Testing import then export of all audio files in directory: %s."), *FolderPath);

			// Create the output exported directory if it doesn't exist
			FString OutputDir = FolderPath / "Exported";
			if (!FPaths::DirectoryExists(OutputDir))
			{
				FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*OutputDir);
			}

			// Get list of .wav files:
			TArray<FString> InputFiles;

			// Get input files in formats we're currently supporting
			IFileManager::Get().FindFiles(InputFiles, *(FolderPath / "*.wav"), true, false);
			IFileManager::Get().FindFiles(InputFiles, *(FolderPath / "*.aif"), true, false);
			IFileManager::Get().FindFiles(InputFiles, *(FolderPath / "*.flac"), true, false);
			IFileManager::Get().FindFiles(InputFiles, *(FolderPath / "*.ogg"), true, false);

			// Make an array of shared ptr sound files
			TArray<TSharedPtr<ISoundFile>> SoundFiles;

			// Array of sound files (names) that have finished importing
			TSet<FString> FinishedImporting;

			// Array of sound files (names) that have error'd out during import
			TSet<FString> HasError;

			FSoundFileImportSettings Settings = ImportSettings;

			// Import all the files!
			UE_LOG(LogUnrealAudio, Display, TEXT("Importing..."));
			for (FString& InputFile : InputFiles)
			{
				Settings.SoundFilePath = FolderPath / InputFile;
				UE_LOG(LogUnrealAudio, Display, TEXT("%s"), *InputFile);

				// start the import process and store the ISoundFile ptr in our array of in-flight imports
				TSharedPtr<ISoundFile> ImportedSoundFile = UnrealAudioModule->ImportSound(Settings);
				SoundFiles.Add(ImportedSoundFile);
			}

			// Loop through in-flight imports and start exports as soon as the file finishes importing
			bool bIsFinishedImporting = false;
			while (!bIsFinishedImporting)
			{
				// Assume we're done, but set false as soon as we find an import still going
				bIsFinishedImporting = true;

				// Loop through our stored sound files
				for (TSharedPtr<ISoundFile> SoundFile : SoundFiles)
				{
					// Get the sound file state, handle any errors that occurred during import
					// Note: its ok for our test assets here to fail since we're testing graceful failure. We
					// might have some pathological sound assets in the test asset directory.
					ESoundFileState::Type State = SoundFile->GetState();
					if (State == ESoundFileState::HAS_ERROR)
					{
						FString Name;
						SoundFile->GetName(Name);

						// Store the error in a list of error'd sound files
						if (!HasError.Contains(Name))
						{
							HasError.Add(Name);
						}
					}
					else if (State != ESoundFileState::IMPORTED)
					{
						bIsFinishedImporting = false;
					}
					else
					{
						FString Name;
						SoundFile->GetName(Name);

						// If this file's state is IMPORTED, then we add it to the list
						// of FinishedImporting sounds, then trigger the export process.
						if (!FinishedImporting.Contains(Name))
						{
							FinishedImporting.Add(Name);

							UE_LOG(LogUnrealAudio, Display, TEXT("Finished importing %s"), *Name);

							TSharedPtr<ISoundFileData> SoundFileData;
							SoundFile->GetSoundFileData(SoundFileData);

							// We're building up a filename with [NAME]_exported.[EXT] in the output directory
							const FString& SoundFileFullPath = SoundFileData->GetFilePath();
							FString BaseSoundFileName = FPaths::GetBaseFilename(SoundFileFullPath);
							FString SoundFileExtension = FPaths::GetExtension(SoundFileFullPath);
							FString SoundFilePath = FPaths::GetPath(SoundFileFullPath);
							FString OutputFileName = BaseSoundFileName + TEXT("_exported.") + SoundFileExtension;
							FString OutputPath = OutputDir / OutputFileName;

							UE_LOG(LogUnrealAudio, Display, TEXT("Now exporting to %s"), *OutputPath);

							UnrealAudioModule->ExportSound(SoundFileData, OutputPath);
						}
					}
				}

				// Sleep until all imports have finished. In-flight exports
				// will cause the UnrealAudio shutdown to block until they are finished.
				FPlatformProcess::Sleep(1);
			}

			return true;
		}
		else if (FPaths::FileExists(ImportSettings.SoundFilePath))
		{
			UE_LOG(LogUnrealAudio, Display, TEXT("Testing import, then export of a single sound source: %s."), *ImportSettings.SoundFilePath);
			UE_LOG(LogUnrealAudio, Display, TEXT("Import Format: %s"), *FString::Printf(TEXT("%s - %s"), ESoundFileFormat::ToStringMajor(ImportSettings.Format), ESoundFileFormat::ToStringMinor(ImportSettings.Format)));
			UE_LOG(LogUnrealAudio, Display, TEXT("Import SampleRate: %d"), ImportSettings.SampleRate);
			UE_LOG(LogUnrealAudio, Display, TEXT("Import EncodingQuality: %.2f"), ImportSettings.EncodingQuality);
			UE_LOG(LogUnrealAudio, Display, TEXT("Perform Peak Normalization: %s"), ImportSettings.bPerformPeakNormalization ? TEXT("Yes") : TEXT("No"));

			// Start the import process on the sound
			TSharedPtr<ISoundFile> ImportedSoundFile = UnrealAudioModule->ImportSound(ImportSettings);

			// Wait for the import to finish (block this thread for test-purposes). Normally, this would be non-blocking.
			ESoundFileState::Type State = ImportedSoundFile->GetState();
			while (State != ESoundFileState::IMPORTED)
			{
				FPlatformProcess::Sleep(1);

				State = ImportedSoundFile->GetState();
				if (State == ESoundFileState::HAS_ERROR)
				{
					UE_LOG(LogUnrealAudio, Error, TEXT("Failed to import sound source!"));
					UE_LOG(LogUnrealAudio, Error, TEXT("Error: %s"), ESoundFileError::ToString(ImportedSoundFile->GetError()));
					return false;
				}
			}

			UE_LOG(LogUnrealAudio, Display, TEXT("Succeeded importing sound source!"));

			// Now setup the export path
			FString SoundFileFullPath = ImportSettings.SoundFilePath;
			FString BaseSoundFileName = FPaths::GetBaseFilename(SoundFileFullPath);
			FString SoundFileExtension = FPaths::GetExtension(SoundFileFullPath);
			FString SoundFileDir = FPaths::GetPath(SoundFileFullPath);

			// Create the export directory if it doesn't exist
			FString OutputDir = SoundFileDir;
			if (!FPaths::DirectoryExists(OutputDir))
			{
				FPlatformFileManager::Get().GetPlatformFile().CreateDirectory(*OutputDir);
			}

			// Append _exported to the file path just to make it clear that this is the exported version of the file
			FString OutputPath = SoundFileDir / (BaseSoundFileName + TEXT("_exported.") + SoundFileExtension);

			// Start the export process. We can exit this test function since UnrealAudio will block on shutdown
			// until all background processes have finished. Normally the Editor will remain running during the export process.
			TSharedPtr<ISoundFileData> SoundFileData;
			ImportedSoundFile->GetSoundFileData(SoundFileData);
			UnrealAudioModule->ExportSound(SoundFileData, OutputPath);
			return true;
		}

		UE_LOG(LogUnrealAudio, Display, TEXT("Path %s is not a single file or a directory."), *ImportSettings.SoundFilePath);
		return false;
	}

}

#endif // #if ENABLE_UNREAL_AUDIO
