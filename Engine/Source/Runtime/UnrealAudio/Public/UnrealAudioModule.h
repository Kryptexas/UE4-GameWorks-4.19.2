// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

#include "ModuleInterface.h"
#include "UnrealAudioTypes.h"
#include "UnrealAudioSoundFile.h"

namespace UAudio
{
	class UNREALAUDIO_API IUnrealAudioModule :  public IModuleInterface
	{
	public:
		virtual ~IUnrealAudioModule() {}

		virtual bool Initialize()
		{
			return false;
		}

		virtual bool Initialize(const FString& DeviceModuleName)
		{
			return false;
		}

		virtual void Shutdown()
		{
		}

		/** 
		ImportSound
		@param ImportSettings Settings to use to import a sound file. Settings file contains all information
				needed to performt he import process.
		@return A shared pointer to an ISoundFile object.
		*/
		virtual	TSharedPtr<ISoundFile> ImportSound(const FSoundFileImportSettings& ImportSettings)
		{
			return nullptr;
		}

		/**
		ImportSound
		@param ImportSettings Settings to use to import a sound file. Settings file contains all information
		needed to performt he import process.
		@return A handle to an ISoundFile object.
		*/
		virtual	void ExportSound(TSharedPtr<ISoundFileData> SoundFileData, const FString& ExportPath)
		{
		}


	};
}



