// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "UnrealAudioModule.h"
#include "UnrealAudioDeviceModule.h"
#include "UnrealAudioTests.h"

#if ENABLE_UNREAL_AUDIO

DECLARE_LOG_CATEGORY_EXTERN(LogUnrealAudio, Log, All);

namespace UAudio
{
	/** 
	* FUnrealAudioModule 
	* Concrete implementation of IUnrealAudioModule
	*/
	class FUnrealAudioModule : public IUnrealAudioModule
	{
	public:

		FUnrealAudioModule();
		~FUnrealAudioModule();

		// IUnrealaAudioModule Interface
		bool Initialize() override;
		bool Initialize(const FString& DeviceModuleName) override;
		void Shutdown() override;
		TSharedPtr<ISoundFile> ImportSound(const FSoundFileImportSettings& ImportSettings) override;
		void ExportSound(TSharedPtr<ISoundFileData> SoundFileData, const FString& ExportPath) override;

		class IUnrealAudioDeviceModule* GetDeviceModule();
		FName GetDefaultDeviceModuleName() const;

		// Internal API
		void IncrementBackgroundTaskCount();
		void DecrementBackgroundTaskCount();

	private:

		bool InitializeInternal();
		
		/** Loads the sound file parsing libs */
		bool LoadSoundFileLib();
		bool ShutdownSoundFileLib();

		/** Initializes unreal audio tests */
		static void InitializeTests(FUnrealAudioModule* Module);

		class IUnrealAudioDeviceModule* UnrealAudioDevice;
		FName ModuleName;

		/** Number of background tasks in-flight */
		FThreadSafeCounter NumBackgroundTasks;

		/** Handle to the sound file DLL */
		void* SoundFileDllHandle;


	};



}

#endif // #if ENABLE_UNREAL_AUDIO


