// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once 

// Only enable unreal audio on windows or mac
#if PLATFORM_WINDOWS
#define ENABLE_UNREAL_AUDIO 1
#else
#define ENABLE_UNREAL_AUDIO 0
#endif

#include "UnrealAudioModule.h"

#if ENABLE_UNREAL_AUDIO

namespace UAudio
{
	/**
	* EDeviceError
	* The types of errors that can occur in IUnrealAudioDevice implementations.
	*/
	namespace EDeviceError
	{
		enum Type
		{
			WARNING,
			UNKNOWN,
			INVALID_PARAMETER,
			PLATFORM,
			SYSTEM,
			THREAD,
		};

		/** @return the stringified version of the enum passed in */
		static inline const TCHAR* ToString(EDeviceError::Type DeviceError)
		{
			switch (DeviceError)
			{
				case WARNING:			return TEXT("WARNING");
				case UNKNOWN:			return TEXT("UNKNOWN");
				case INVALID_PARAMETER:	return TEXT("INVALID_PARAMETER");
				case PLATFORM:			return TEXT("PLATFORM");
				case SYSTEM:			return TEXT("SYSTEM");
				case THREAD:			return TEXT("THREAD");
				default:				check(false);
			}
			return TEXT("");
		}
	}

	/**
	* EDeviceApi
	* Implemented platform-Specific audio device APIs.
	*/
	namespace EDeviceApi
	{
		enum Type
		{
			WASAPI,				// Windows
			XAUDIO2,			// Windows/XBox
			NGS2,				// PS4
			ALSA,				// Linux
			CORE_AUDIO,			// Mac/IOS
			OPENAL,				// Linux
			HTML5,				// Web
			DUMMY,				// DUMMY (no device api)
		};

		/** @return the stringified version of the enum passed in */
		static inline const TCHAR* ToString(EDeviceApi::Type DeviceApi)
		{
			switch (DeviceApi)
			{
			case WASAPI:		return TEXT("WASAPI");
			case XAUDIO2:		return TEXT("XAUDIO2");
			case NGS2:			return TEXT("NGS2");
			case ALSA:			return TEXT("ALSA");
			case CORE_AUDIO:	return TEXT("CORE_AUDIO");
			case OPENAL:		return TEXT("OPENAL");
			case HTML5:			return TEXT("HTML5");
			case DUMMY:			return TEXT("DUMMY");
			default:			check(false);
			}
			return TEXT("");
		}
	};

	/**
	* ESpeaker
	* An enumeration to specify speaker types
	*/
	namespace ESpeaker
	{
		enum Type
		{
			FRONT_LEFT,
			FRONT_RIGHT,
			FRONT_CENTER,
			LOW_FREQUENCY,
			BACK_LEFT,
			BACK_RIGHT,
			FRONT_LEFT_OF_CENTER,
			FRONT_RIGHT_OF_CENTER,
			BACK_CENTER,
			SIDE_LEFT,
			SIDE_RIGHT,
			TOP_CENTER,
			TOP_FRONT_LEFT,
			TOP_FRONT_CENTER,
			TOP_FRONT_RIGHT,
			TOP_BACK_LEFT,
			TOP_BACK_CENTER,
			TOP_BACK_RIGHT,
			UNUSED,
			SPEAKER_TYPE_COUNT
		};

		/** @return the stringified version of the enum passed in */
		static inline const TCHAR* ToString(ESpeaker::Type Speaker)
		{
			switch (Speaker)
			{
			case FRONT_LEFT:				return TEXT("FRONT_LEFT");
			case FRONT_RIGHT:				return TEXT("FRONT_RIGHT");
			case FRONT_CENTER:				return TEXT("FRONT_CENTER");
			case LOW_FREQUENCY:				return TEXT("LOW_FREQUENCY");
			case BACK_LEFT:					return TEXT("BACK_LEFT");
			case BACK_RIGHT:				return TEXT("BACK_RIGHT");
			case FRONT_LEFT_OF_CENTER:		return TEXT("FRONT_LEFT_OF_CENTER");
			case FRONT_RIGHT_OF_CENTER:		return TEXT("FRONT_RIGHT_OF_CENTER");
			case BACK_CENTER:				return TEXT("BACK_CENTER");
			case SIDE_LEFT:					return TEXT("SIDE_LEFT");
			case SIDE_RIGHT:				return TEXT("SIDE_RIGHT");
			case TOP_CENTER:				return TEXT("TOP_CENTER");
			case TOP_FRONT_LEFT:			return TEXT("TOP_FRONT_LEFT");
			case TOP_FRONT_CENTER:			return TEXT("TOP_FRONT_CENTER");
			case TOP_FRONT_RIGHT:			return TEXT("TOP_FRONT_RIGHT");
			case TOP_BACK_LEFT:				return TEXT("TOP_BACK_LEFT");
			case TOP_BACK_CENTER:			return TEXT("TOP_BACK_CENTER");
			case TOP_BACK_RIGHT:			return TEXT("TOP_BACK_RIGHT");
			case UNUSED:					return TEXT("UNUSED");
			default:						return TEXT("UKNOWN");
			}
			return TEXT("");
		}
	};

}


#endif // #if ENABLE_UNREAL_AUDIO

