// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IAudioExtensionPlugin.h"
#include "OculusAudioMixer.h"
#if PLATFORM_WINDOWS
#include "OculusAudioLegacy.h"
#endif
#include "OculusAudioSourceSettings.h"

/************************************************************************/
/* FOculusSpatializationPluginFactory                                   */
/* Handles initialization of the required Oculus Audio Spatialization   */
/* plugin.                                                              */
/************************************************************************/
class FOculusSpatializationPluginFactory : public IAudioSpatializationFactory
{
public:
	//~ Begin IAudioSpatializationFactory
	virtual FString GetDisplayName() override
	{
		static FString DisplayName = FString(TEXT("Oculus Audio"));
		return DisplayName;
	}

	virtual bool SupportsPlatform(EAudioPlatform Platform) override
	{
		if (Platform == EAudioPlatform::Windows || Platform == EAudioPlatform::Android)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

    virtual UClass* GetCustomSpatializationSettingsClass() const override { return UOculusAudioSourceSettings::StaticClass(); }

	virtual TAudioSpatializationPtr CreateNewSpatializationPlugin(FAudioDevice* OwningDevice) override;
	//~ End IAudioSpatializationFactory

	virtual TAmbisonicsMixerPtr CreateNewAmbisonicsMixer(FAudioDevice* OwningDevice) override;

};

class FOculusReverbPluginFactory : public IAudioReverbFactory
{
public:
    //~ Begin IAudioReverbFactory
    virtual FString GetDisplayName() override
    {
        static FString DisplayName = FString(TEXT("Oculus Audio"));
        return DisplayName;
    }

    virtual bool SupportsPlatform(EAudioPlatform Platform) override
    {
        if (Platform == EAudioPlatform::Windows)
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    virtual TAudioReverbPtr CreateNewReverbPlugin(FAudioDevice* OwningDevice) override;
    //~ End IAudioReverbFactory
};

