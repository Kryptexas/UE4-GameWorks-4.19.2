// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

namespace Audio {

	namespace EAudioMixerPlatformApi
	{
		enum Type
		{
			Wasapi,
			XAudio2,
			Ngs2,
			CoreAudio,
			OpenAL,
			Html5,
			Null,
		};
	}

	namespace EAudioMixerStreamDataFormat
	{
		enum Type
		{
			Unknown,
			Float,
			Double,
			Int16,
			Int24,
			Int32,
			Unsupported
		};
	}

	/**
	 * EAudioOutputStreamState
	 * Specifies the state of the output audio stream.
	 */
	namespace EAudioOutputStreamState
	{
		enum Type
		{
			/* The audio stream is shutdown or not uninitialized. */
			Closed,
		
			/* The audio stream is open but not running. */
			Open,

			/** The audio stream is open but stopped. */
			Stopped,
		
			/** The audio output stream is stopping. */
			Stopping,

			/** The audio output stream is open and running. */
			Running,
		};
	}

}
